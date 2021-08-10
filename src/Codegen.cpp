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

// std::string gen_random(const int len)
// {
//     std::string tmp_s;
//     static const char alphanum[] =
//         "0123456789"
//         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//         "abcdefghijklmnopqrstuvwxyz";

//     srand((unsigned)time(NULL));

//     tmp_s.reserve(len);

//     for (int i = 0; i < len; ++i)
//         tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

//     return tmp_s;
// }

using namespace Parsing;

SymbolNode CodeGeneration::rootSymbols(nullptr);

CodeType *CodeGeneration::LiteralType(const SyntaxNode &node)
{
    switch (node.GetType())
    {
    case SyntaxType::Integer:
    {
        auto value = node.As<IntegerSyntax>().GetValue();
        return new CodeType(llvm::IntegerType::get(context, GetNumBits(value)));
    }
    case SyntaxType::Floating:
        return new CodeType(llvm::Type::getDoubleTy(context));
    case SyntaxType::Boolean:
        return new CodeType(llvm::Type::getInt1Ty(context));
    case SyntaxType::String:
        return new CodeType(llvm::ArrayType::get(llvm::Type::getInt8Ty(context), node.As<StringSyntax>().GetValue().size() + 1));
    case SyntaxType::ArrayLiteral:
    {
        auto arr = node.As<ArrayLiteral>();

        if (arr.GetValues().size() > 0)
        {
            auto type = LiteralType(arr.GetValues()[0]->GetExpression());
            return new CodeType(llvm::ArrayType::get(type->type, node.As<ArrayLiteral>().GetValues().size() + 1));
        }
        else
        {
            return new CodeType(llvm::ArrayType::get(llvm::Type::getInt8Ty(context), node.As<ArrayLiteral>().GetValues().size() + 1));
        }
    }
    default:
        return nullptr;
    }
}

CodeType *CodeGeneration::TypeType(const SyntaxNode &node)
{
    switch (node.GetType())
    {
    case SyntaxType::PrimitiveType:
    {
        auto type = node.As<PrimitiveType>().GetToken();
        switch (type.type)
        {
        case TokenType::Int:
            return new CodeType(llvm::IntegerType::get(context, type.ivalue == 0 ? 32 : type.ivalue), true);
        case TokenType::Uint:
            return new CodeType(llvm::IntegerType::get(context, type.ivalue == 0 ? 32 : type.ivalue), false);
        case TokenType::Float:
            return new CodeType(llvm::IntegerType::getDoubleTy(context), false);
        case TokenType::Bool:
            return new CodeType(llvm::Type::getInt1Ty(context), false);

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
                return new CodeType(dynamic_cast<TemplateNode *>(sym)->GetTemplate()->type->type);
            case SymbolNodeType::TypeAliasNode:
                return new CodeType(dynamic_cast<TemplateNode *>(sym)->GetTemplate()->type->type);
            default:
                return nullptr;
            }
        }
        return nullptr;
    }
    case SyntaxType::ArrayType:
    {
        auto arr = node.As<ArrayType>();
        auto type = TypeType(arr.GetArrayType());
        auto &size = arr.GetArraySize();
        if (size.GetType() == SyntaxType::Integer)
            return new CodeType(llvm::ArrayType::get(type->type, size.As<IntegerSyntax>().GetValue()));
        else
            return nullptr;
    }
    case SyntaxType::ReferenceType:
    {
        auto arr = node.As<ReferenceType>();
        auto type = TypeType(arr.GetReferenceType());
        return new CodeType(llvm::PointerType::get(type->type, 0));
    }
    case SyntaxType::FunctionType:
    {
        auto arr = node.As<FunctionType>();
        auto type = arr.GetRetType() ? TypeType(*arr.GetRetType()) : nullptr;
        std::vector<llvm::Type *> parameters;
        for (auto a : arr.GetParameters())
            parameters.push_back(TypeType(*a)->type);
        if (type)
            return new CodeType(llvm::FunctionType::get(type->type, parameters, false)->getPointerTo());
        else
            return new CodeType(llvm::FunctionType::get(llvm::Type::getVoidTy(context), parameters, false)->getPointerTo());
    }
    default:
        return nullptr;
        break;
    }
}

