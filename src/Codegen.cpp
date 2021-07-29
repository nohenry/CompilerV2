#include <CodeGen.hpp>
#include <Parser.hpp>
#include <ModuleUnit.hpp>

#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>

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
        // TODO search up in symbol tree
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

        if (gen.GetInsertPoint()->GetType() == SymbolNodeType::ModuleNode)
        {
            if (keyword.type == TokenType::Const)
            {
                if (llvm::isa<llvm::Constant>(init->value))
                {
                    auto global = new llvm::GlobalVariable(gen.GetModule(), type->type, true, gen.IsUsed(CodeGeneration::Using::Export) ? llvm::GlobalValue::LinkageTypes::ExternalLinkage : llvm::GlobalValue::LinkageTypes::PrivateLinkage, static_cast<llvm::Constant *>(init->value), gen.GenerateMangledName(identifier.raw));
                    const llvm::DataLayout &DL = gen.GetModule().getDataLayout();
                    llvm::Align AllocaAlign = DL.getPrefTypeAlign(type->type);
                    global->setAlignment(AllocaAlign);
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
                }
                else
                {
                    // TODO throw error initilizaer of global is not constant
                }
            }
        }
        else if (gen.GetInsertPoint()->GetType() == SymbolNodeType::FunctionNode)
        {
            if (keyword.type == TokenType::Const)
            {
                auto varValue = new CodeValue(type ? gen.Cast(init, type)->value : init->value, type);
                gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue);
                return varValue;
            }
            else if (keyword.type == TokenType::Let)
            {
                auto inst = gen.CreateEntryBlockAlloca(type->type, identifier.raw);

                if (initializer)
                {
                    if (init->value->getValueID() == llvm::Instruction::Alloca + llvm::Value::InstructionVal && initializer->GetType() == Parsing::SyntaxType::IdentifierExpression)
                    {
                        auto load = new CodeValue(gen.GetBuilder().CreateLoad(init->value), init->type);
                        // value->type->type = load->getType();
                        gen.GetBuilder().CreateStore(type ? gen.Cast(load, type)->value : init->value, inst);
                        delete load;
                    }
                    else
                    {
                        gen.GetBuilder().CreateStore(type ? gen.Cast(init, type)->value : init->value, inst);
                    }
                }

                auto varValue = new CodeValue(inst, type);
                gen.GetInsertPoint()->AddChild<VariableNode>(identifier.raw, varValue);
                return varValue;
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
            case TokenType::Or:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateLogicalOr(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::And:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateLogicalAnd(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
            }
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
            case TokenType::TripleRightShift:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateROR(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            case TokenType::TripleLeftShift:
            {
                switch (left->type->type->getTypeID())
                {
                case llvm::Type::IntegerTyID:
                    ret->value = gen.GetBuilder().CreateROL(left->value, right->value);
                    break;
                case llvm::Type::StructTyID:
                    //  TODO operator overloading
                    break;
                default:
                    break;
                }
                break;
            }
            default:
                break;
            }
            return ret;
        }
    }

    const CodeValue *UnaryExpression::CodeGen(CodeGeneration &gen) const
    {
        auto expr = expression->CodeGen(gen);
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
                // auto load = gen.GetBuilder().CreateLoad(f->value);
                // auto value = new CodeValue(load, f->type);
                // value->type->type = load->getType();
                return f;
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
        if (!checkFunc)
        {
            auto funcReturnType = retType == nullptr ? new CodeType(llvm::Type::getVoidTy(gen.GetContext())) : gen.TypeType(*retType);
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

        auto fValue = new CodeValue(checkFunc, new CodeType(checkFunc->getType()));
        gen.SetCurrentFunction(fValue);

        auto fnScope = gen.NewScope<FunctionNode>(identifier.raw, fValue);
        if (gen.IsUsed(CodeGeneration::Using::Export))
            fnScope->Export();

        auto insertPoint = llvm::BasicBlock::Create(gen.GetContext(), "entry", checkFunc);
        gen.GetBuilder().SetInsertPoint(insertPoint);

        gen.Use(CodeGeneration::Using::NoBlock);
        const CodeValue *funcBody = body->CodeGen(gen);

        if (retType)
        {
            // gen.GetBuilder().CreateRet()
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
