#include <CodeGen.hpp>
#include <Parser.hpp>
#include <ModuleUnit.hpp>

#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#ifdef PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
#endif
#include <codecvt>

std::string gen_random(const int len)
{
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    srand((unsigned)time(NULL));

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}

using namespace Parsing;

std::string LLVMTypeToString(std::shared_ptr<CodeType> base, llvm::Type *type)
{
    switch (type->getTypeID())
    {
    case llvm::Type::HalfTyID:
    case llvm::Type::BFloatTyID:
        return "Float16";
    case llvm::Type::FloatTyID:
        return "Float32";
    case llvm::Type::DoubleTyID:
        return "Float64";
    case llvm::Type::X86_FP80TyID:
        return "Float80";
    case llvm::Type::FP128TyID:
    case llvm::Type::PPC_FP128TyID:
        return "Float128";
    case llvm::Type::IntegerTyID:
    {
        std::string s = "";
        if (!base->isSigned)
            s += "u";
        int width = static_cast<llvm::IntegerType *>(base->type)->getBitWidth();
        if (width == 8 && base->isChar)
            s = "char";
        else if (width == 1 && base->isBool)
            s = "bool";
        else
            s += std::to_string(width);
        return s;
    }
    case llvm::Type::FunctionTyID:
    {
        std::string s = "(";
        auto func = static_cast<llvm::FunctionType *>(base->type);
        for (auto param : func->params())
        {
        }
        s += ")";
    }
    case llvm::Type::PointerTyID:
        return "&" + LLVMTypeToString(base, type->getPointerElementType());
    default:
        break;
    }
    return "";
}

std::string TypeToString(std::shared_ptr<CodeType> type)
{
    if (auto templ = std::dynamic_pointer_cast<TemplateCodeType>(type))
    {
        std::string s = "";
        if (templ->type->isPointerTy())
            s += "&";
        s += templ->node.GetParent()->findSymbol(&templ->node);
        return s;
    }
    else if (auto func = std::dynamic_pointer_cast<FunctionCodeType>(type))
    {
        std::string s = "(";
        // auto func = static_cast<llvm::FunctionType *>(base->type);
        const auto &parameters = func->GetParameters();
        for (int i = 0; i < parameters.size(); i++)
        {
            s += TypeToString(parameters[i]);
            if (i != parameters.size() - 1)
                s += ", ";
        }
        s += ") => ";

        if (!func->type->isVoidTy())
            s += TypeToString(func);

        return s;
    }
    else
        return LLVMTypeToString(type, type->type);
}

// SymbolNode CodeGeneration::rootSymbols(nullptr);

std::shared_ptr<CodeType> CodeGeneration::LiteralType(const SyntaxNode &node)
{
    switch (node.GetType())
    {
    case SyntaxType::Integer:
    {
        auto value = node.As<IntegerSyntax>().GetValue();
        return std::make_shared<CodeType>(llvm::IntegerType::get(context, GetNumBits(value)));
    }
    case SyntaxType::Floating:
        return std::make_shared<CodeType>(llvm::Type::getDoubleTy(context));
    // case SyntaxType::Char:
    //     return std::make_shared<CodeType>(llvm::Type::getDoubleTy(context));
    case SyntaxType::Boolean:
        return std::make_shared<CodeType>(llvm::Type::getInt1Ty(context), false, false, true);
    case SyntaxType::String:
        return std::make_shared<CodeType>(llvm::ArrayType::get(llvm::Type::getInt8Ty(context), node.As<StringSyntax>().GetValue().size() + 1));
    case SyntaxType::ArrayLiteral:
    {
        auto arr = node.As<ArrayLiteral>();

        if (arr.GetValues().size() > 0)
        {
            // auto type = LiteralType(arr.GetValues()[0]->GetExpression());
            // return std::make_shared<CodeType>(llvm::ArrayType::get(type->type, node.As<ArrayLiteral>().GetValues().size() + 1));
            return TypeFromArrayInitializer(node);
        }
        else
        {
            return std::make_shared<CodeType>(llvm::ArrayType::get(llvm::Type::getInt8Ty(context), node.As<ArrayLiteral>().GetValues().size() + 1));
        }
    }
    default:
        return nullptr;
    }
}

std::shared_ptr<CodeType> CodeGeneration::TypeType(const SyntaxNode &node)
{
    switch (node.GetType())
    {
    case SyntaxType::PrimitiveType:
    {
        auto type = node.As<PrimitiveType>().GetToken();
        switch (type.type)
        {
        case TokenType::Int:
            return std::make_shared<CodeType>(llvm::IntegerType::get(context, type.ivalue == 0 ? 32 : type.ivalue), true);
        case TokenType::Uint:
            return std::make_shared<CodeType>(llvm::IntegerType::get(context, type.ivalue == 0 ? 32 : type.ivalue), false);
        case TokenType::Float:
            return std::make_shared<CodeType>(llvm::Type::getDoubleTy(context), false);
        case TokenType::Char:
            return std::make_shared<CodeType>(llvm::IntegerType::get(context, 8), false, true);
        case TokenType::Bool:
            return std::make_shared<CodeType>(llvm::Type::getInt1Ty(context), false, false, true);

        default:
            break;
        }
    }
    case SyntaxType::IdentifierType:
    {
        if (auto sym = FindSymbolInScope(node.As<IdentifierType>().GetToken().raw))
        {
            switch (sym->GetType())
            {
            case SymbolNodeType::TemplateNode:
                return dynamic_cast<TemplateNode *>(sym)->GetTemplate();
                // return std::make_shared<TemplateCodeValue>(static_cast<llvm::StructType *>(dynamic_cast<TemplateNode *>(sym)->GetTemplate()->type), *dynamic_cast<TemplateNode *>(sym));
            case SymbolNodeType::TypeAliasNode:
                return dynamic_cast<TypeAliasNode *>(sym)->GetReferencedType();
                // return std::make_shared<TemplateCodeValue>(static_cast<llvm::StructType *>(dynamic_cast<TemplateNode *>(sym)->GetTemplate()->type), *dynamic_cast<TemplateNode *>(sym));
            case SymbolNodeType::SpecNode:
                // return dynamic_cast<SpecNode *>(sym).
                return std::make_shared<SpecCodeType>(nullptr, *dynamic_cast<SpecNode *>(sym));
            default:
                return nullptr;
            }
        }
        ThrowCompilerError(
            ErrorType::Type, ErrorCode::UnkownType,
            "Unkown type `" + node.As<IdentifierType>().GetToken().raw + "`",
            Range(node.GetStart(), node.GetEnd()));
        return nullptr;
    }
    case SyntaxType::ArrayType:
    {
        auto arr = node.As<ArrayType>();
        auto type = TypeType(arr.GetArrayType());
        auto &size = arr.GetArraySize();
        if (size.GetType() == SyntaxType::Integer)
            return std::make_shared<CodeType>(llvm::ArrayType::get(type->type, size.As<IntegerSyntax>().GetValue()));
        else
            return nullptr;
    }
    case SyntaxType::ReferenceType:
    {
        auto arr = node.As<ReferenceType>();
        auto type = TypeType(arr.GetReferenceType());
        return std::make_shared<CodeType>(llvm::PointerType::get(type->type, 0));
    }
    case SyntaxType::FunctionType:
    {
        auto arr = node.As<FunctionType>();
        auto type = arr.GetRetType() ? TypeType(*arr.GetRetType()) : nullptr;
        std::vector<llvm::Type *> parameters;
        for (auto a : arr.GetParameters())
            parameters.push_back(TypeType(*a)->type);
        if (type)
            return std::make_shared<CodeType>(llvm::FunctionType::get(type->type, parameters, false)->getPointerTo());
        else
            return std::make_shared<CodeType>(llvm::FunctionType::get(llvm::Type::getVoidTy(context), parameters, false)->getPointerTo());
    }
    case SyntaxType::GenericType:
    {
        auto &generic = node.As<GenericType>();
        auto &identifier = generic.GetBaseType()->As<IdentifierType>();
        auto symbol = FindSymbolInScope(identifier.GetToken().raw);
        if (symbol)
        {
            if (const auto templ = symbol->Cast<TemplateNode>())
            {
                if (templ->GetGeneric()->GetParameters().size() != generic.GetArguments().size())
                {
                    ThrowCompilerError(
                        ErrorType::Generic, ErrorCode::ArgMisMatch,
                        std::string("Generic type expected ") + std::to_string(templ->GetGeneric()->GetParameters().size()) + " arguments but recieved " + std::to_string(generic.GetArguments().size()),
                        Range(generic.GetLeft().position.start, generic.GetRight().position.end));
                }

                auto lastPoint = insertPoint;
                insertPoint = templ->GetParent();

                auto structType = llvm::StructType::create(context, GenerateMangledTypeName(identifier.GetToken().raw));
                const auto newTempl = symbol->GetParent()->AddChild<TemplateNode>(identifier.GetToken().raw + symbol->GetParent()->GenerateName(), structType);
                insertPoint = lastPoint;

                auto it = templ->GetGeneric()->GetParameters().begin();
                bool bad = false;
                for (auto p : generic.GetArguments())
                {
                    try
                    {
                        auto rType = TypeType(*p);
                        for (auto constrait : (*it)->GetConstraints())
                        {
                            if (!TypeImplements(rType, &(std::dynamic_pointer_cast<SpecCodeType>(TypeType(*constrait))->GetNode())))
                            {
                                ThrowCompilerError(
                                    ErrorType::Generic, ErrorCode::ArgMisMatch,
                                    "Generic argument `" + TypeToString(rType) + "` does not conform to constraints!",
                                    Range(p->GetStart(), p->GetEnd()));
                            }
                        }
                        auto found = templ->findSymbol((*it)->GetIdentifier().raw);
                        if (found != templ->children.end())
                        {
                            if (found->second->GetType() == SymbolNodeType::TypeAliasNode)
                            {
                                auto &node = found->second->As<TypeAliasNode>();

                                node.SetReferencedType(rType);
                                newTempl->AddChild<TypeAliasNode>(found->first, node);
                            }
                        }
                    }
                    catch (const BaseException &ex)
                    {
                        bad = true;
                    }
                    it++;
                }
                if (bad)
                    ThrowMidCompilerErrorSnippet(
                        ErrorType::Generic, ErrorCode::ArgMisMatch,
                        "Aborting due to generic type error",
                        Range(),
                        CreateSnippet(Range(templ->GetGeneric()->GetLeft().position.start, templ->GetGeneric()->GetRight().position.end), "Generic parameters have constraints:"));
                insertPoint = newTempl;

                Use(CodeGeneration::Using::NoBlock);

                templ->GetBody()->CodeGen(*this);

                structType->setBody(newTempl->GetMembers());

                insertPoint = lastPoint;

                return newTempl->GetTemplate();
            }
            else if (const auto typeAlias = symbol->Cast<TypeAliasNode>())
            {
                if (typeAlias->GetGeneric()->GetParameters().size() != generic.GetArguments().size())
                {
                    ThrowCompilerError(
                        ErrorType::Generic, ErrorCode::ArgMisMatch,
                        std::string("Generic type expected ") + std::to_string(typeAlias->GetGeneric()->GetParameters().size()) + " arguments but recieved " + std::to_string(generic.GetArguments().size()),
                        Range(generic.GetLeft().position.start, generic.GetRight().position.end));
                }

                auto lastPoint = insertPoint;
                insertPoint = typeAlias->GetParent();

                const auto newTempl = symbol->GetParent()->AddChild<TypeAliasNode>(identifier.GetToken().raw + symbol->GetParent()->GenerateName());
                insertPoint = newTempl;

                auto it = typeAlias->GetGeneric()->GetParameters().begin();
                bool bad = false;
                for (auto p : generic.GetArguments())
                {
                    try
                    {
                        auto rType = TypeType(*p);
                        for (auto constrait : (*it)->GetConstraints())
                        {
                            if (!TypeImplements(rType, &(std::dynamic_pointer_cast<SpecCodeType>(TypeType(*constrait))->GetNode())))
                            {
                                ThrowCompilerError(
                                    ErrorType::Generic, ErrorCode::ArgMisMatch,
                                    "Generic argument `" + TypeToString(rType) + "` does not conform to constraints!",
                                    Range(p->GetStart(), p->GetEnd()));
                            }
                        }
                        auto found = typeAlias->findSymbol((*it)->GetIdentifier().raw);
                        if (found != typeAlias->children.end())
                        {
                            if (found->second->GetType() == SymbolNodeType::TypeAliasNode)
                            {
                                auto &node = found->second->As<TypeAliasNode>();

                                node.SetReferencedType(TypeType(*p));
                                insertPoint->AddChild<TypeAliasNode>(found->first, node);
                            }
                        }
                    }
                    catch (const BaseException &exc)
                    {
                        bad = true;
                    }
                    it++;
                }
                if (bad)
                    ThrowMidCompilerErrorSnippet(
                        ErrorType::Generic, ErrorCode::ArgMisMatch,
                        "Aborting due to generic type error",
                        Range(),
                        CreateSnippet(Range(typeAlias->GetGeneric()->GetLeft().position.start, typeAlias->GetGeneric()->GetRight().position.end), "Generic parameters have constraints:"));

                Use(CodeGeneration::Using::NoBlock);

                // typeAlias->GetBody()->CodeGen(*this);
                newTempl->SetReferencedType(TypeType(*typeAlias->GetBody()));

                insertPoint = lastPoint;

                return newTempl->GetReferencedType();
            }
        }
        else
        {
            ThrowCompilerError(
                ErrorType::TemplateInitializer, ErrorCode::CannotFind,
                std::string("Cannot find type ") + identifier.GetToken().raw + " in scope!",
                Range(identifier.GetStart(), identifier.GetEnd()));
        }
    }
    default:
        ThrowCompilerError(
            ErrorType::Type, ErrorCode::UnkownType,
            "Unkown type",
            Range(node.GetStart(), node.GetEnd()));

        return nullptr;
        break;
    }
}