const CodeValue *CodeGeneration::Cast(const CodeValue *value, CodeType *toType, bool implicit)
{
    auto vt = value->type->type->getTypeID();
    auto tt = toType->type->getTypeID();
    if (vt == tt && vt == llvm::Type::FloatTyID)
        return value;
    else if (vt == tt && vt == llvm::Type::DoubleTyID)
        return value;
    else if (value->type->type == toType->type)
        return value;
    else if ((vt == llvm::Type::FloatTyID || vt == llvm::Type::DoubleTyID) && (tt == llvm::Type::FloatTyID || tt == llvm::Type::DoubleTyID))
    {
        return new CodeValue(builder.CreateFPCast(value->value, toType->type), toType);
    }
    else if ((vt == llvm::Type::FloatTyID || vt == llvm::Type::DoubleTyID) && (tt == llvm::Type::IntegerTyID))
    {
        if (!implicit)
        {
            if (toType->isSigned)
                return new CodeValue(builder.CreateFPToSI(value->value, toType->type), toType);
            else
                return new CodeValue(builder.CreateFPToUI(value->value, toType->type), toType);
        }
        else
        {
            // TODO throw error about not being able to implicitly cast
            return nullptr;
        }
    }
    else if ((tt == llvm::Type::FloatTyID || tt == llvm::Type::DoubleTyID) && (vt == llvm::Type::IntegerTyID))
    {
        if (toType->isSigned)
            return new CodeValue(builder.CreateSIToFP(value->value, toType->type), toType);
        else
            return new CodeValue(builder.CreateUIToFP(value->value, toType->type), toType);
    }
    else if (value->type->type != toType->type || value->type->isSigned != toType->isSigned)
    {
        auto val = builder.CreateIntCast(value->value, toType->type, toType->isSigned);
        return new CodeValue(val, toType);
    }
    return nullptr;
}

// llvm::Value *LoadIfAlloca(llvm::Value *value)
// {
//     if (value->getValueID() == llvm::Instruction::Alloca + llvm::Value::InstructionVal && initializer->GetType() == Parsing::SyntaxType::IdentifierExpression) // If the initializer is a variable, load it and store it
//     {
//         auto load = new CodeValue(gen.GetBuilder().CreateLoad(init->value), init->type);
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