bool CodeGeneration::TypeImplements(const SyntaxNode &node, SpecNode *spec)
{
    switch (node.GetType())
    {
    case SyntaxType::PrimitiveType:
    {
        auto type = node.As<PrimitiveType>().GetToken();
        switch (type.type)
        {
        case TokenType::Int:
        case TokenType::Uint:
        case TokenType::Float:

        case TokenType::Bool:
            return true;
        default:
            break;
        }
    }
    case SyntaxType::IdentifierType:
    {
        if (auto sym = FindSymbolInScope(node.As<IdentifierType>().GetToken().raw))
        {
            switch (sym->GetType())
            {
            case SymbolNodeType::TemplateNode:

                return TypeImplements(dynamic_cast<TemplateNode *>(sym)->GetTemplate(), spec);
                // return std::make_shared<TemplateCodeValue>(static_cast<llvm::StructType *>(dynamic_cast<TemplateNode *>(sym)->GetTemplate()->type), *dynamic_cast<TemplateNode *>(sym));
            case SymbolNodeType::TypeAliasNode:
                return TypeImplements(dynamic_cast<TypeAliasNode *>(sym)->GetReferencedType(), spec);
                // return dynamic_cast<TypeAliasNode *>(sym)->GetReferencedType();
                // return std::make_shared<TemplateCodeValue>(static_cast<llvm::StructType *>(dynamic_cast<TemplateNode *>(sym)->GetTemplate()->type), *dynamic_cast<TemplateNode *>(sym));
            // case SymbolNodeType::SpecNode:
            // return dynamic_cast<SpecNode *>(sym).
            // return std::make_shared<SpecCodeType>(nullptr, *dynamic_cast<SpecNode *>(sym));
            default:
                return false;
            }
        }
        ThrowCompilerError(
            ErrorType::Type, ErrorCode::UnkownType,
            "Unkown type `" + node.As<IdentifierType>().GetToken().raw + "`",
            Range(node.GetStart(), node.GetEnd()));
        return false;
    }
    case SyntaxType::GenericType:
    {
        // auto &generic = node.As<GenericType>();
        // auto &identifier = generic.GetBaseType()->As<IdentifierType>();
        // auto symbol = FindSymbolInScope(identifier.GetToken().raw);
        // if (symbol)
        // {
        //     if (const auto templ = symbol->Cast<TemplateNode>())
        //     {
        //         if (templ->GetGeneric()->GetParameters().size() != generic.GetArguments().size())
        //         {
        //             ThrowCompilerError(
        //                 ErrorType::Generic, ErrorCode::ArgMisMatch,
        //                 std::string("Generic type expected ") + std::to_string(templ->GetGeneric()->GetParameters().size()) + " arguments but recieved " + std::to_string(generic.GetArguments().size()),
        //                 Range(generic.GetLeft().position.start, generic.GetRight().position.end));
        //         }

        //         auto lastPoint = insertPoint;
        //         insertPoint = templ->GetParent();

        //         auto structType = llvm::StructType::create(context, GenerateMangledTypeName(identifier.GetToken().raw));
        //         const auto newTempl = symbol->GetParent()->AddChild<TemplateNode>(identifier.GetToken().raw + symbol->GetParent()->GenerateName(), structType);
        //         insertPoint = lastPoint;

        //         auto it = templ->GetGeneric()->GetParameters().begin();
        //         for (auto p : generic.GetArguments())
        //         {

        //             auto found = templ->findSymbol((*it)->GetIdentifier().raw);
        //             if (found != templ->children.end())
        //             {
        //                 if (found->second->GetType() == SymbolNodeType::TypeAliasNode)
        //                 {
        //                     auto &node = found->second->As<TypeAliasNode>();

        //                     node.SetReferencedType(TypeType(*p));
        //                     newTempl->AddChild<TypeAliasNode>(found->first, node);
        //                 }
        //             }
        //             it++;
        //         }
        //         insertPoint = newTempl;

        //         Use(CodeGeneration::Using::NoBlock);

        //         templ->GetBody()->CodeGen(*this);

        //         structType->setBody(newTempl->GetMembers());

        //         insertPoint = lastPoint;

        //         return newTempl->GetTemplate();
        //     }
        //     else if (const auto typeAlias = symbol->Cast<TypeAliasNode>())
        //     {
        //         if (typeAlias->GetGeneric()->GetParameters().size() != generic.GetArguments().size())
        //         {
        //             ThrowCompilerError(
        //                 ErrorType::Generic, ErrorCode::ArgMisMatch,
        //                 std::string("Generic type expected ") + std::to_string(typeAlias->GetGeneric()->GetParameters().size()) + " arguments but recieved " + std::to_string(generic.GetArguments().size()),
        //                 Range(generic.GetLeft().position.start, generic.GetRight().position.end));
        //         }

        //         auto lastPoint = insertPoint;
        //         insertPoint = typeAlias->GetParent();

        //         const auto newTempl = symbol->GetParent()->AddChild<TypeAliasNode>(identifier.GetToken().raw + symbol->GetParent()->GenerateName());
        //         insertPoint = newTempl;

        //         auto it = typeAlias->GetGeneric()->GetParameters().begin();
        //         for (auto p : generic.GetArguments())
        //         {
        //             auto found = typeAlias->findSymbol((*it)->GetIdentifier().raw);
        //             if (found != typeAlias->children.end())
        //             {
        //                 if (found->second->GetType() == SymbolNodeType::TypeAliasNode)
        //                 {
        //                     auto &node = found->second->As<TypeAliasNode>();

        //                     node.SetReferencedType(TypeType(*p));
        //                     insertPoint->AddChild<TypeAliasNode>(found->first, node);
        //                 }
        //             }
        //             it++;
        //         }

        //         Use(CodeGeneration::Using::NoBlock);

        //         // typeAlias->GetBody()->CodeGen(*this);
        //         newTempl->SetReferencedType(TypeType(*typeAlias->GetBody()));

        //         insertPoint = lastPoint;

        //         return newTempl->GetReferencedType();
        //     }
        // }
        // else
        // {
        //     ThrowCompilerError(
        //         ErrorType::TemplateInitializer, ErrorCode::CannotFind,
        //         std::string("Cannot find type ") + identifier.GetToken().raw + " in scope!",
        //         Range(identifier.GetStart(), identifier.GetEnd()));
        // }
    }
    default:
        ThrowCompilerError(
            ErrorType::Type, ErrorCode::UnkownType,
            "Unkown type",
            Range(node.GetStart(), node.GetEnd()));

        return false;
        break;
    }
}

bool CodeGeneration::TypeImplements(std::shared_ptr<CodeType> type, SpecNode *spec)
{
    if (auto templ = std::dynamic_pointer_cast<TemplateCodeType>(type))
    {
        const auto &specs = templ->GetNode().GetImplementedSpecs();
        auto found = std::find(specs.begin(), specs.end(), spec);
        if (found != specs.end())
            return true;
    }
    else if (auto templ = std::dynamic_pointer_cast<ArrayCodeType>(type))
    {
    }
    else if (auto templ = std::dynamic_pointer_cast<FunctionCodeType>(type))
    {
    }
    else if (auto templ = std::dynamic_pointer_cast<SpecCodeType>(type))
    {
    }
    else
    {
        switch (type->type->getTypeID())
        {
        case llvm::Type::IntegerTyID:
            if (type->type->getIntegerBitWidth() >= 8)
                return true;
            break;
        case llvm::Type::FloatTyID:
            return true;

        default:
            break;
        }
    }

    return false;
}

std::shared_ptr<CodeValue> CodeGeneration::Cast(std::shared_ptr<CodeValue> value, std::shared_ptr<CodeType> toType, bool implicit)
{
    auto vt = value->type->type->getTypeID();
    auto tt = toType->type->getTypeID();
    if (vt == tt && vt == llvm::Type::FloatTyID)
        return std::move(value);
    else if (vt == tt && vt == llvm::Type::DoubleTyID)
        return std::move(value);
    else if (value->type->type == toType->type)
        return std::move(value);
    else if ((vt == llvm::Type::FloatTyID || vt == llvm::Type::DoubleTyID) && (tt == llvm::Type::FloatTyID || tt == llvm::Type::DoubleTyID))
    {
        return std::make_shared<CodeValue>(builder.CreateFPCast(value->value, toType->type), toType);
    }
    else if ((vt == llvm::Type::FloatTyID || vt == llvm::Type::DoubleTyID) && (tt == llvm::Type::IntegerTyID))
    {
        if (!implicit)
        {
            if (toType->isSigned)
                return std::make_shared<CodeValue>(builder.CreateFPToSI(value->value, toType->type), toType);
            else
                return std::make_shared<CodeValue>(builder.CreateFPToUI(value->value, toType->type), toType);
        }
        else
        {

            ThrowCompilerError(
                ErrorType::Cast, ErrorCode::NoImplicitCast,
                "Cannot implicitly convert floating value to integer value!",
                currentRange);
            return nullptr;
        }
    }
    else if ((tt == llvm::Type::FloatTyID || tt == llvm::Type::DoubleTyID) && (vt == llvm::Type::IntegerTyID))
    {
        if (toType->isSigned)
            return std::make_shared<CodeValue>(builder.CreateSIToFP(value->value, toType->type), toType);
        else
            return std::make_shared<CodeValue>(builder.CreateUIToFP(value->value, toType->type), toType);
    }
    else if (value->type->type != toType->type || value->type->isSigned != toType->isSigned)
    {
        auto val = builder.CreateIntCast(value->value, toType->type, toType->isSigned);
        return std::make_shared<CodeValue>(val, toType);
    }
    return nullptr;
}

// llvm::Value *LoadIfAlloca(llvm::Value *value)
// {
//     if (value->getValueID() == llvm::Instruction::Alloca + llvm::Value::InstructionVal && initializer->GetType() == Parsing::SyntaxType::IdentifierExpression) // If the initializer is a variable, load it and store it
//     {
//         auto load = std::make_shared<CodeValue>(gen.GetBuilder().CreateLoad(init->value), init->type);
//     }
// }

uint8_t CodeGeneration::GetNumBits(uint64_t val)
{
    if (val <= UINT8_MAX)
        return 8;
    else if (val <= UINT16_MAX)
        return 16;
    else if (val <= UINT32_MAX)
        return 32;
    else
        return 64;
}

std::shared_ptr<TemplateCodeType> CodeGeneration::TypeFromObjectInitializer(const SyntaxNode &object)
{
    if (object.GetType() == SyntaxType::ObjectInitializer)
    {
        const auto &init = object.As<ObjectInitializer>();

        auto structType = llvm::StructType::create(context, GenerateMangledTypeName("anon"));
        auto node = insertPoint->AddChild<TemplateNode>("anon" + insertPoint->GenerateName(), structType);
        std::vector<llvm::Type *> types;
        for (const auto &val : init.GetValues())
        {
            if (val->GetValue().GetType() == SyntaxType::ObjectInitializer)
            {
                auto value = TypeFromObjectInitializer(val->GetValue());
                types.push_back(value->type);
                node->AddChild<VariableNode>(val->GetKey().raw, std::make_shared<CodeValue>(nullptr, value));
            }
            else
            {
                auto gen = val->GetValue().CodeGen(*this);
                types.push_back(gen->type->type);
                node->AddChild<VariableNode>(val->GetKey().raw, gen);
            }
        }
        structType->setBody(types);
        return std::make_shared<TemplateCodeType>(structType, *node);
    }
    return nullptr;
}

std::shared_ptr<CodeType> CodeGeneration::TypeFromArrayInitializer(const SyntaxNode &object)
{
    if (object.GetType() == SyntaxType::ArrayLiteral)
    {
        const auto &init = object.As<ArrayLiteral>();

        std::shared_ptr<CodeType> currentType;
        uint64_t len = 0;

        for (const auto val : init.GetValues())
        {
            auto valLen = val->GetLength();
            if (valLen == 0)
            {
                ThrowCompilerError(
                    ErrorType::ArrayLiteral, ErrorCode::Const,
                    "Array literal length must be const!",
                    Range(val->GetStart(), val->GetEnd()));
            }
            if (!currentType)
                currentType = LiteralType(val->GetExpression());
            else
            {
                auto next = LiteralType(val->GetExpression());
                auto cType = currentType->type->getTypeID();
                auto nType = next->type->getTypeID();
                if (cType == llvm::Type::IntegerTyID && nType == llvm::Type::IntegerTyID)
                {
                    if (currentType->type->getIntegerBitWidth() < next->type->getIntegerBitWidth())
                        currentType = next;
                }
                else if (cType == llvm::Type::FloatTyID && nType == llvm::Type::DoubleTyID)
                    currentType = next;
                else
                    Cast(val->CodeGen(*this), currentType);
            }
        }
        auto arr = llvm::ArrayType::get(currentType->type, init.GetValues().size());
        return std::make_shared<ArrayCodeType>(arr, currentType);
        // structType->setBody(types);
        // return std::make_shared<TemplateCodeType>(structType, *node);
    }
    return nullptr;
}