namespace Parsing
{
    const CodeValue *VariableDeclerationStatement::CodeGen(CodeGeneration &gen) const
    {
        if (initializer && initializer->GetType() == SyntaxType::TemplateInitializer)
        {
            auto symbol = gen.FindSymbolInScope(initializer->As<TemplateInitializer>().GetIdentifier().raw);
            if (symbol != nullptr && symbol->GetType() == SymbolNodeType::TemplateNode)
            {
                auto &templ = symbol->As<TemplateNode>();
                if (keyword.type == TokenType::Const) // Constant variable
                {
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto inst = gen.CreateEntryBlockAlloca(templ.GetTemplate()->type->type, identifier.raw); // Allocate the variable on the stack

                    auto varValue = new CodeValue(inst, templ.GetTemplate()->type);
                    gen.SetCurrentVar(varValue);

                    initializer->CodeGen(gen);

                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
            }
            else
            {
                // TODO throw error cannot find symbol
            }
        }
        else
        {
            auto init = initializer ? initializer->CodeGen(gen) : nullptr;
            CodeType *type = nullptr;
            if (initializer && !this->type)
                type = init->type;
            else if (this->type)
                type = gen.TypeType(*this->type); // Get type value from type annotation
            else
            {
                // TODO throw error cannot determine type!
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

                        auto varValue = new CodeValue(global, type);
                        gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue);
                    }
                    else
                    {
                        // TODO throw error initilizaer of global is not constant
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

                        auto varValue = new CodeValue(global, type);
                        gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue);
                    }
                    else
                    {
                        // TODO throw error initilizaer of global is not constant
                    }
                }
            }
            else if (gen.GetInsertPoint()->GetType() == SymbolNodeType::FunctionNode) // Local variables
            {
                if (keyword.type == TokenType::Const) // Constant variable
                {
                    if (init)
                    {

                        CodeValue *varValue;
                        if (type) // If type is specified cast the value to the type
                        {
                            auto casted = gen.Cast(init, type);
                            varValue = new CodeValue(casted->value, type);
                            if (casted != init)
                                delete casted;
                        }
                        else
                            varValue = new CodeValue(init->value, type);

                        gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                        return varValue;
                    }
                    else
                    {
                        // TODO throw error const must have initiliazer
                    }
                }
                else if (keyword.type == TokenType::Let)
                {
                    auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw); // Allocate the variable on the stack

                    if (initializer)
                    {

                        if (type)
                        {
                            auto casted = gen.Cast(init, type);
                            gen.GetBuilder().CreateStore(casted->value, inst);
                            if (casted != init)
                                delete casted;
                        }
                        else
                            gen.GetBuilder().CreateStore(init->value, inst);
                    }

                    auto varValue = new CodeValue(inst, type);
                    gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue); // Insert variable into symbol tree
                    return varValue;
                }
            }
            else if (gen.GetInsertPoint()->GetType() == SymbolNodeType::TemplateNode) // Template members
            {
                auto &tn = gen.GetInsertPoint()->As<TemplateNode>();

                tn.AddMember(type->type);

                auto val = new CodeValue(nullptr, type);
                gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, val);
                return val;
            }
        }

        return nullptr;
    }

    const CodeValue *BinaryExpression::CodeGen(CodeGeneration &gen) const
    {
        auto left = LHS->CodeGen(gen);
        auto right = RHS->CodeGen(gen);
        if (left->type->type == right->type->type || (left->type->type->getTypeID() == llvm::Type::IntegerTyID && right->type->type->getTypeID() == llvm::Type::IntegerTyID))
        {
            auto ret = new CodeValue(nullptr, new CodeType(*left->type));
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
    }

    const CodeValue *UnaryExpression::CodeGen(CodeGeneration &gen) const
    {
        if (op.type == TokenType::Ampersand)
            gen.Use(CodeGeneration::Using::Reference);
        auto expr = expression->CodeGen(gen);
        if (op.type == TokenType::Ampersand)
            gen.UnUse(CodeGeneration::Using::Reference);
        auto ret = new CodeValue(nullptr, new CodeType(*expr->type));
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

            if (expr->value->getValueID() == llvm::Instruction::Alloca + llvm::Value::InstructionVal)
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

    const CodeValue *PostfixExpression::CodeGen(CodeGeneration &gen) const
    {
    }

    const CodeValue *IdentifierExpression::CodeGen(CodeGeneration &gen) const
    {
        const auto found = gen.FindSymbolInScope(identifierToken.raw);
        if (found)
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
                    auto load = gen.GetBuilder().CreateLoad(f->value);
                    auto value = new CodeValue(load, f->type);
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
            default:
                break;
            }
        }
        // TODO throw error identifier not found
    }

    const CodeValue *CallExpression::CodeGen(CodeGeneration &gen) const
    {
        auto fnExpr = static_cast<const FunctionCodeValue *>(fn->CodeGen(gen));
        if (fnExpr)
        {
            if (fnExpr->Function()->arg_size() != arguments.size())
            {
                // TODO throw error arguments don't match
                return nullptr;
            }
            std::vector<llvm::Value *> args(arguments.size());
            auto checkargs = fnExpr->Function()->args().begin();
            for (auto v : arguments)
            {
                auto val = v->CodeGen(gen);
                if (val->type->type == checkargs->getType())
                {
                    args.push_back(val->value);
                }
                else
                {
                    // TODO throw error arg type doesn't match!
                }
                checkargs++;
                delete val;
            }
            auto call = gen.GetBuilder().CreateCall(fnExpr->Function(), args);
            return new CodeValue(call, fnExpr->type);
        }
        // TODO throw error function callee is not function type!
        return nullptr;
    }

    const CodeValue *SubscriptExpression::CodeGen(CodeGeneration &gen) const
    {
    }

    const CodeValue *CastExpression::CodeGen(CodeGeneration &gen) const
    {
    }

    const CodeValue *ArrayLiteral::CodeGen(CodeGeneration &gen) const
    {
        auto type = static_cast<llvm::ArrayType *>(gen.LiteralType(*this)->type);
        std::vector<const llvm::Value *> vals;
        for (auto v : values)
        {
            switch (v->GetType())
            {
            case SyntaxType::ArrayLiteralExpressionEntry:
            {
                auto expr = v->As<ArrayLiteralExpressionEntry>().GetExpression().CodeGen(gen);
                vals.push_back(expr->value);
                break;
            }
            case SyntaxType::ArrayLiteralBoundaryEntry:
                /* code */
                break;

            default:
                break;
            }
        }

        // auto variable = new llvm::AllocaInst(
        //     I, "array_size", funcEntry);
        // return new CodeValue(llvm::makeArrayRef(vals), new CodeType(type));
    }

    const CodeValue *StringSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        std::vector<llvm::Constant *> vals;
        for (auto c : GetValue())
        {
            vals.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(gen.GetContext()), c));
        }
        vals.push_back(llvm::ConstantInt::get(llvm::Type::getInt8Ty(gen.GetContext()), 0));
        return new CodeValue(llvm::ConstantArray::get(static_cast<llvm::ArrayType *>(type->type), llvm::makeArrayRef(vals)), type);
    }

    const CodeValue *BooleanSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        return new CodeValue(llvm::ConstantInt::get(type->type, llvm::APInt(1, GetValue())), type);
    }

    const CodeValue *FloatingSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        return new CodeValue(llvm::ConstantFP::get(type->type, llvm::APFloat(GetValue())), type);
    }

    const CodeValue *IntegerSyntax::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.LiteralType(*this);
        return new CodeValue(llvm::ConstantInt::get(type->type, llvm::APInt(CodeGeneration::GetNumBits(GetValue()), GetValue())), type);
    }

    const CodeValue *FunctionDeclerationStatement::CodeGen(CodeGeneration &gen) const
    {
        auto checkFunc = gen.GetModule().getFunction(identifier.raw);
        CodeType *funcReturnType;
        if (!checkFunc)
        {
            funcReturnType = (retType == nullptr ? new CodeType(llvm::Type::getVoidTy(gen.GetContext())) : gen.TypeType(*retType));
            std::vector<llvm::Type *> parameters;
            for (auto p : this->parameters)
                parameters.push_back(gen.TypeType(*p->GetVariableType())->type);
            auto functionType = llvm::FunctionType::get(funcReturnType->type, parameters, false);
            checkFunc = llvm::Function::Create(functionType, gen.IsUsed(CodeGeneration::Using::Export) ? llvm::Function::ExternalLinkage : llvm::Function::PrivateLinkage, 0, gen.GenerateMangledName(identifier.raw), &(gen.GetModule()));
            if (IsPrototype())
                return new CodeValue(checkFunc, new CodeType(functionType));
        }

        if (!checkFunc->empty())
        {
            // TODO log error about function already existing
        }

        auto retBlock = llvm::BasicBlock::Create(gen.GetContext());
        auto fValue = new FunctionCodeValue(checkFunc, funcReturnType, nullptr, retBlock);
        gen.SetCurrentFunction(fValue);

        auto fnScope = gen.NewScope<FunctionNode>(identifier.raw, fValue);
        if (gen.IsUsed(CodeGeneration::Using::Export))
            fnScope->Export();

        auto insertPoint = llvm::BasicBlock::Create(gen.GetContext(), "entry", checkFunc);
        gen.GetBuilder().SetInsertPoint(insertPoint);

        if (retType)
        {
            auto retval = gen.CreateEntryBlockAlloca(checkFunc->getFunctionType()->getReturnType(), "ret");
            fValue->SetReturnLocation(retval);
        }

        gen.Use(CodeGeneration::Using::NoBlock);
        const CodeValue *funcBody = body->CodeGen(gen);

        if (retType)
        {
            if ((body->GetType() == SyntaxType::BlockStatement &&
                 body->As<BlockStatement<>>().GetStatements().size() > 0 &&
                 body->As<BlockStatement<>>().GetStatements().back()->GetType() == SyntaxType::ExpressionStatement) ||
                body->GetType() == SyntaxType::ExpressionStatement)
            {
                auto casted = gen.Cast(funcBody, funcReturnType);
                gen.Return(casted->value);
                // gen.GetBuilder().CreateRet(casted->value);
                if (casted != funcBody)
                    delete casted;
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
                auto load = gen.GetBuilder().CreateLoad(cfv->GetReturnLocation());
                gen.GetBuilder().CreateRet(load);
            }
            else
            {
                // TODO throw error about empty return statement with non void function
                cfv->GetReturnLocation()->eraseFromParent();
                gen.GetBuilder().CreateRet(llvm::Constant::getNullValue(fValue->Function()->getFunctionType()->getReturnType()));
                return nullptr;
            }
        }
        else
            gen.GetBuilder().CreateRetVoid();

        if (llvm::verifyFunction(*checkFunc, &llvm::errs()))
            return nullptr;

        gen.LastScope();
        return fValue;

        checkFunc->eraseFromParent();
        return nullptr;
    }

    const CodeValue *ExportDecleration::CodeGen(CodeGeneration &gen) const
    {
        gen.Use(CodeGeneration::Using::Export);
        auto stGen = statement->CodeGen(gen);
        gen.UnUse(CodeGeneration::Using::Export);
        return stGen;
    }

    const CodeValue *ReturnStatement::CodeGen(CodeGeneration &gen) const
    {
        auto retType = static_cast<llvm::Function *>(gen.GetCurrentFunction()->value)->getFunctionType()->getReturnType();
        if (retType->getTypeID() == llvm::Type::VoidTyID && expression != nullptr)
        {
            // TODO throw error returned value with a void function
        }
        else if (expression)
        {
            auto cgen = expression->CodeGen(gen);
            auto hretType = new CodeType(retType);
            auto expr = gen.Cast(cgen, hretType);
            delete hretType;

            gen.Return(expr->value);
            return nullptr;
        }
        else
        {
            // TODO throw error about empty return statement with non void function
            if (retType->isVoidTy())
                gen.Return();
            else
                gen.Return(llvm::Constant::getNullValue(retType));

            return nullptr;
        }
    }

    const CodeValue *IfStatement::CodeGen(CodeGeneration &gen) const
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
            delete expr;
        }
        else
        {
            delete expr;
            // TODO throw error if expression is not boolean
        }
        return nullptr;
    }

    const CodeValue *TemplateStatement::CodeGen(CodeGeneration &gen) const
    {
        if (gen.GetInsertPoint()->findSymbol(identifier.raw) != gen.GetInsertPoint()->GetChildren().end())
        {
            // TODO throw error template name already found
            return nullptr;
        }
        auto structType = llvm::StructType::create(gen.GetContext(), gen.GenerateMangledTypeName(identifier.raw));
        auto tValue = new CodeValue(nullptr, new CodeType(structType));

        auto fnScope = gen.NewScope<TemplateNode>(identifier.raw, structType);
        if (gen.IsUsed(CodeGeneration::Using::Export))
            fnScope->Export();

        gen.Use(CodeGeneration::Using::NoBlock);

        body->CodeGen(gen);

        structType->setBody(fnScope->GetMembers());

        gen.LastScope();
        return tValue;
    }

    const CodeValue *TemplateInitializer::CodeGen(CodeGeneration &gen) const
    {
        if (auto sym = gen.FindSymbolInScope<TemplateNode>(identifier.raw))
        {
            auto &children = sym->GetChildren();
            auto values = body->GetValues();
            llvm::Value *strc = nullptr;
            if (gen.GetCurrentVar())
            {
                strc = gen.GetCurrentVar()->value;
            }
            else
            {
                strc = gen.CreateEntryBlockAlloca(sym->GetTemplate()->type->type, "");
            }

            for (auto v : values)
            {
                const auto &key = v->GetKey().raw;
                auto found = sym->findSymbol(key);
                if (found != children.end() && found->second->GetType() == SymbolNodeType::VariableNode)
                {
                    int index = std::distance(children.begin(), found);
                    auto gep = gen.GetBuilder().CreateStructGEP(sym->GetTemplate()->type->type, strc, index);
                    auto val = gen.Cast(v->GetValue().CodeGen(gen), dynamic_cast<VariableNode *>(found->second)->GetVariable()->type);
                    gen.GetBuilder().CreateStore(val->value, gep);
                }
                else
                {
                    // TODO throw error key value not found in template
                }
            }
            if (!gen.GetCurrentVar())
            {
                return new CodeValue(gen.GetBuilder().CreateLoad(strc), sym->GetTemplate()->type);
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            // TODO throw error can not find template
        }
    }

    const CodeValue *ActionBaseStatement::CodeGen(CodeGeneration &gen) const
    {
        auto type = gen.TypeType(*templateType);
        TypeSyntax *t = templateType;

        while (t->GetType() == SyntaxType::GenericType)
            t = t->As<GenericType>().GetBaseType();

        if (type)
        {
            // std::string str = templateType->GetType() == SyntaxType::IdentifierType ? templateType->As<IdentifierType>().GetToken().raw : templateType->As<GenericType>().

            auto actnScope = gen.NewScope<ActionNode>(gen.GetInsertPoint()->GenerateName(), t->As<IdentifierType>().GetToken().raw);

            gen.Use(CodeGeneration::Using::NoBlock);

            body->CodeGen(gen);

            gen.LastScope();
        }
        return nullptr;
    }

    const CodeValue *LoopStatement::CodeGen(CodeGeneration &gen) const
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