std::shared_ptr<CodeValue> CodeGeneration::FollowDotChain(const SyntaxNode &node)
{

    if (node.GetType() != SyntaxType::BinaryExpression)
        return nullptr;

    const auto &bin = node.As<BinaryExpression>();

    if (bin.GetRHS().GetType() != SyntaxType::IdentifierExpression)
        return nullptr;

    if (bin.GetLHS().GetType() == SyntaxType::IdentifierExpression)
    {
        bool isReference = IsUsed(Using::Reference);
        Use(Using::Reference);

        auto left = bin.GetLHS().CodeGen(*this);

        if (!isReference)
            UnUse(Using::Reference);

        if (left->value == nullptr)
        {
            if (auto templ = std::dynamic_pointer_cast<TemplateCodeType>(left->type))
            {
                const auto &templNode = templ->GetNode();

                auto found = templNode.findSymbol(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
                if (found != templNode.children.end())
                {
                    if (found->second->GetType() == SymbolNodeType::FunctionNode)
                    {
                        if (!found->second->As<FunctionNode>().GetFunction()->GetFunctionType()->IsMember())
                        {
                            return std::static_pointer_cast<CodeValue>(found->second->As<FunctionNode>().GetFunction());
                        }
                        else
                        {
                            ThrowCompilerError(
                                ErrorType::FollowDotChain, ErrorCode::NonInstance,
                                "Function must be called with instance value!",
                                Range(bin.GetLHS().GetStart(), bin.GetRHS().GetEnd()));
                        }
                    }
                    else
                    {
                        ThrowCompilerError(
                            ErrorType::FollowDotChain, ErrorCode::NonInstance,
                            "Variable cannot be accessed from non instance value!",
                            Range(bin.GetLHS().GetStart(), bin.GetRHS().GetEnd()));
                    }
                }
            }
        }
        else
        {
            if (left->type->type->isPointerTy())
            {
                auto loaded = builder.CreateLoad(left->type->type, left->value);
                left->type->type = left->type->type->getPointerElementType();
                left = std::make_shared<CodeValue>(loaded, left->type);
            }
            dotExprBase = left;

            if (auto templ = std::dynamic_pointer_cast<TemplateCodeType>(left->type))
            {
                const auto &templNode = templ->GetNode();

                auto found = templNode.findSymbol(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
                if (found != templNode.children.end())
                {
                    if (found->second->GetType() == SymbolNodeType::FunctionNode)
                    {
                        return std::static_pointer_cast<CodeValue>(found->second->As<FunctionNode>().GetFunction());
                    }
                    else
                    {
                        if (IsUsed(CodeGeneration::Using::Reference)) // Return reference and don't load
                        {
                            auto index = templNode.IndexOf(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
                            auto gep = builder.CreateStructGEP(templ->type, left->value, index);

                            auto value = std::make_shared<CodeValue>(gep, found->second->As<VariableNode>().GetVariable()->type);
                            // value->type->type = gep->getType();
                            return value;
                        }
                        else // Load value
                        {
                            auto index = templNode.IndexOf(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
                            auto gep = builder.CreateStructGEP(templ->type, left->value, index);

                            auto load = builder.CreateLoad(templ->type, gep);
                            auto value = std::make_shared<CodeValue>(load, found->second->As<VariableNode>().GetVariable()->type);
                            value->type->type = load->getType();
                            return value;
                        }
                    }
                }
                else
                {
                    ThrowCompilerError(
                        ErrorType::FollowDotChain, ErrorCode::CannotFind,
                        std::string("Cannot find value ") + bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw + " in type " + templ->GetNode().GetParent()->findSymbol((SymbolNode *)&templNode),
                        Range(bin.GetLHS().GetStart(), bin.GetRHS().GetEnd()));
                }
            }
        }
    }
    else
    {
        bool isReference = IsUsed(Using::Reference);
        Use(Using::Reference);

        auto left = FollowDotChain(bin.As<BinaryExpression>().GetLHS());

        if (!isReference)
            UnUse(Using::Reference);

        if (auto templ = std::static_pointer_cast<TemplateCodeType>(left->type))
        {
            const auto &templNode = templ->GetNode();
            auto found = templNode.findSymbol(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
            if (found != templNode.children.end())
            {
                if (found->second->GetType() == SymbolNodeType::FunctionNode)
                {
                    return std::static_pointer_cast<CodeValue>(found->second->As<FunctionNode>().GetFunction());
                }
                else
                {
                    auto found = templNode.findSymbol(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
                    if (IsUsed(CodeGeneration::Using::Reference)) // Return reference and don't load
                    {
                    }
                    else // Load value
                    {
                        auto index = templNode.IndexOf(bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw);
                        auto gep = builder.CreateStructGEP(templ->type, left->value, index);

                        auto load = builder.CreateLoad(templ->type, gep);
                        auto value = std::make_shared<CodeValue>(load, found->second->As<VariableNode>().GetVariable()->type);
                        value->type->type = load->getType();
                        return value;
                    }
                }
            }
            else
            {
                ThrowCompilerError(
                    ErrorType::FollowDotChain, ErrorCode::CannotFind,
                    std::string("Cannot find value ") + bin.GetRHS().As<IdentifierExpression>().GetIdentiferToken().raw + " in type " + templ->GetNode().GetParent()->findSymbol((SymbolNode *)&templNode),
                    Range(bin.GetLHS().GetStart(), bin.GetRHS().GetEnd()));
            }
        }
    }
    return nullptr;
}

void CodeGeneration::GenerateMain()
{
    if (auto programMain = dynamic_cast<FunctionNode *>(FindSymbolInScope("main")))
    {
        std::vector<llvm::Type *> parameters;
        parameters.push_back(llvm::IntegerType::get(context, 32));
        parameters.push_back(llvm::IntegerType::get(context, 8)->getPointerTo()->getPointerTo());

        auto functionType = llvm::FunctionType::get(llvm::IntegerType::get(context, 32), parameters, false);
        auto mainFunc = llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, 0, "main", module);

        auto insertPoint = llvm::BasicBlock::Create(context, "entry", mainFunc);
        builder.SetInsertPoint(insertPoint);

        builder.CreateCall(programMain->GetFunction()->Function(), std::vector<llvm::Value *>());
        builder.CreateRet(llvm::ConstantInt::get(llvm::IntegerType::get(context, 32), 0));
    }
}

namespace Parsing
{
    std::shared_ptr<CodeValue> VariableDeclerationStatement::CodeGen(CodeGeneration &gen) const
    {
        if (initializer && initializer->GetType() == SyntaxType::TemplateInitializer)
        {
            const auto &init = initializer->As<TemplateInitializer>();
            auto type = gen.TypeType(init.GetTemplateType());
            gen.SetCurrentType(type);
            // auto symbol = gen.FindSymbolInScope(init.GetIdentifier().raw);

            if (keyword.type == TokenType::Const) // Constant variable
            {
            }
            else if (keyword.type == TokenType::Let)
            {
                auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw); // Allocate the variable on the stack

                auto varValue = std::make_shared<CodeValue>(inst, type);
                gen.SetCurrentVar(varValue);
                initializer->CodeGen(gen);

                gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                return varValue;
            }
        }
        else if (initializer && initializer->GetType() == SyntaxType::ObjectInitializer)
        {
            if (type == nullptr)
            {
                if (keyword.type == TokenType::Const) // Constant variable
                {
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto type = gen.TypeFromObjectInitializer(*initializer);
                    auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw); // Allocate the variable on the stack

                    auto varValue = std::make_shared<CodeValue>(inst, type);
                    gen.SetCurrentVar(varValue);

                    initializer->CodeGen(gen);

                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
            }
            else
            {
                if (keyword.type == TokenType::Const) // Constant variable
                {
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto type = gen.TypeType(*this->type);
                    auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw); // Allocate the variable on the stack

                    auto varValue = std::make_shared<CodeValue>(inst, type);
                    gen.SetCurrentVar(varValue);

                    initializer->CodeGen(gen);

                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
                return nullptr;
            }
        }
        else if (initializer && initializer->GetType() == SyntaxType::ArrayLiteral)
        {
            if (type == nullptr)
            {
                if (keyword.type == TokenType::Const) // Constant variable
                {
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto type = gen.TypeFromArrayInitializer(*initializer);
                    auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw); // Allocate the variable on the stack

                    auto varValue = std::make_shared<CodeValue>(inst, type);
                    gen.SetCurrentVar(varValue);

                    initializer->CodeGen(gen);

                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
            }
            else
            {
                if (keyword.type == TokenType::Const) // Constant variable
                {
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto inst = gen.CreateEntryBlockAlloca(gen.TypeType(*this->type)->type, identifier.raw); // Allocate the variable on the stack

                    auto varValue = std::make_shared<CodeValue>(inst, gen.TypeType(*this->type));
                    gen.SetCurrentVar(varValue);

                    initializer->CodeGen(gen);

                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
                return nullptr;
            }
        }
        else
        {
            auto init = initializer ? initializer->CodeGen(gen) : nullptr;
            std::shared_ptr<CodeType> type;
            if (initializer && !this->type)
                type = init->type;
            else if (this->type)
                type = gen.TypeType(*this->type); // Get type value from type annotation
            else
            {
                ThrowCompilerError(
                    ErrorType::VariableDecleration, ErrorCode::CannotDetermine,
                    "Cannot determine  the type of variable! Please provide a type or initializer.",
                    Range(keyword.GetStart(), identifier.GetEnd()));
            }

            if (gen.GetInsertPoint()->GetType() == SymbolNodeType::ModuleNode) // In top level scope
            {
                if (keyword.type == TokenType::Const)
                {
                    if (llvm::isa<llvm::Constant>(init->value))
                    {
                        auto global = new llvm::GlobalVariable(gen.GetModule(), type->type, true, gen.IsUsed(CodeGeneration::Using::Export) ? llvm::GlobalValue::LinkageTypes::ExternalLinkage : llvm::GlobalValue::LinkageTypes::PrivateLinkage, static_cast<llvm::Constant *>(init->value), gen.GenerateMangledName(identifier.raw));
                        const llvm::DataLayout &DL = gen.GetModule().getDataLayout();
                        llvm::Align AllocaAlign = DL.getPrefTypeAlign(type->type);
                        global->setAlignment(AllocaAlign);

                        auto varValue = std::make_shared<CodeValue>(global, type);
                        gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue);
                    }
                    else
                    {
                        ThrowCompilerError(
                            ErrorType::VariableDecleration, ErrorCode::Const,
                            "The initializer of a const global varible must be constant!",
                            Range(keyword.GetStart(), initializer->GetEnd()));
                    }
                }
                else if (keyword.type == TokenType::Let)
                {
                    if (llvm::isa<llvm::Constant>(init->value))
                    {
                        auto global = new llvm::GlobalVariable(gen.GetModule(), type->type, false, gen.IsUsed(CodeGeneration::Using::Export) ? llvm::GlobalValue::LinkageTypes::ExternalLinkage : llvm::GlobalValue::LinkageTypes::PrivateLinkage, static_cast<llvm::Constant *>(init->value), gen.GenerateMangledName(identifier.raw));
                        const llvm::DataLayout &DL = gen.GetModule().getDataLayout();
                        llvm::Align AllocaAlign = DL.getPrefTypeAlign(type->type);
                        global->setAlignment(AllocaAlign);

                        auto varValue = std::make_shared<CodeValue>(global, type);
                        gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue);
                    }
                    else
                    {
                        ThrowCompilerError(
                            ErrorType::VariableDecleration, ErrorCode::Const,
                            "The initializer of a global varible must be constant!",
                            Range(keyword.GetStart(), initializer->GetEnd()));
                    }
                }
            }
            else if (gen.GetInsertPoint()->GetType() == SymbolNodeType::FunctionNode) // Local variables
            {
                if (keyword.type == TokenType::Const) // Constant variable
                {
                    if (init)
                    {

                        std::shared_ptr<CodeValue> varValue;
                        if (type) // If type is specified cast the value to the type
                        {
                            gen.SetCurrentRange(Range(this->type->GetStart(), initializer->GetEnd()));
                            auto casted = gen.Cast(init, type);
                            varValue = std::make_shared<CodeValue>(casted->value, type);
                        }
                        else
                            varValue = std::make_shared<CodeValue>(init->value, type);

                        gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                        return varValue;
                    }
                    else
                    {
                        ThrowCompilerError(
                            ErrorType::VariableDecleration, ErrorCode::Const,
                            "Const variable must have an initializer!",
                            Range(keyword.GetStart(), GetEnd()));
                    }
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw); // Allocate the variable on the stack

                    if (initializer)
                    {

                        if (this->type)
                        {
                            gen.SetCurrentRange(Range(this->type->GetStart(), initializer->GetEnd()));
                            auto casted = gen.Cast(init, type);
                            gen.GetBuilder().CreateStore(casted->value, inst);
                        }
                        else
                            gen.GetBuilder().CreateStore(init->value, inst);
                    }

                    auto varValue = std::make_shared<CodeValue>(inst, type);
                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
            }
            else if (gen.GetInsertPoint()->GetType() == SymbolNodeType::TemplateNode) // Template members
            {
                auto &tn = gen.GetInsertPoint()->As<TemplateNode>();
                if (!tn.IsGeneric())
                    tn.AddMember(type->type, identifier.raw);

                auto val = std::make_shared<CodeValue>(nullptr, type);
                gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, val);
                return val;
            }
        }

        return nullptr;
    }

    std::shared_ptr<CodeValue> BinaryExpression::CodeGen(CodeGeneration &gen) const
    {
        if (op.type == TokenType::Dot)
            return gen.FollowDotChain(*this);

        if (op.type == TokenType::Equal)
            gen.Use(CodeGeneration::Using::Reference);

        auto left = LHS->CodeGen(gen);

        if (op.type == TokenType::Equal)
            gen.UnUse(CodeGeneration::Using::Reference);

        auto right = RHS->CodeGen(gen);
        if (left && right && (left->type->type == right->type->type || (left->type->type->getTypeID() == llvm::Type::IntegerTyID && right->type->type->getTypeID() == llvm::Type::IntegerTyID)))
        {
            auto ret = std::make_shared<CodeValue>(nullptr, std::make_shared<CodeType>(*left->type));
            switch (op.type)
            {
            case TokenType::Plus:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFAdd(left->value, right->value);
                    break;
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateAdd(left->value, right->value);
                    break;

                default:
                    break;
                }
                break;
            }
            case TokenType::Minus:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFSub(left->value, right->value);
                    break;
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateSub(left->value, right->value);
                    break;

                default:
                    break;
                }
                break;
            }
            case TokenType::Star:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFMul(left->value, right->value);
                    break;
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateMul(left->value, right->value);
                    break;

                default:
                    break;
                }
                break;
            }
            case TokenType::ForwardSlash:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFDiv(left->value, right->value);
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateSDiv(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateUDiv(left->value, right->value);
                    break;

                default:
                    break;
                }
                break;
            }
            case TokenType::Pipe:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateOr(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::Ampersand:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateAnd(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::Carrot:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateXor(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            // case TokenType::Or:
            // {
            //     switch (left->type->type->getTypeID())
            //     {
            //     case llvm::Type::IntegerTyID:
            //         ret->value = gen.GetBuilder().Create(left->value, right->value);
            //         break;
            //     case llvm::Type::StructTyID:
            //         //  TODO operator overloading
            //         break;
            //     default:
            //         break;
            //     }
            //     break;
            // }
            // case TokenType::And:
            // {
            //     switch (left->type->type->getTypeID())
            //     {
            //     case llvm::Type::IntegerTyID:
            //         ret->value = gen.GetBuilder().CreateLogicalAnd(left->value, right->value);
            //         break;
            //     case llvm::Type::StructTyID:
            //         //  TODO operator overloading
            //         break;
            //     default:
            //         break;
            //     }
            // }
            case TokenType::DoubleEqual:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOEQ(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateICmpEQ(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::NotEqual:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpUNE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateICmpNE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::LeftAngle:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOLT(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateICmpSLT(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateICmpULT(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::SmallerEqual:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOLE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateICmpSLE(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateICmpULE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::RightAngle:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOGT(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateICmpSGT(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateICmpUGT(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::BiggerEqual:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOGE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateICmpSGE(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateICmpUGE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::NotBigger:

            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOLE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateICmpSLE(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateICmpULE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::NotSmaller:

            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::FloatTyID:
                case llvm::Type::DoubleTyID:
                    ret->value = gen.GetBuilder().CreateFCmpOGE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateICmpSGE(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateICmpUGE(left->value, right->value);
                    ret->type->type = ret->value->getType();
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::LeftShift:

            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateShl(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::RightShift:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    if (left->type->isSigned)
                        ret->value = gen.GetBuilder().CreateAShr(left->value, right->value);
                    else
                        ret->value = gen.GetBuilder().CreateLShr(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::Equal:
            {
                gen.SetCurrentRange(Range(LHS->GetStart(), RHS->GetEnd()));
                auto val = gen.Cast(right, left->type);
                ret->value = gen.GetBuilder().CreateStore(val->value, left->value);
                break;
            }

            // case TokenType::TripleRightShift:
            // {
            //     switch (left->type->type->getTypeID())
            //     {
            //     case llvm::Type::IntegerTyID:
            //         ret->value = gen.GetBuilder().CreateROR(left->value, right->value);
            //         break;
            //     case llvm::Type::StructTyID:
            //         //  TODO operator overloading
            //         break;
            //     default:
            //         break;
            //     }
            //     break;
            // }
            // case TokenType::TripleLeftShift:
            // {
            //     switch (left->type->type->getTypeID())
            //     {
            //     case llvm::Type::IntegerTyID:
            //         ret->value = gen.GetBuilder().CreateROL(left->value, right->value);
            //         break;
            //     case llvm::Type::StructTyID:
            //         //  TODO operator overloading
            //         break;
            //     default:
            //         break;
            //     }
            //     break;
            // }
            default:
                break;
            }
            return ret;
        }
        else
        {
            switch (op.type)
            {
            case TokenType::Dot:
            {
                std::cout << "Dot" << std::endl;
                break;
            }

            default:
                break;
            }
        }
        return nullptr;
    }

    std::shared_ptr<CodeValue> UnaryExpression::CodeGen(CodeGeneration &gen) const
    {
        if (op.type == TokenType::Ampersand)
            gen.Use(CodeGeneration::Using::Reference);
        auto expr = expression->CodeGen(gen);
        if (op.type == TokenType::Ampersand)
            gen.UnUse(CodeGeneration::Using::Reference);
        auto ret = std::make_shared<CodeValue>(nullptr, std::make_shared<CodeType>(*expr->type));
        switch (op.type)
        {
        case TokenType::Minus:
        {
            switch (expr->type->type->getTypeID())
            {
            case llvm::Type::FloatTyID:
            case llvm::Type::DoubleTyID:
                ret->value = gen.GetBuilder().CreateFNeg(expr->value);
                break;
            case llvm::Type::IntegerTyID:
                ret->value = gen.GetBuilder().CreateNeg(expr->value);
                break;

            default:
                // TODO throw error
                break;
            }
            break;
        }
        case TokenType::DoublePlus:
        {
            switch (expr->type->type->getTypeID())
            {
            case llvm::Type::FloatTyID:
            case llvm::Type::DoubleTyID:
                ret->value = gen.GetBuilder().CreateFAdd(expr->value, llvm::ConstantFP::get(expr->type->type, 1.0));
                break;
            case llvm::Type::IntegerTyID:
                ret->value = gen.GetBuilder().CreateAdd(expr->value, llvm::ConstantInt::get(expr->type->type, 1));
                break;

            default:
                // TODO throw error
                break;
            }
            break;
        }
        case TokenType::DoubleMinus:
        {
            switch (expr->type->type->getTypeID())
            {
            case llvm::Type::FloatTyID:
            case llvm::Type::DoubleTyID:
                ret->value = gen.GetBuilder().CreateFAdd(expr->value, llvm::ConstantFP::get(expr->type->type, -1.0));
                break;
            case llvm::Type::IntegerTyID:
                ret->value = gen.GetBuilder().CreateAdd(expr->value, llvm::ConstantInt::get(expr->type->type, -1));
                break;

            default:
                // TODO throw error
                break;
            }
            break;
        }
        case TokenType::Not:
        {
            switch (expr->type->type->getTypeID())
            {
            case llvm::Type::IntegerTyID:
                if (expr->type->type->getIntegerBitWidth() == 1)
                {
                    ret->value = gen.GetBuilder().CreateNeg(expr->value);
                    break;
                }

            default:
                // TODO throw error
                break;
            }
            break;
        }
        case TokenType::Tilda:
        {
            switch (expr->type->type->getTypeID())
            {
            case llvm::Type::IntegerTyID:
                ret->value = gen.GetBuilder().CreateNeg(expr->value);
                break;

            default:
                // TODO throw error
                break;
            }
            break;
        }
        case TokenType::Ampersand:
        {

            if (expr->value->getValueID() == (uint32_t)llvm::Instruction::Alloca + (uint32_t)llvm::Value::InstructionVal || expr->value->getValueID() == (uint32_t)llvm::Instruction::GetElementPtr + (uint32_t)llvm::Value::InstructionVal)
            {
                // llvm::Instruction::AddrSpaceCast
                // expr->value->getType()->getPointerTo->print(llvm::outs());
                ret->value = expr->value;
                ret->type->type = expr->type->type->getPointerTo();
            }
            // switch (expr->type->type->getTypeID())
            // {

            // default:
            //     // TODO throw error
            //     break;
            // }
            break;
        }

        default:
            break;
        }
        return ret;
    }

    std::shared_ptr<CodeValue> PostfixExpression::CodeGen(CodeGeneration &gen) const
    {
        return nullptr;
    }

    std::shared_ptr<CodeValue> IdentifierExpression::CodeGen(CodeGeneration &gen) const
    {
        const auto found = gen.FindSymbolInScope(identifierToken.raw);
        if (found != nullptr)
        {
            switch (found->GetType())
            {
            case SymbolNodeType::VariableNode:
            {
                auto f = found->As<VariableNode>().GetVariable();
                if (gen.IsUsed(CodeGeneration::Using::Reference))
                {
                    return f;
                }
                else
                {
                    auto load = gen.GetBuilder().CreateLoad(f->type->type, f->value);
                    auto value = std::make_shared<CodeValue>(load, f->type);
                    value->type->type = load->getType();
                    return value;
                }
            }
            case SymbolNodeType::FunctionNode:
            {
                auto f = found->As<FunctionNode>().GetFunction();
                // f->type->type = f->value->getType();
                return f;
            }
            case SymbolNodeType::TemplateNode:
            {
                auto t = found->As<TemplateNode>().GetTemplate();
                // f->type->type = f->value->getType();
                return std::make_shared<CodeValue>(nullptr, t);
            }
            default:
                break;
            }
        }
        ThrowCompilerError(
            ErrorType::IdentifierExpression, ErrorCode::CannotFind,
            "Symbol " + identifierToken.raw + " was not found in scope!",
            Range(identifierToken.GetStart(), identifierToken.GetEnd()));
        return nullptr;
    }

    std::shared_ptr<CodeValue> CallExpression::CodeGen(CodeGeneration &gen) const
    {
        auto fnExpr = std::static_pointer_cast<FunctionCodeValue>(fn->CodeGen(gen));
        if (fnExpr)
        {
            size_t argSize = arguments.size();
            bool needsThis = gen.GetDotExprBase() != nullptr && fnExpr->GetFunctionType()->IsMember();
            if (needsThis)
                argSize++;

            if (fnExpr->Function()->arg_size() != argSize)
            {
                ThrowCompilerError(
                    ErrorType::FunctionCall, ErrorCode::ArgMisMatch,
                    "Function was expecting " + std::to_string(fnExpr->Function()->arg_size()) + " but " + std::to_string(arguments.size()) + "were found!",
                    Range(GetStart(), GetEnd()));
                return nullptr;
            }

            std::vector<llvm::Value *> args;

            if (needsThis)
                args.push_back(gen.GetDotExprBase()->value);

            auto checkargs = fnExpr->GetFunctionType()->GetParameters().begin();

            if (needsThis)
                checkargs++;

            for (auto v : arguments)
            {
                auto val = v->CodeGen(gen);
                gen.SetCurrentRange(Range(v->GetStart(), v->GetEnd()));
                auto casted = gen.Cast(val, *checkargs);

                args.push_back(casted->value);
                checkargs++;
            }
            auto call = gen.GetBuilder().CreateCall(fnExpr->Function(), args);

            auto funcType = std::static_pointer_cast<FunctionCodeType>(fnExpr->type);

            if (auto templ = std::dynamic_pointer_cast<TemplateCodeType>(funcType->GetReturnType()))
                return std::make_shared<CodeValue>(call, templ);
            else
                return std::make_shared<CodeValue>(call, fnExpr->type);
        }
        ThrowCompilerError(
            ErrorType::FunctionCall, ErrorCode::NonFunction,
            "Called value isn't a function!",
            Range(fn->GetStart(), fn->GetEnd()));
        return nullptr;
    }

    std::shared_ptr<CodeValue> SubscriptExpression::CodeGen(CodeGeneration &gen) const
    {
        bool isReference = gen.IsUsed(CodeGeneration::Using::Reference);
        gen.Use(CodeGeneration::Using::Reference);

        auto expression = expr->CodeGen(gen);

        if (!isReference)
            gen.UnUse(CodeGeneration::Using::Reference);

        auto subscript = subsr->CodeGen(gen);

        if (subscript->type->type->isIntegerTy())
        {
            std::vector<llvm::Value *> idxs = {llvm::ConstantInt::get(llvm::IntegerType::get(gen.GetContext(), 64), 0), subscript->value};
            auto gep = gen.GetBuilder().CreateInBoundsGEP(expression->value, idxs);
            // auto gep = gen.GetBuilder().CreateConstInBoundsGEP2_32(nullptr, expression->value, subscript->value);
            if (isReference)
                return std::make_shared<CodeValue>(gep, std::dynamic_pointer_cast<ArrayCodeType>(expression->type)->GetBaseType());
            else
                return std::make_shared<CodeValue>(gen.GetBuilder().CreateLoad(expression->type->type, gep), std::dynamic_pointer_cast<ArrayCodeType>(expression->type)->GetBaseType());
        }
        else
        {
            ThrowCompilerError(
                ErrorType::Subscript, ErrorCode::NotIntegral,
                "Subscript expression does not contain integral type!",
                Range(subsr->GetStart(), subsr->GetEnd()));
        }
    }

    std::shared_ptr<CodeValue> CastExpression::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.TypeType(*this->type);
        auto expr = this->LHS->CodeGen(gen);
        return gen.Cast(expr, type, false);
    }

    std::shared_ptr<CodeValue> ArrayLiteral::CodeGen(CodeGeneration &gen) const
    {
        // auto type = static_cast<llvm::ArrayType *>(gen.LiteralType(*this)->type);
        llvm::Value *strc = nullptr;
        std::shared_ptr<CodeType> type;
        if (gen.GetCurrentVar())
        {
            strc = gen.GetCurrentVar()->value;
            type = gen.GetCurrentVar()->type;
        }
        else
        {
            type = gen.LiteralType(*this);
            strc = gen.CreateEntryBlockAlloca(type->type, "");
        }

        if (auto arrType = std::dynamic_pointer_cast<ArrayCodeType>(type))
        {
            int index = 0;

            auto tempCurr = gen.GetCurrentVar();
            for (auto v : values)
            {
                switch (v->GetType())
                {
                case SyntaxType::ArrayLiteralExpressionEntry:
                {
                    auto expr = v->As<ArrayLiteralExpressionEntry>().GetExpression().CodeGen(gen);
                    auto gep = gen.GetBuilder().CreateStructGEP(type->type, strc, index++);
                    auto val = gen.Cast(v->CodeGen(gen), arrType->GetBaseType());
                    gen.GetBuilder().CreateStore(val->value, gep);
                    break;
                }
                case SyntaxType::ArrayLiteralBoundaryEntry:
                {
                    auto &bound = v->As<ArrayLiteralBoundaryEntry>();
                    auto expr = bound.GetExpression().CodeGen(gen);
                    auto size = bound.GetBoundary().CodeGen(gen);

                    for (int i = 0; i < bound.GetLength(); i++)
                    {
                        auto gep = gen.GetBuilder().CreateStructGEP(type->type, strc, index++);
                        auto val = gen.Cast(v->CodeGen(gen), arrType->GetBaseType());
                        gen.GetBuilder().CreateStore(val->value, gep);
                    }
                    break;
                }
                default:
                    break;
                }
            }
            gen.SetCurrentVar(tempCurr);

            if (!gen.GetCurrentVar())
                return std::make_shared<CodeValue>(gen.GetBuilder().CreateLoad(type->type, strc), type);
        }
        // std::vector<const llvm::Value *> vals;

        return nullptr;
    }

    std::shared_ptr<CodeValue> StringSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        std::vector<llvm::Constant *> vals;
        for (auto c : GetValue())
        {
            vals.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(gen.GetContext()), c));
        }
        vals.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(gen.GetContext()), 0));
        return std::make_shared<CodeValue>(llvm::ConstantArray::get(static_cast<llvm::ArrayType *>(type->type), llvm::makeArrayRef(vals)), type);
    }

    // std::shared_ptr<CodeValue> CharSyntax::CodeGen(CodeGeneration &gen) const
    // {
    //     auto type = gen.LiteralType(*this);
    //     return std::make_shared<CodeValue>(llvm::ConstantArray::get(static_cast<llvm::ArrayType *>(type->type), llvm::makeArrayRef(vals)), type);
    // }

    std::shared_ptr<CodeValue> BooleanSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        return std::make_shared<CodeValue>(llvm::ConstantInt::get(type->type, llvm::APInt(1, GetValue())), type);
    }

    std::shared_ptr<CodeValue> FloatingSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        return std::make_shared<CodeValue>(llvm::ConstantFP::get(type->type, llvm::APFloat(GetValue())), type);
    }

    std::shared_ptr<CodeValue> IntegerSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        return std::make_shared<CodeValue>(llvm::ConstantInt::get(type->type, llvm::APInt(CodeGeneration::GetNumBits(GetValue()), GetValue())), type);
    }

    std::shared_ptr<CodeValue> FunctionDeclerationStatement::CodeGen(CodeGeneration &gen) const
    {
        auto checkFunc = gen.GetModule().getFunction(gen.GenerateMangledName(identifier.raw));
        auto found = gen.GetInsertPoint()->findSymbol(identifier.raw);

        if (!checkFunc->empty())
        {
            ThrowCompilerError(
                ErrorType::FunctionDecleration, ErrorCode::AlreadyFound,
                "Function or type with name " + identifier.raw + " was already found in scope!",
                Range(GetStart(), GetEnd()));
        }

        if (found == gen.GetInsertPoint()->children.end() && found->second->GetType() != SymbolNodeType::FunctionNode)
            return nullptr;

        auto func = found->second->As<FunctionNode>();
        auto funcVal = func.GetFunction();

        auto retBlock = llvm::BasicBlock::Create(gen.GetContext());
        funcVal->SetReturnLabel(retBlock);

        gen.SetCurrentFunction(funcVal);

        auto lastScope = gen.GetInsertPoint();
        gen.SetInsertPoint(found->second);
        if (gen.IsUsed(CodeGeneration::Using::Export))
            func.Export();

        auto insertPoint = llvm::BasicBlock::Create(gen.GetContext(), "entry", checkFunc);
        gen.GetBuilder().SetInsertPoint(insertPoint);

        int index = 0;
        for (auto &arg : checkFunc->args())
        {
            auto inst = gen.CreateEntryBlockAlloca(arg.getType(), this->parameters[index]->GetIdentifier().raw); // Allocate the variable on the stack
            gen.GetBuilder().CreateStore(&arg, inst);

            auto varValue = std::make_shared<CodeValue>(inst, funcVal->GetFunctionType()->GetParameters()[index]);

            gen.GetInsertPoint()->AddChild<VariableNode>(this->parameters[index++]->GetIdentifier().raw, varValue);
        }

        if (retType)
        {
            auto retval = gen.CreateEntryBlockAlloca(checkFunc->getFunctionType()->getReturnType(), "ret");
            funcVal->SetReturnLocation(retval);
        }

        gen.Use(CodeGeneration::Using::NoBlock);
        std::shared_ptr<CodeValue> funcBody = body->CodeGen(gen);

        if (retType)
        {
            if ((body->GetType() == SyntaxType::BlockStatement &&
                 body->As<BlockStatement<>>().GetStatements().size() > 0 &&
                 body->As<BlockStatement<>>().GetStatements().back()->GetType() == SyntaxType::ExpressionStatement) ||
                body->GetType() == SyntaxType::ExpressionStatement)
            {
                auto casted = gen.Cast(funcBody, funcVal->type);
                gen.Return(casted->value);
            }

            auto cfv = gen.GetCurrentFunction();
            if (cfv->GetNumRets() == 1)
            {
                cfv->lastStore->eraseFromParent();
                auto p = cfv->lastbr->getParent();

                cfv->lastbr->eraseFromParent();

                cfv->GetReturnLocation()->eraseFromParent();
                // gen.GetBuilder().CreateRet(llvm::Constant::getNullValue(fValue->Function()->getFunctionType()->getReturnType()));
                if (p->getValueID() == llvm::Value::BasicBlockVal)
                    gen.GetBuilder().SetInsertPoint(p);
                gen.GetBuilder().CreateRet(cfv->lastStoreValue);
            }
            else if (cfv->GetNumRets() >= 1)
            {
                if (gen.GetBuilder().GetInsertBlock()->getTerminator() == nullptr)
                    gen.GetBuilder().CreateBr(retBlock);
                checkFunc->getBasicBlockList().push_back(retBlock);
                gen.GetBuilder().SetInsertPoint(retBlock);
                auto load = gen.GetBuilder().CreateLoad(cfv->type->type, cfv->GetReturnLocation());
                gen.GetBuilder().CreateRet(load);
            }
            else
            {
                ThrowCompilerError(
                    ErrorType::FunctionDecleration, ErrorCode::NoReturn,
                    "Function with return type doesn't return a value!",
                    Range(retType->GetStart(), retType->GetEnd()));
                cfv->GetReturnLocation()->eraseFromParent();
                gen.GetBuilder().CreateRet(llvm::Constant::getNullValue(funcVal->Function()->getFunctionType()->getReturnType()));
                return nullptr;
            }
        }
        else
            gen.GetBuilder().CreateRetVoid();

        if (llvm::verifyFunction(*checkFunc, &llvm::errs()))
            return nullptr;

        // gen.LastScope();
        gen.SetInsertPoint(lastScope);
        return funcVal;

        checkFunc->eraseFromParent();
        return nullptr;
    }

    void FunctionDeclerationStatement::PreCodeGen(CodeGeneration &gen) const
    {
        switch (gen.GetPreCodeGenPass())
        {
        case 20:
            if (gen.GetInsertPoint()->GetType() != SymbolNodeType::TemplateNode)
                break;
        case 30:
        {
            std::shared_ptr<CodeType> funcReturnType;
            std::vector<std::shared_ptr<CodeType>> funcParameters;
            bool member = false;

            funcReturnType = (retType == nullptr ? std::make_shared<CodeType>(llvm::Type::getVoidTy(gen.GetContext())) : gen.TypeType(*retType));
            std::vector<llvm::Type *> parameters;

            if (this->parameters.size() > 0 && this->parameters[0]->GetVariableType() == nullptr)
            {
                auto type = std::make_shared<TemplateCodeType>(*std::static_pointer_cast<TemplateCodeType>(gen.GetCurrentType()));
                type->type = type->type->getPointerTo();
                parameters.push_back(type->type);
                funcParameters.push_back(type);
                member = true;
            }

            for (auto p : this->parameters)
                if (p->GetVariableType())
                {
                    auto type = gen.TypeType(*p->GetVariableType());
                    parameters.push_back(type->type);
                    funcParameters.push_back(type);
                }
            auto functionType = llvm::FunctionType::get(funcReturnType->type, parameters, false);
            auto checkFunc = llvm::Function::Create(functionType, gen.IsUsed(CodeGeneration::Using::Export) ? llvm::Function::ExternalLinkage : llvm::Function::PrivateLinkage, 0, gen.GenerateMangledName(identifier.raw), &(gen.GetModule()));

            auto fValue = std::make_shared<FunctionCodeValue>(checkFunc, std::make_shared<FunctionCodeType>(funcReturnType, funcParameters, member));

            auto fnScope = gen.NewScope<FunctionNode>(identifier.raw, fValue);
            if (gen.IsUsed(CodeGeneration::Using::Export))
                fnScope->Export();

            gen.LastScope();
            break;
        }
        }
    }

    std::shared_ptr<CodeValue> ExportDecleration::CodeGen(CodeGeneration &gen) const
    {
        gen.Use(CodeGeneration::Using::Export);
        auto stGen = statement->CodeGen(gen);
        gen.UnUse(CodeGeneration::Using::Export);
        return stGen;
    }

    std::shared_ptr<CodeValue> ReturnStatement::CodeGen(CodeGeneration &gen) const
    {
        auto retType = static_cast<llvm::Function *>(gen.GetCurrentFunction()->value)->getFunctionType()->getReturnType();
        if (retType->getTypeID() == llvm::Type::VoidTyID && expression != nullptr)
        {
            ThrowCompilerError(
                ErrorType::Return, ErrorCode::NoReturn,
                "Function with return value in a function without a return type!",
                Range(GetStart(), GetEnd()));
        }
        else if (expression)
        {
            auto cgen = expression->CodeGen(gen);
            auto hretType = std::make_shared<CodeType>(retType);

            gen.SetCurrentRange(Range(expression->GetStart(), expression->GetEnd()));
            auto expr = gen.Cast(cgen, hretType);

            gen.Return(expr->value);
            return nullptr;
        }
        else
        {
            ThrowCompilerError(
                ErrorType::Return, ErrorCode::NoReturn,
                "Return statement expecting value in function with return type!",
                Range(GetStart(), GetEnd()));
            if (retType->isVoidTy())
                gen.Return();
            else
                gen.Return(llvm::Constant::getNullValue(retType));

            return nullptr;
        }
    }

    std::shared_ptr<CodeValue> IfStatement::CodeGen(CodeGeneration &gen) const
    {
        auto currentBlock = gen.GetBuilder().GetInsertBlock();
        auto endBlock = llvm::BasicBlock::Create(gen.GetContext());
        llvm::BasicBlock *ifBlock;
        if (body->GetType() == SyntaxType::BlockStatement && body->As<BlockStatement<>>().GetStatements().size() == 0)
            ifBlock = endBlock;
        else
        {
            ifBlock = llvm::BasicBlock::Create(gen.GetContext(), "", gen.GetCurrentFunction()->Function());
            gen.GetBuilder().SetInsertPoint(ifBlock);
            body->CodeGen(gen);
            if (ifBlock->getTerminator() == nullptr)
                gen.GetBuilder().CreateBr(endBlock);
            gen.GetBuilder().SetInsertPoint(currentBlock);
        }

        llvm::BasicBlock *elseBlock;
        if (elseClause == nullptr || (elseClause->GetBody().GetType() == SyntaxType::BlockStatement && elseClause->GetBody().As<BlockStatement<>>().GetStatements().size() == 0))
            elseBlock = endBlock;
        else
        {
            elseBlock = llvm::BasicBlock::Create(gen.GetContext(), "", gen.GetCurrentFunction()->Function());
            gen.GetBuilder().SetInsertPoint(elseBlock);
            elseClause->CodeGen(gen);
            if (elseBlock->getTerminator() == nullptr)
                gen.GetBuilder().CreateBr(endBlock);
            gen.GetBuilder().SetInsertPoint(currentBlock);
        }

        auto expr = expression->CodeGen(gen);
        if (llvm::isa<llvm::Constant>(expr->value))
        {
            std::cout << "Const";
        }
        if (expr->type->type->isIntegerTy() && expr->type->type->getIntegerBitWidth() == 1) // Make sure it is boolean type
        {
            gen.GetBuilder().CreateCondBr(expr->value, ifBlock, elseBlock);
            // gen.GetCurrentFunction()->Function()->getBasicBlockList().push_back(ifBlock);
            // if (ifBlock != endBlock)
            //     gen.GetCurrentFunction()->Function()->getBasicBlockList().push_back(elseBlock);

            gen.GetCurrentFunction()->Function()->getBasicBlockList().push_back(endBlock);
            gen.GetBuilder().SetInsertPoint(endBlock);
        }
        else
        {
            ThrowCompilerError(
                ErrorType::IfStatement, ErrorCode::NotBoolean,
                "If statement expression is not a boolean expression",
                Range(expression->GetStart(), expression->GetEnd()));
        }
        return nullptr;
    }

    std::shared_ptr<CodeValue> TemplateStatement::CodeGen(CodeGeneration &gen) const
    {
        return nullptr;
    }

    void TemplateStatement::PreCodeGen(CodeGeneration &gen) const
    {
        switch (gen.GetPreCodeGenPass())
        {
        case 0:
        {
            if (gen.GetInsertPoint()->findSymbol(identifier.raw) != gen.GetInsertPoint()->GetChildren().end())
            {
                ThrowCompilerError(
                    ErrorType::TemplateScope, ErrorCode::AlreadyFound,
                    "Type or function with name " + identifier.raw + " was already found in scope!",
                    Range(identifier.GetStart(), identifier.GetEnd()));
                return;
            }
            if (generic)
            {
                gen.NewScope<TemplateNode>(identifier.raw, generic, body);
                for (auto g : generic->GetParameters())
                {
                    gen.GetInsertPoint()->AddChild<TypeAliasNode>(g->GetIdentifier().raw);
                }
            }
            else
            {
                auto structType = llvm::StructType::create(gen.GetContext(), gen.GenerateMangledTypeName(identifier.raw));
                gen.NewScope<TemplateNode>(identifier.raw, structType);
            }

            gen.LastScope();
            break;
        }
        case 10:
        {
            auto found = gen.GetInsertPoint()->findSymbol(identifier.raw);
            if (found != gen.GetInsertPoint()->children.end())
            {
                gen.SetInsertPoint(found->second);

                auto &fnScope = found->second->As<TemplateNode>();
                if (gen.IsUsed(CodeGeneration::Using::Export))
                    fnScope.Export();

                gen.Use(CodeGeneration::Using::NoBlock);

                body->CodeGen(gen);

                if (!generic)
                    fnScope.GetTemplate()->Template()->setBody(fnScope.GetMembers());

                gen.LastScope();
            }

            break;
        }
        }

        // auto tValue = std::make_shared<TemplateCodeValue>(structType, *fnScope);
    }

    std::shared_ptr<CodeValue> TemplateInitializer::CodeGen(CodeGeneration &gen) const
    {
        // auto type = gen.TypeType(this->GetTemplateType());
        auto type = gen.GetCurrentType();
        if (const auto templ = std::dynamic_pointer_cast<TemplateCodeType>(type))
        {
            auto &node = templ->GetNode();
            auto &children = node.GetChildren();

            auto values = body->GetValues();
            llvm::Value *strc = nullptr;
            if (gen.GetCurrentVar())
                strc = gen.GetCurrentVar()->value;
            else
                strc = gen.CreateEntryBlockAlloca(node.GetTemplate()->type, "");
            auto tempCurr = gen.GetCurrentVar();

            for (auto v : values)
            {
                const auto &key = v->GetKey().raw;
                auto found = node.findSymbol(key);
                if (found != children.end() && found->second->GetType() == SymbolNodeType::VariableNode)
                {
                    // int index = node.IndexOf(found);
                    // int index = std::find(templ->node.GetOrderedMembers().
                    int index = templ->node.IndexOf(key);
                    auto gep = gen.GetBuilder().CreateStructGEP(node.GetTemplate()->type, strc, index);
                    auto newVal = std::make_shared<CodeValue>(gep, found->second->As<VariableNode>().GetVariable()->type);
                    gen.SetCurrentVar(newVal);
                    auto init = v->GetValue().CodeGen(gen);
                    if (init)
                    {
                        gen.SetCurrentRange(Range(v->GetStart(), v->GetEnd()));
                        auto val = gen.Cast(init, dynamic_cast<VariableNode *>(found->second)->GetVariable()->type);
                        gen.GetBuilder().CreateStore(val->value, gep);
                    }
                }
                else
                {
                    auto str = ModuleUnit::GetFptr().StringFromRange(Range(this->type->GetStart(), this->type->GetEnd()));
                    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
                    ThrowCompilerError(
                        ErrorType::TemplateInitializer, ErrorCode::CannotFind,
                        "Cannot find variable " + key + " in type " + str + "!",
                        Range(v->GetKey().GetStart(), v->GetValue().GetEnd()));
                }
            }
            gen.SetCurrentVar(tempCurr);
            if (!gen.GetCurrentVar())
            {
                return std::make_shared<CodeValue>(gen.GetBuilder().CreateLoad(node.GetTemplate()->type, strc), node.GetTemplate());
            }
            else
                return nullptr;
        }
        return nullptr;
    }

    std::shared_ptr<CodeValue> ObjectInitializer::CodeGen(CodeGeneration &gen) const
    {
        // if (gen.GetCurrentVar())
        // {
        if (auto templ = std::static_pointer_cast<TemplateCodeType>(gen.GetCurrentVar()->type))
        {
            auto tmpVal = gen.GetCurrentVar();
            auto &node = templ->GetNode();
            for (auto v : values)
            {
                const auto &key = v->GetKey().raw;

                auto found = node.findSymbol(key);
                if (found != node.children.end() && found->second->GetType() == SymbolNodeType::VariableNode)
                {
                    int index = node.IndexOf(key);
                    auto gep = gen.GetBuilder().CreateStructGEP(templ->type, tmpVal->value, index);
                    // std::make_shared<CodeType>(gep->getType())
                    auto newVal = std::make_shared<CodeValue>(gep, found->second->As<VariableNode>().GetVariable()->type);

                    gen.SetCurrentVar(newVal);

                    auto init = v->GetValue().CodeGen(gen);
                    if (init)
                    {
                        gen.SetCurrentRange(Range(v->GetStart(), v->GetEnd()));
                        auto val = gen.Cast(init, dynamic_cast<VariableNode *>(found->second)->GetVariable()->type);
                        gen.GetBuilder().CreateStore(val->value, gep);
                    }

                    gen.SetCurrentVar(tmpVal);
                }
                else
                {
                    ThrowCompilerError(
                        ErrorType::TemplateInitializer, ErrorCode::CannotFind,
                        "Cannot find variable " + key + " in type " + gen.FindSymbolInScope(&templ->GetNode()) + "!",
                        Range(v->GetKey().GetStart(), v->GetValue().GetEnd()));
                }
            }
        }
        // }

        return nullptr;
    }

    std::shared_ptr<CodeValue> ActionBaseStatement::CodeGen(CodeGeneration &gen) const
    {
        // auto type = gen.TypeType(*templateType);
        // TypeSyntax *t = templateType;

        // while (t->GetType() == SyntaxType::GenericType)
        //     t = t->As<GenericType>().GetBaseType();

        // auto foundA = gen.GetInsertPoint()->findSymbol()

        // if (type)
        // {
        //     // std::string str = templateType->GetType() == SyntaxType::IdentifierType ? templateType->As<IdentifierType>().GetToken().raw : templateType->As<GenericType>().
        //     auto found = gen.FindSymbolInScope(t->As<IdentifierType>().GetToken().raw);

        //     if (found && found->GetType() == SymbolNodeType::TemplateNode)
        //     {
        //         auto actnScope = gen.NewScope<ActionNode>(gen.GetInsertPoint()->GenerateName(), t->As<IdentifierType>().GetToken().raw);
        //         found->As<TemplateNode>().AddAction(actnScope);
        //         gen.SetCurrentType(found->As<TemplateNode>().GetTemplate());

        //         gen.Use(CodeGeneration::Using::NoBlock);

        //         body->CodeGen(gen);

        //         gen.LastScope();
        //     }
        // }
        auto type = gen.TypeType(*templateType);
        TypeSyntax *t = templateType;

        while (t->GetType() == SyntaxType::GenericType)
            t = t->As<GenericType>().GetBaseType();

        auto lastScope = gen.GetInsertPoint();

        if (type)
        {
            auto found = gen.FindSymbolInScope(t->As<IdentifierType>().GetToken().raw);

            if (found && found->GetType() == SymbolNodeType::TemplateNode)
            {
                gen.SetInsertPoint(found);

                gen.SetCurrentType(found->As<TemplateNode>().GetTemplate());

                gen.Use(CodeGeneration::Using::NoBlock);
                body->CodeGen(gen);

                gen.SetInsertPoint(lastScope);
            }
            // gen.NewScope<ActionNode>(gen.GetInsertPoint()->GenerateName(), t->As<IdentifierType>().GetToken().raw);
        }
        return nullptr;
    }

    void ActionBaseStatement::PreCodeGen(CodeGeneration &gen) const
    {
        switch (gen.GetPreCodeGenPass())
        {
        case 20:
        {
            auto type = gen.TypeType(*templateType);
            auto lastScope = gen.GetInsertPoint();

            if (type)
            {
                auto &node = std::static_pointer_cast<TemplateCodeType>(type)->GetNode();
                // auto found = gen.FindSymbolInScope(t->As<IdentifierType>().GetToken().raw);

                // if (found && found->GetType() == SymbolNodeType::TemplateNode)
                // {
                gen.SetInsertPoint(&node);

                gen.SetCurrentType(node.GetTemplate());

                gen.Use(CodeGeneration::Using::NoBlock);
                body->PreCodeGen(gen);

                gen.SetInsertPoint(lastScope);
                // }
                // gen.NewScope<ActionNode>(gen.GetInsertPoint()->GenerateName(), t->As<IdentifierType>().GetToken().raw);
            }
            break;
        }
        }
    }

    std::shared_ptr<CodeValue> ActionSpecStatement::CodeGen(CodeGeneration &gen) const
    {

        auto type = gen.TypeType(*templateType);

        auto specType = gen.TypeType(*this->specType);
        auto lastScope = gen.GetInsertPoint();

        if (type && specType)
        {
            auto &node = std::static_pointer_cast<TemplateCodeType>(type)->GetNode();
            // auto found = gen.FindSymbolInScope(t->As<IdentifierType>().GetToken().raw);

            gen.SetInsertPoint(&node);

            gen.SetCurrentType(node.GetTemplate());

            gen.Use(CodeGeneration::Using::NoBlock);
            body->CodeGen(gen);

            gen.SetInsertPoint(lastScope);
        }

        return nullptr;
    }

    void ActionSpecStatement::PreCodeGen(CodeGeneration &gen) const
    {
        switch (gen.GetPreCodeGenPass())
        {
        case 30:
        {
            auto type = gen.TypeType(*templateType);
            auto specType = gen.TypeType(*this->specType);

            auto lastScope = gen.GetInsertPoint();

            if (type && specType)
            {
                auto &node = std::static_pointer_cast<TemplateCodeType>(type)->GetNode();
                auto &specNode = std::static_pointer_cast<SpecCodeType>(specType)->GetNode();
                node.Implement(specNode);

                gen.SetInsertPoint(&node);

                gen.SetCurrentType(node.GetTemplate());

                gen.Use(CodeGeneration::Using::NoBlock);
                std::unordered_map<std::string, bool> symbols;
                for (auto c : specNode.children)
                {
                    symbols[c.first] = false;
                }

                for (auto &stmt : body->GetStatements())
                {
                    switch (stmt->GetType())
                    {
                    case SyntaxType::FunctionDeclerationStatement:
                    {
                        auto &func = stmt->As<FunctionDeclerationStatement>();
                        auto found = specNode.findSymbol(func.GetIdentifier().raw);
                        symbols[func.GetIdentifier().raw] = true;

                        if (found != specNode.children.end() && found->second->GetType() != SymbolNodeType::FunctionNode)
                        {
                            ThrowCompilerError(
                                ErrorType::ActionSpecStatement, ErrorCode::NonFunction,
                                "Symbol " + func.GetIdentifier().raw + " is not a function in spec " + gen.FindSymbolInScope(&specNode) + "!",
                                Range(func.GetStart(), func.GetEnd()));
                        }
                        else if (found != specNode.children.end())
                        {
                            std::dynamic_pointer_cast<TemplateCodeType>(type)->GetNode().GetBody()->As<BlockStatement<>>().GetStatements().push_back(&func);
                            auto &foundFunc = found->second->As<FunctionNode>();

                            auto retType = (func.GetRetType() == nullptr ? std::make_shared<CodeType>(llvm::Type::getVoidTy(gen.GetContext())) : gen.TypeType(*func.GetRetType()));
                            bool member = false;
                            std::vector<std::shared_ptr<CodeType>> funcParameters;

                            if (func.GetParameters().size() > 0 && func.GetParameters()[0]->GetVariableType() == nullptr)
                            {
                                // auto type = std::make_shared<TemplateCodeValue>(*static_cast<TemplateCodeValue *>(gen.GetCurrentType()));
                                // type->type = type->type->getPointerTo();
                                funcParameters.push_back(nullptr);
                                member = true;
                            }

                            for (auto p : func.GetParameters())
                                if (p->GetVariableType())
                                {
                                    auto type = gen.TypeType(*p->GetVariableType());
                                    funcParameters.push_back(type);
                                }

                            if (foundFunc.GetFunction()->GetFunctionType()->IsMember() != member)
                            {
                                ThrowCompilerError(
                                    ErrorType::ActionSpecStatement, ErrorCode::ArgMisMatch,
                                    "Function arguments do not match spec function's arguments!",
                                    Range(func.GetStart(), func.GetEnd()));
                                break;
                            }

                            if (foundFunc.GetFunction()->GetFunctionType()->GetParameters().size() != func.GetParameters().size())
                            {
                                ThrowCompilerError(
                                    ErrorType::ActionSpecStatement, ErrorCode::ArgMisMatch,
                                    "Function arguments do not match spec function's arguments!",
                                    Range(func.GetStart(), func.GetEnd()));
                                break;
                            }

                            if (member)
                            {
                                for (size_t i = 1; i < funcParameters.size(); i++)
                                {
                                    auto pType = gen.TypeType(*func.GetParameters()[i]->GetVariableType());
                                    if (*pType != *foundFunc.GetFunction()->GetFunctionType()->GetParameters()[i])
                                    {
                                        ThrowCompilerError(
                                            ErrorType::ActionSpecStatement, ErrorCode::ArgMisMatch,
                                            "Function arguments do not match spec function's arguments!",
                                            Range(func.GetStart(), func.GetEnd()));
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                for (size_t i = 0; i < funcParameters.size(); i++)
                                {
                                    auto pType = gen.TypeType(*func.GetParameters()[i]->GetVariableType());
                                    if (*pType != *foundFunc.GetFunction()->GetFunctionType()->GetParameters()[i])
                                    {
                                        ThrowCompilerError(
                                            ErrorType::ActionSpecStatement, ErrorCode::ArgMisMatch,
                                            "Function arguments do not match spec function's arguments!",
                                            Range(func.GetStart(), func.GetEnd()));
                                        break;
                                    }
                                }
                            }
                            auto ret = std::dynamic_pointer_cast<FunctionCodeType>(foundFunc.GetFunction()->type)->GetReturnType();
                            if (*gen.TypeType(*func.GetRetType()) != *ret)
                            {
                                ThrowCompilerError(
                                    ErrorType::ActionSpecStatement, ErrorCode::ArgMisMatch,
                                    "Function return type does not match spec function's return type!",
                                    Range(func.GetRetType()->GetStart(), func.GetRetType()->GetEnd()));
                            }
                            // gen.GetInsertPoint()->AddChild<FunctionNode>(func.GetIdentifier().raw, new FunctionCodeValue(nullptr, retType, funcParameters, member));
                        }
                        else
                        {
                            ThrowCompilerError(
                                ErrorType::ActionSpecStatement, ErrorCode::CannotFind,
                                "Function " + func.GetIdentifier().raw + " does not exist in spec!",
                                Range(func.GetIdentifier().GetStart(), func.GetIdentifier().GetEnd()));
                        }

                        break;
                    }

                    default:
                        break;
                    }
                }

                for (auto f : symbols)
                {
                    if (!f.second)
                    {
                        auto found = specNode.findSymbol(f.first);
                        if (found != specNode.children.end())
                        {
                            if (found->second->GetType() == SymbolNodeType::FunctionNode)
                            {
                                ThrowCompilerError(
                                    ErrorType::ActionSpecStatement, ErrorCode::CannotFind,
                                    "Function " + f.first + " not implemented in action!",
                                    Range(keyword.position.start, this->specType->GetEnd()));
                            }
                            else if (found->second->GetType() == SymbolNodeType::TypeAliasNode)
                            {
                                ThrowCompilerError(
                                    ErrorType::ActionSpecStatement, ErrorCode::CannotFind,
                                    "Type alias " + f.first + " not implemented in action!",
                                    Range(keyword.position.start, this->specType->GetEnd()));
                            }
                        }
                    }
                }

                body->PreCodeGen(gen);

                gen.SetInsertPoint(lastScope);
                // gen.NewScope<ActionNode>(gen.GetInsertPoint()->GenerateName(), t->As<IdentifierType>().GetToken().raw);
            }
            break;
        }
        }
    }

    std::shared_ptr<CodeValue> LoopStatement::CodeGen(CodeGeneration &gen) const
    {
        if (expression)
        {
            // TODO use range type
        }
        else
        {
            llvm::BasicBlock *startBlock = nullptr;
            // auto startBlock = llvm::BasicBlock::Create(gen.GetContext(), "", gen.GetCurrentFunction()->Function()); // Start of loop {
            auto endBlock = llvm::BasicBlock::Create(gen.GetContext(), "", gen.GetCurrentFunction()->Function()); // End of loop }
            if (gen.GetBuilder().GetInsertBlock()->getTerminator() == nullptr)
            {
                /* If the previous block doesn't have a terminator and has 
                   no instructions then reuse the block as the starting block for the loop */
                if (gen.GetBuilder().GetInsertBlock()->getInstList().size() == 0)
                    startBlock = gen.GetBuilder().GetInsertBlock();
                else
                {
                    startBlock = llvm::BasicBlock::Create(gen.GetContext(), "", gen.GetCurrentFunction()->Function());
                    gen.GetBuilder().CreateBr(startBlock);
                }
            }
            else
                startBlock = llvm::BasicBlock::Create(gen.GetContext(), "", gen.GetCurrentFunction()->Function());

            gen.GetBuilder().SetInsertPoint(startBlock);
            if (body)
                body->CodeGen(gen);
            gen.GetBuilder().CreateBr(startBlock);
            gen.GetBuilder().SetInsertPoint(endBlock);
        }
        return nullptr;
    }

    std::shared_ptr<CodeValue> SpecStatement::CodeGen(CodeGeneration &gen) const
    {
        return nullptr;
    }

    void SpecStatement::PreCodeGen(CodeGeneration &gen) const
    {
        // auto type = gen.TypeType(*templateType);
        // TypeSyntax *t = templateType;

        // while (t->GetType() == SyntaxType::GenericType)
        //     t = t->As<GenericType>().GetBaseType();
        switch (gen.GetPreCodeGenPass())
        {
        case 0:
        {
            auto scope = gen.NewScope<SpecNode>(identifier.raw);
            gen.LastScope();
            break;
        }
        case 20:
        {
            // auto scope = gen.NewScope<SpecNode>(identifier.raw);
            auto found = gen.GetInsertPoint()->findSymbol(identifier.raw);

            if (found != gen.GetInsertPoint()->children.end())
            {
                gen.SetInsertPoint(found->second);
                for (auto &stmt : body->GetStatements())
                {
                    auto &func = stmt->As<FunctionDeclerationStatement>();
                    auto retType = (func.GetRetType() == nullptr ? std::make_shared<CodeType>(llvm::Type::getVoidTy(gen.GetContext())) : gen.TypeType(*func.GetRetType()));
                    bool member = false;
                    std::vector<std::shared_ptr<CodeType>> funcParameters;

                    if (func.GetParameters().size() > 0 && func.GetParameters()[0]->GetVariableType() == nullptr)
                    {
                        // auto type = std::make_shared<TemplateCodeValue>(*static_cast<TemplateCodeValue *>(gen.GetCurrentType()));
                        // type->type = type->type->getPointerTo();
                        funcParameters.push_back(nullptr);
                        member = true;
                    }

                    for (auto p : func.GetParameters())
                        if (p->GetVariableType())
                        {
                            auto type = gen.TypeType(*p->GetVariableType());
                            funcParameters.push_back(type);
                        }

                    gen.GetInsertPoint()->AddChild<FunctionNode>(
                        func.GetIdentifier().raw,
                        std::make_shared<FunctionCodeValue>(
                            nullptr,
                            std::make_shared<FunctionCodeType>(retType, funcParameters, member)));
                }
                gen.LastScope();
            }

            break;
        }
        }
    }

    void TypeAliasStatement::PreCodeGen(CodeGeneration &gen) const
    {
        switch (gen.GetPreCodeGenPass())
        {
        case 0:
            static TypeAliasNode *node = nullptr;
            if (generic)
            {
                node = gen.GetInsertPoint()->AddChild<TypeAliasNode>(identifier.raw, generic, type);
                for (auto g : generic->GetParameters())
                {
                    node->AddChild<TypeAliasNode>(g->GetIdentifier().raw);
                }
            }
            else
                node = gen.GetInsertPoint()->AddChild<TypeAliasNode>(identifier.raw);
            break;
        case 1:
        {

            if (!generic)
            {
                auto lastPoint = gen.GetInsertPoint();
                gen.SetInsertPoint(node);

                node->SetReferencedType(gen.TypeType(*type));

                gen.SetInsertPoint(lastPoint);
            }
        }
        default:
            break;
        }
    }

} // namespace Parsing

void PrintSymbols(const SymbolNode &node, const std::string &name, int index, const std::wstring &indent, bool last)
{
#ifdef PLATFORM_WINDOWS
    _setmode(_fileno(stdout), _O_U16TEXT);
    std::wcout << indent;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
    std::cout << convert.to_bytes(indent);
#endif

    if (index != 0)
    {
#ifdef PLATFORM_WINDOWS
        std::wcout << (last ? L"└── " : L"├── ");
#else
        std::cout << convert.to_bytes((last ? L"└── " : L"├── "));
#endif
    }
#ifdef PLATFORM_WINDOWS
    _setmode(_fileno(stdout), _O_TEXT);
#endif

    std::cout << node;
    if (node.IsExported())
        std::cout << " Exported";
    if (name.size() > 0 && name[0] != '$')
        std::cout << " `" << name << "`" << std::endl;
    else
    {
        std::cout << " new scope" << std::endl;
    }

    std::wstring nindent = indent + (index == 0 ? L"" : (last ? L"    " : L"│   "));
    const auto &children = node.GetChildren();
    int i = 0;
    for (auto &c : children)
    {
        PrintSymbols(*c.second, c.first, index + 1, nindent, i == children.size() - 1);
        i++;
    }
}

std::ostream &operator<<(std::ostream &stream, const SymbolNodeType &e)
{

#define ets(type)              \
    case SymbolNodeType::type: \
        stream << #type;       \
        break;
    switch (e)
    {
        ets(SymbolNode);
        ets(PackageNode);
        ets(ModuleNode);
        ets(FunctionNode);
        ets(TemplateNode);
        ets(VariableNode);
        ets(TypeAliasNode);
        ets(ScopeNode);
        ets(ActionNode);
        ets(SpecNode);
    }
#undef ets
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const SymbolNode &node)
{
    stream << node.GetType();
    try
    {
        switch (node.GetType())
        {
            // case SymbolNodeType::SymnolNode:
            // {
            //     stream << " " << node.As<Parsing::BinaryExpression>().GetOperator().type;
            //     break;
            // }

        default:
            break;
        }
    }
    catch (const std::bad_cast &e)
    {
        std::cerr << "Bad cast " << e.what() << std::endl;
    }
    return stream;
}
