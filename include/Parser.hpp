#pragma once

#include <stdint.h>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <bitset>

#include <Log.hpp>
#include <Token.hpp>
#include <Trie.hpp>
#include <Errors.hpp>
#include <CodeGen.hpp>

#include <llvm/IR/Value.h>

struct CodeValue;
class CodeGeneration;
class ModuleUnit;

namespace Parsing
{

    class ExpressionSyntax : public SyntaxNode
    {
    public:
        virtual ~ExpressionSyntax() {}
        virtual const CodeValue *CodeGen(CodeGeneration &gen) const = 0;
    };

    using Expression = ExpressionSyntax *;

    class TypeSyntax : public SyntaxNode
    {
    public:
        virtual ~TypeSyntax() {}
    };

    class TypeExpression : public ExpressionSyntax
    {
    private:
        TypeSyntax *type;

    public:
        TypeExpression(TypeSyntax *type) : type{type} {}
        virtual ~TypeExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::TypeExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *type;
        }

        virtual const Position &GetStart() const override
        {
            return type->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return type->GetEnd();
        }
    };

    class PrimitiveType : public TypeSyntax
    {
    private:
        const Token &token;

    public:
        PrimitiveType(const Token &token) : token{token} {}
        virtual ~PrimitiveType() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::PrimitiveType; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return token.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return token.GetEnd();
        }

        const auto &GetToken() const { return token; }
    };

    class IdentifierType : public TypeSyntax
    {
    private:
        const Token &token;

    public:
        IdentifierType(const Token &token) : token{token} {}
        virtual ~IdentifierType() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::IdentifierType; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return token.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return token.GetEnd();
        }

        const auto &GetToken() const { return token; }
    };

    class ArrayType : public TypeSyntax
    {
    private:
        const Token &open;
        TypeSyntax *type;
        const Token &colon;
        Expression size;
        const Token &close;

    public:
        ArrayType(const Token &open,
                  TypeSyntax *type,
                  const Token &colon,
                  Expression size,
                  const Token &close) : open{open}, type{type}, colon{colon}, size{size}, close{close} {}
        ArrayType(const Token &open,
                  TypeSyntax *type,
                  const Token &close) : open{open}, type{type}, colon{TokenNull}, size{nullptr}, close{close} {}
        virtual ~ArrayType() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ArrayType; }

        virtual const uint8_t NumChildren() const override
        {
            return 1 + (colon == TokenNull ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *type;
            case 1:
                return *size;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return open.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return close.GetEnd();
        }

        const auto &GetOpen() const { return open; }
        const auto &GetArrayType() const { return *type; }
        const auto &GetColon() const { return colon; }
        const auto &GetArraySize() const { return *size; }
        const auto &GetClose() const { return close; }
    };

    class FunctionType : public TypeSyntax
    {
        const Token &left;
        std::vector<TypeSyntax *> parameters;
        const Token &right;
        const Token &arrow;
        TypeSyntax *retType;

    public:
        FunctionType(const Token &left,
                     const std::vector<TypeSyntax *> &parameters,
                     const Token &right,
                     const Token &arrow,
                     TypeSyntax *retType) : left{left}, parameters{parameters}, right{right}, arrow{arrow}, retType{retType} {}
        virtual ~FunctionType() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::FunctionType; }

        virtual const uint8_t NumChildren() const override
        {
            return parameters.size() + (retType != nullptr ? 1 : 0);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            if (index < parameters.size())
                return *parameters[index];
            else
                return *retType;
        }

        virtual const Position &GetStart() const override
        {
            return left.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return retType->GetEnd();
        }

        const auto &GetLeft() const { return left; }
        const auto &GetParameters() const { return parameters; }
        const auto &GetRight() const { return right; }
        const auto &GetArrow() const { return arrow; }
        const auto GetRetType() const { return retType; }
    };

    class ReferenceType : public TypeSyntax
    {
    private:
        const Token &token;
        TypeSyntax *type;

    public:
        ReferenceType(const Token &token, TypeSyntax *type);
        virtual ~ReferenceType() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ReferenceType; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *type;
        }

        virtual const Position &GetStart() const override
        {
            return token.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return type->GetEnd();
        }

        const auto &GetToken() const { return token; }
        const auto &GetReferenceType() const { return *type; }
    };

    class GenericType : public TypeSyntax
    {
    private:
        TypeSyntax *type;
        const Token &left;
        std::vector<TypeSyntax *> arguments;
        const Token &right;

    public:
        GenericType(TypeSyntax *type,
                    const Token &left,
                    const std::vector<TypeSyntax *> &arguments,
                    const Token &right) : type{type}, left{left}, arguments{arguments}, right{right} {}
        virtual ~GenericType() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::GenericType; }

        virtual const uint8_t NumChildren() const override
        {
            return arguments.size() + 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            if (index == 0)
                return *type;
            else
                return *arguments[index - 1];
        }

        virtual const Position &GetStart() const override
        {
            return type->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return right.GetEnd();
        }

        // const auto &GetBaseType() const { return *type; }
        auto GetBaseType() { return type; }
        const auto &GetLeft() const { return left; }
        const auto &GetArguments() const { return arguments; }
        const auto &GetRight() const { return right; }
    };

    class IntegerSyntax : public ExpressionSyntax
    {
    private:
        const Token &valueToken;

    public:
        IntegerSyntax(const Token &token) : valueToken{token}
        {
            assert(valueToken.type == TokenType::Integer && "Token should be an integer token");
        }
        virtual ~IntegerSyntax() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::Integer; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return valueToken;
        }

        virtual const Position &GetStart() const override
        {
            return valueToken.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return valueToken.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const uint64_t GetValue() const { return valueToken.ivalue; }
        const std::string &GetRawValue() const { return valueToken.raw; }
    };

    class FloatingSyntax : public ExpressionSyntax
    {
    private:
        const Token &valueToken;

    public:
        FloatingSyntax(const Token &token) : valueToken{token}
        {
            assert(valueToken.type == TokenType::Floating && "Token should be an integer token");
        }
        virtual ~FloatingSyntax() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::Floating; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return valueToken;
        }

        virtual const Position &GetStart() const override
        {
            return valueToken.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return valueToken.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const double GetValue() const { return valueToken.fvalue; }
        const std::string &GetRawValue() const { return valueToken.raw; }
    };

    class BooleanSyntax : public ExpressionSyntax
    {
    private:
        const Token &boolToken;
        bool value;

    public:
        BooleanSyntax(const Token &token) : boolToken{token}, value{boolToken.type == TokenType::True}
        {
            assert((boolToken.type == TokenType::True || boolToken.type == TokenType::False) && "Token should be an boolean token");
        }
        virtual ~BooleanSyntax() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::Boolean; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return boolToken;
        }

        virtual const Position &GetStart() const override
        {
            return boolToken.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return boolToken.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetBoolToken() const { return boolToken; }
        const auto &GetValue() const { return value; }
    };

    class StringSyntax : public ExpressionSyntax
    {
    private:
        const Token &token;

    public:
        StringSyntax(const Token &token) : token{token}
        {
            assert(token.type == TokenType::String && "Token should be an boolean token");
        }
        virtual ~StringSyntax() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::String; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return token.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return token.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetToken() const { return token; }
        const auto &GetValue() const { return token.raw; }
    };

    class ObjectKeyValue : public SyntaxNode
    {
    private:
        const Token &key;
        const Token &colon;
        Expression value;

    public:
        ObjectKeyValue(const Token &key, const Token &colon, Expression value) : key{key}, colon{colon}, value{value}
        {
        }
        virtual ~ObjectKeyValue() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ObjectKeyValue; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *value;
        }

        virtual const Position &GetStart() const override
        {
            return key.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return value->GetEnd();
        }

        const auto &GetKey() const { return key; }
        const auto &GetColon() const { return colon; }
        const auto &GetValue() const { return *value; }
    };

    class ObjectInitializer : public ExpressionSyntax
    {
    private:
        const Token &left;
        std::vector<ObjectKeyValue *> values;
        const Token &right;

    public:
        ObjectInitializer(const Token &left,
                          std::vector<ObjectKeyValue *> values,
                          const Token &right) : left{left}, values{values}, right{right}
        {
        }
        virtual ~ObjectInitializer() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ObjectInitializer; }

        virtual const uint8_t NumChildren() const override
        {
            return values.size();
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *values[index];
        }

        virtual const Position &GetStart() const override
        {
            return left.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return right.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetLeft() const { return left; }
        const auto &GetValues() const { return values; }
        const auto &GetRight() const { return right; }
    };

    class TemplateInitializer : public ExpressionSyntax
    {
    private:
        const Token &identifier;
        ObjectInitializer *body;

    public:
        TemplateInitializer(const Token &identifier, ObjectInitializer *body) : identifier{identifier}, body{body}
        {
        }
        virtual ~TemplateInitializer() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::TemplateInitializer; }

        virtual const uint8_t NumChildren() const override
        {
            return body == nullptr ? 0 : 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *body;
        }

        virtual const Position &GetStart() const override
        {
            return identifier.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetIdentifier() const { return identifier; }
        const auto &GetBody() const { return *body; }
    };

    class ArrayLiteralEntry : public SyntaxNode
    {
    public:
        virtual ~ArrayLiteralEntry() {}
        virtual const ExpressionSyntax &GetExpression() const = 0;
    };

    class ArrayLiteralExpressionEntry : public ArrayLiteralEntry
    {
    private:
        Expression expression;

    public:
        ArrayLiteralExpressionEntry(Expression expression) : expression{expression}
        {
        }
        virtual ~ArrayLiteralExpressionEntry() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ArrayLiteralExpressionEntry; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *expression;
        }

        virtual const Position &GetStart() const override
        {
            return expression->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        // virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const ExpressionSyntax &GetExpression() const override { return *expression; }
    };

    class ArrayLiteralBoundaryEntry : public ArrayLiteralEntry
    {
    private:
        Expression expression;
        const Token &colon;
        Expression boundary;

    public:
        ArrayLiteralBoundaryEntry(Expression expression,
                                  const Token &colon,
                                  Expression boundary) : expression{expression}, colon{colon}, boundary{boundary}
        {
        }
        virtual ~ArrayLiteralBoundaryEntry() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ArrayLiteralBoundaryEntry; }

        virtual const uint8_t NumChildren() const override
        {
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expression;
            case 1:
                return *boundary;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return expression->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        // virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const ExpressionSyntax &GetExpression() const override { return *expression; }
        const auto &GetColon() const { return colon; }
        const auto &GetBoundary() const { return *boundary; }
    };

    class ArrayLiteral : public ExpressionSyntax
    {
    private:
        const Token &left;
        std::vector<ArrayLiteralEntry *> values;
        const Token &right;

    public:
        ArrayLiteral(const Token &left, const std::vector<ArrayLiteralEntry *> &values, const Token &right) : left{left}, values{values}, right{right}
        {
        }
        virtual ~ArrayLiteral() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ArrayLiteral; }

        virtual const uint8_t NumChildren() const override
        {
            return values.size();
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *values[index];
        }

        virtual const Position &GetStart() const override
        {
            return left.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return right.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetLeft() const { return left; }
        const auto &GetValues() const { return values; }
        const auto &GetRight() const { return right; }
    };

    class BinaryExpression : public ExpressionSyntax
    {
    private:
        Expression LHS;
        Expression RHS;
        const Token &op;

    public:
        BinaryExpression(ExpressionSyntax *LHS, ExpressionSyntax *RHS, const Token &op) : LHS{LHS}, RHS{RHS}, op{op}
        {
        }
        virtual ~BinaryExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::BinaryExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *LHS;
            case 1:
                return *RHS;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return LHS->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return RHS->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetLHS() const { return *LHS; }
        const auto &GetRHS() const { return *RHS; }
        const auto &GetOperator() const { return op; }
    };

    class UnaryExpression : public ExpressionSyntax
    {
    private:
        Expression expression;
        const Token &op;

    public:
        UnaryExpression(Expression expression, const Token &op) : expression{expression}, op{op}
        {
        }
        virtual ~UnaryExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::UnaryExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expression;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return op.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetExpression() const { return *expression; }
        const auto &GetOperator() const { return op; }
    };

    class PostfixExpression : public ExpressionSyntax
    {
    private:
        Expression expression;
        const Token &op;

    public:
        PostfixExpression(Expression expression, const Token &op) : expression{expression}, op{op}
        {
        }
        virtual ~PostfixExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::PostfixExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expression;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return op.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetExpression() const { return *expression; }
        const auto &GetOperator() const { return op; }
    };

    class IdentifierExpression : public ExpressionSyntax
    {
    private:
        const Token &identifierToken;

    public:
        IdentifierExpression(const Token &identifierToken) : identifierToken{identifierToken} {}
        virtual ~IdentifierExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::IdentifierExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return identifierToken.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return identifierToken.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetIdentiferToken() const { return identifierToken; }
    };

    class CallExpression : public ExpressionSyntax
    {
    private:
        Expression fn;
        const Token &leftParen;
        const Token &rightParen;
        std::vector<Expression> arguments;

    public:
        CallExpression(Expression fn, const Token &leftParen,
                       const Token &rightParen, std::vector<Expression> arguments) : fn{fn}, leftParen{leftParen}, rightParen{rightParen}, arguments{arguments}
        {
        }
        virtual ~CallExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::CallExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return arguments.size() + 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *fn;
            default:
                return *arguments[index - 1];
            }
        }

        virtual const Position &GetStart() const override
        {
            return fn->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return arguments.back()->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetFunctionExpression() const { return *fn; }
        const auto &GetFunctionArgs() const { return arguments; }
    };

    class SubscriptExpression : public ExpressionSyntax
    {
    private:
        Expression expr;
        const Token &left;
        Expression subsr;
        const Token &right;

    public:
        SubscriptExpression(Expression expr,
                            const Token &left,
                            Expression subsr,
                            const Token &right) : expr{expr}, left{left}, subsr{subsr}, right{right}
        {
        }
        virtual ~SubscriptExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::SubscriptExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expr;
            default:
                return *subsr;
            }
        }

        virtual const Position &GetStart() const override
        {
            return expr->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return right.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetExpression() const { return *expr; }
        const auto &GetLeft() const { return left; }
        const auto &GetSubscript() const { return *subsr; }
        const auto &GetRight() const { return right; }
    };

    class CastExpression : public ExpressionSyntax
    {
    private:
        Expression LHS;
        const Token &keyword;
        TypeSyntax *type;

    public:
        CastExpression(Expression LHS,
                       const Token &keyword,
                       TypeSyntax *type) : LHS{LHS}, keyword{keyword}, type{type}
        {
        }
        virtual ~CastExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::CastExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *LHS;
            default:
                return *type;
            }
        }

        virtual const Position &GetStart() const override
        {
            return LHS->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return type->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetExpression() const { return *LHS; }
        const auto &GetKeyword() const { return keyword; }
        const auto &GetCastType() const { return *type; }
    };

    class StatementSyntax : public SyntaxNode
    {
    public:
        virtual ~StatementSyntax() {}
        virtual const CodeValue *CodeGen(CodeGeneration &gen) const = 0;
    };

    using Statement = StatementSyntax *;

    template <class T>
    concept IsSyntaxNode = std::is_base_of<SyntaxNode, T>::value;

    template <class T>
    concept IsStatement = std::is_base_of<StatementSyntax, T>::value;

    template <class T>
    concept IsExpression = std::is_base_of<ExpressionSyntax, T>::value;

    class GenericParameterEntry : public SyntaxNode
    {
    private:
        const Token &identifier;
        std::vector<TypeSyntax *> constraints;

    public:
        GenericParameterEntry(const Token &identifier, const std::vector<TypeSyntax *> &constraints) : identifier{identifier}, constraints{constraints} {}
        virtual ~GenericParameterEntry() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::GenericParameterEntry; }

        virtual const uint8_t NumChildren() const override
        {
            return constraints.size();
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *constraints[index];
        }

        virtual const Position &GetStart() const override
        {
            return identifier.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return constraints.back()->GetEnd();
        }

        const auto &GetIdentifier() const { return identifier; }
        const auto &GetConstraints() const { return constraints; }
    };

    class GenericParameter : public SyntaxNode
    {

    private:
        const Token &left;
        std::vector<GenericParameterEntry *> parameters;
        const Token &right;

    public:
        GenericParameter(const Token &left,
                         const std::vector<GenericParameterEntry *> &parameters,
                         const Token &right) : left{left}, parameters{parameters}, right{right} {}
        virtual ~GenericParameter() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::GenericParameter; }

        virtual const uint8_t NumChildren() const override
        {
            return parameters.size();
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *parameters[index];
        }

        virtual const Position &GetStart() const override
        {
            return left.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return right.GetStart();
        }

        const auto &GetLeft() const { return left; }
        const auto &GetParameters() const { return parameters; }
        const auto &GetRight() const { return right; }
    };

    class ExpressionBodyStatement : public StatementSyntax
    {
    private:
        const Token &getArrow;
        Statement get;
        const Token &setArrow;
        Statement set;

    public:
        ExpressionBodyStatement(const Token &getArrow,
                                Statement get,
                                const Token &setArrow,
                                Statement set) : getArrow{getArrow}, get{get}, setArrow{setArrow}, set{set} {}
        ExpressionBodyStatement(const Token &getArrow,
                                const Token &setArrow,
                                Statement set) : getArrow{getArrow}, get{nullptr}, setArrow{setArrow}, set{set} {}
        ExpressionBodyStatement(const Token &getArrow,
                                Statement get) : getArrow{getArrow}, get{get}, setArrow{TokenNull}, set{nullptr} {}
        virtual ~ExpressionBodyStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ExpressionBodyStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return (get == nullptr ? 0 : 1) + (set == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (get)
                    return *get;
                else
                    return *set;
            default:
                return *set;
            }
        }

        virtual const Position &GetStart() const override
        {
            return getArrow.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            if (set)
            {
                return set->GetStart();
            }
            else
            {
                return get->GetStart();
            }
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetGetArrow() const { return getArrow; }
        const auto &GetGetStatement() const { return *get; }
        const auto &GetSetArrow() const { return setArrow; }
        const auto &GetSetStatement() const { return *set; }
    };

    class ExpressionBodySpecStatement : public StatementSyntax
    {
    private:
        const Token &left;
        const Token &get;
        const Token &set;
        const Token &right;
        bool hasGet;
        bool hasSet;

    public:
        ExpressionBodySpecStatement(const Token &left,
                                    const Token &get,
                                    const Token &set,
                                    const Token &right) : left{left}, get{get}, set{set}, right{right}, hasGet{get != TokenNull}, hasSet{set != TokenNull} {}
        virtual ~ExpressionBodySpecStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ExpressionBodySpecStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return get;
        }

        virtual const Position &GetStart() const override
        {
            return left.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return right.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetLeft() const { return left; }
        const auto &GetGet() const { return get; }
        const auto &GetSet() const { return set; }
        const auto &GetRight() const { return right; }
        const bool HasGet() const { return hasGet; }
        const bool HasSet() const { return hasSet; }
    };

    template <IsSyntaxNode T = StatementSyntax>
    class BlockStatement : public StatementSyntax
    {
    private:
        using PT = T *;

    private:
        const Token &open;
        std::vector<PT> statements;
        const Token &close;

    public:
        BlockStatement(const Token &open,
                       const std::vector<PT> &statements,
                       const Token &close) : open{open}, statements{statements}, close{close} {}
        virtual ~BlockStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::BlockStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return statements.size();
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *statements[index];
        }

        virtual const Position &GetStart() const override
        {
            return open.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return close.GetStart();
        }

        virtual CodeValue *CodeGen(CodeGeneration &gen) const override
        {
            bool used = gen.IsUsed(CodeGeneration::Using::NoBlock);
            if (used)
                gen.UnUse(CodeGeneration::Using::NoBlock);
            else
                gen.NewScope<ScopeNode>();

            CodeValue *ret = nullptr;
            for (auto s : statements)
            {
                ret = (CodeValue *)s->CodeGen(gen);
                if (static_cast<SyntaxNode *>(s)->GetType() == SyntaxType::ReturnStatement &&
                    gen.GetInsertPoint()->GetType() == SymbolNodeType::FunctionNode)
                    break;
            }

            if (!used)
                gen.LastScope();
            return ret;
        }

        virtual void PreCodeGen(CodeGeneration &gen) const override
        {
            for (auto s : statements)
                s->PreCodeGen(gen);
        }

        const auto &GetOpen() const { return open; }
        const auto &GetStatements() const { return statements; }
        const auto &GetClose() const { return close; }
    };

    class TemplateStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        GenericParameter *generic;
        // const Token &open;
        // std::vector<Statement> statements;
        // const Token &close;
        BlockStatement<> *body;

    public:
        TemplateStatement(const Token &keyword,
                          const Token &identifier,
                          GenericParameter *generic,
                          const Token &open,
                          const std::vector<Statement> &statements,
                          const Token &close) : keyword{keyword}, identifier{identifier}, generic{generic}, body{new BlockStatement(open, statements, close)} {}
        virtual ~TemplateStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::TemplateStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1 + (generic == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (generic)
                    return *generic;
                else
                    return *body;
            default:
                return *body;
                break;
            }
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetStart();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;
        virtual void PreCodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetBody() const { return *body; }
    };

    class SpecStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        GenericParameter *generic;
        // const Token &open;
        // std::vector<Statement> statements;
        // const Token &close;
        BlockStatement<> *body;

    public:
        SpecStatement(const Token &keyword,
                      const Token &identifier,
                      GenericParameter *generic,
                      const Token &open,
                      const std::vector<Statement> &statements,
                      const Token &close) : keyword{keyword}, identifier{identifier}, generic{generic}, body{new BlockStatement(open, statements, close)} {}
        virtual ~SpecStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::SpecStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1 + generic == nullptr ? 0 : 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (generic)
                    return *generic;
                else
                    return *body;
            default:
                return *body;
                break;
            }
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetStart();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;
        virtual void PreCodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetBody() const { return *body; }
    };

    class ExpressionStatement : public StatementSyntax
    {
    private:
        Expression expression;

    public:
        ExpressionStatement(Expression expression) : expression{expression} {}
        virtual ~ExpressionStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ExpressionStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *expression;
        }

        virtual const Position &GetStart() const override
        {
            return expression->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override
        {
            return expression->CodeGen(gen);
        }
    };

    class VariableDeclerationStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        TypeSyntax *type;
        Expression initializer;
        ExpressionBodyStatement *expressionBody;
        ExpressionBodySpecStatement *specExpressionBody;

    public:
        VariableDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     TypeSyntax *type = nullptr,
                                     Expression initializer = nullptr) : keyword{keyword}, identifier{identifier}, type{type}, initializer{initializer}, expressionBody{nullptr}, specExpressionBody{nullptr} {}
        VariableDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     ExpressionBodyStatement *expressionBody,
                                     TypeSyntax *type = nullptr) : keyword{keyword}, identifier{identifier}, type{type}, initializer{nullptr}, expressionBody{expressionBody}, specExpressionBody{nullptr} {}
        VariableDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     ExpressionBodySpecStatement *specExpressionBody,
                                     TypeSyntax *type) : keyword{keyword}, identifier{identifier}, type{type}, initializer{nullptr}, expressionBody{nullptr}, specExpressionBody{specExpressionBody} {}
        virtual ~VariableDeclerationStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::VariableDeclerationStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return (type == nullptr ? 0 : 1) + (initializer == nullptr ? 0 : 1) + (expressionBody == nullptr ? 0 : 1) + (specExpressionBody == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int inindex) const override
        {
            int index = 0;
            int tIndex = type == nullptr ? -1 : index++;
            int iIndex = initializer == nullptr ? -1 : index++;
            int eIndex = expressionBody == nullptr ? -1 : index++;
            int sIndex = specExpressionBody == nullptr ? -1 : index++;

            if (inindex == tIndex)
                return *type;
            else if (inindex == iIndex)
                return *initializer;
            else if (inindex == eIndex)
                return *expressionBody;
            else if (inindex == sIndex)
                return *specExpressionBody;
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return initializer->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto GetVariableType() const { return type; }
        const auto &GetInitializer() const { return *initializer; }
        const auto &GetExpressionBody() const { return *expressionBody; }
        const auto &GetSpecExpressionBody() const { return *specExpressionBody; }
        bool HasInitializer() const { return initializer != nullptr; }
    };

    class FunctionDeclerationStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        GenericParameter *generic;
        const Token &left;
        std::vector<VariableDeclerationStatement *> parameters;
        const Token &right;
        const Token &arrow;
        TypeSyntax *retType;
        Statement body;

    public:
        FunctionDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     GenericParameter *generic,
                                     const Token &left,
                                     const std::vector<VariableDeclerationStatement *> &parameters,
                                     const Token &right,
                                     const Token &arrow = TokenNull,
                                     TypeSyntax *retType = nullptr,
                                     Statement body = nullptr) : keyword{keyword}, identifier{identifier}, generic{generic}, left{left}, parameters{parameters}, right{right}, arrow{arrow}, retType{retType}, body{body} {}
        virtual ~FunctionDeclerationStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::FunctionDeclerationStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return parameters.size() + (body == nullptr ? 0 : 1) + (retType == nullptr ? 0 : 1) + (generic == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int inindex) const override
        {
            int index = 0;
            int gIndex = generic == nullptr ? -1 : index++;
            int pIndex = parameters.empty() ? -1 : index;
            index += parameters.size();
            int rIndex = retType == nullptr ? -1 : index++;
            int bIndex = body == nullptr ? -1 : index++;

            if (inindex == gIndex)
                return *generic;
            else if (pIndex > -1 && inindex >= pIndex && (inindex < pIndex + parameters.size()))
                return *parameters[inindex - pIndex];
            else if (inindex == rIndex)
                return *retType;
            else if (inindex == bIndex)
                return *body;
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;
        virtual void PreCodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetLeftParen() const { return left; }
        const auto &GetParameters() const { return parameters; }
        const auto &GetRightParen() const { return right; }
        const auto &GetFuncArrow() const { return arrow; }
        const auto GetRetType() const { return retType; }
        const auto &GetBody() const { return body; }
        const bool IsPrototype() const { return body == nullptr; }
    };

    class AnonymousFunctionExpression : public ExpressionSyntax
    {
    private:
        const Token &left;
        std::vector<VariableDeclerationStatement *> parameters;
        const Token &right;
        const Token &arrow;
        TypeSyntax *retType;
        Statement body;

    public:
        AnonymousFunctionExpression(const Token &left,
                                    std::vector<VariableDeclerationStatement *> parameters,
                                    const Token &right,
                                    const Token &arrow,
                                    TypeSyntax *retType,
                                    Statement body) : left{left}, parameters{parameters}, right{right}, arrow{arrow}, retType{retType}, body{body}
        {
        }
        virtual ~AnonymousFunctionExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::AnonymousFunctionExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return parameters.size() + (body == nullptr ? 0 : 1) + (retType == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int inindex) const override
        {
            int index = 0;
            int pIndex = parameters.empty() ? -1 : index;
            index += parameters.size();
            int rIndex = retType == nullptr ? -1 : index++;
            int bIndex = body == nullptr ? -1 : index++;

            if (inindex >= pIndex && inindex < rIndex)
                return *parameters[inindex - pIndex];
            else if (inindex == rIndex)
                return *retType;
            else if (inindex == bIndex)
                return *body;
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return left.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetLeftParen() const { return left; }
        const auto &GetParameters() const { return parameters; }
        const auto &GetRightParen() const { return right; }
        const auto &GetFuncArrow() const { return arrow; }
        const auto &GetRetType() const { return retType; }
        const auto &GetBody() const { return body; }
    };

    class ElseStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        Statement body;

    public:
        ElseStatement(const Token &keyword,
                      Statement body) : keyword{keyword}, body{body}
        {
        }
        virtual ~ElseStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ElseStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *body;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override
        {
            return body->CodeGen(gen);
        }

        const auto &GetKeyword() const { return keyword; }
        const auto &GetBody() const { return *body; }
    };

    class IfStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        Expression expression;
        Statement body;
        ElseStatement *elseClause;

    public:
        IfStatement(const Token &keyword,
                    Expression expression,
                    Statement body,
                    ElseStatement *elseClause = nullptr) : keyword{keyword}, expression{expression}, body{body}, elseClause{elseClause} {}
        virtual ~IfStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::IfStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 2 + (elseClause == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expression;
            case 1:
                return *body;
            case 2:
                if (elseClause)
                    return *elseClause;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expression; }
        const auto &GetBody() const { return *body; }
        const auto &GetElse() const { return *elseClause; }
    };

    class LoopStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        Expression expression;
        Statement body;

    public:
        LoopStatement(const Token &keyword,
                      Expression expression,
                      Statement body) : keyword{keyword}, expression{expression}, body{body} {}
        virtual ~LoopStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::LoopStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1 + (expression == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (expression != nullptr)
                    return *expression;
                else
                    return *body;
            case 1:
                return *body;
            }
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expression; }
        const auto &GetBody() const { return *body; }
    };

    class ReturnStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        Expression expression;

    public:
        ReturnStatement(const Token &keyword,
                        Expression expression) : keyword{keyword}, expression{expression} {}
        virtual ~ReturnStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ReturnStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return expression == nullptr ? 0 : 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *expression;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expression; }
    };

    class YieldStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        Expression expression;

    public:
        YieldStatement(const Token &keyword,
                       Expression expression) : keyword{keyword}, expression{expression} {}
        virtual ~YieldStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::YieldStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *expression;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return expression->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expression; }
    };

    class ActionBaseStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        TypeSyntax *templateType;
        BlockStatement<> *body;

    public:
        ActionBaseStatement(const Token &keyword,
                            TypeSyntax *templateType,
                            BlockStatement<> *body) : keyword{keyword}, templateType{templateType}, body{body} {}
        virtual ~ActionBaseStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ActionBaseStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *templateType;
            case 1:
                return *body;
            }
            return *templateType;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;
        virtual void PreCodeGen(CodeGeneration &gen) const override;
        const auto &GetKeyword() const { return keyword; }
        const auto &GetTemplateType() const { return *templateType; }
        const auto &GetBody() const { return *body; }
    };

    class ActionSpecStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        TypeSyntax *specType;
        const Token &in;
        TypeSyntax *templateType;
        BlockStatement<> *body;

    public:
        ActionSpecStatement(const Token &keyword,
                            TypeSyntax *specType,
                            const Token &in,
                            TypeSyntax *templateType,
                            BlockStatement<> *body) : keyword{keyword}, specType{specType}, in{in}, templateType{templateType}, body{body} {}
        virtual ~ActionSpecStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ActionSpecStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 3;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *specType;
            case 1:
                return *templateType;
            case 2:
                return *body;
            }
            return *specType;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;
        virtual void PreCodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetSpecType() const { return *specType; }
        const auto &GetInKeyword() const { return in; }
        const auto &GetTemplateType() const { return *templateType; }
        const auto &GetBody() const { return *body; }
    };

    class EnumIdentifierStatement : public StatementSyntax
    {
    private:
        const Token &identifier;

    public:
        EnumIdentifierStatement(const Token &identifier) : identifier{identifier} {}
        virtual ~EnumIdentifierStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::EnumIdentifierStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 0;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        virtual const Position &GetStart() const override
        {
            return identifier.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return identifier.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetIdentifier() const { return identifier; }
    };

    class EnumStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        BlockStatement<EnumIdentifierStatement> *body;

    public:
        EnumStatement(const Token &keyword,
                      const Token &identifier,
                      BlockStatement<EnumIdentifierStatement> *body) : keyword{keyword}, identifier{identifier}, body{body} {}
        EnumStatement(const Token &keyword,
                      const Token &identifier,
                      const Token &left,
                      const std::vector<EnumIdentifierStatement *> &statements,
                      const Token &right) : keyword{keyword}, identifier{identifier}, body{new BlockStatement<EnumIdentifierStatement>(left, statements, right)} {}
        virtual ~EnumStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::EnumStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *body;
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return body->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetBody() const { return *body; }
    };

    class TypeAliasStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        GenericParameter *generic;
        const Token &eq;
        TypeSyntax *type;

    public:
        TypeAliasStatement(const Token &keyword,
                           const Token &identifier,
                           GenericParameter *generic,
                           const Token &eq,
                           TypeSyntax *type) : keyword{keyword}, identifier{identifier}, generic{generic}, eq{eq}, type{type} {}
        TypeAliasStatement(const Token &keyword,
                           const Token &identifier,
                           GenericParameter *generic) : keyword{keyword}, identifier{identifier}, generic{generic}, eq{TokenNull}, type{nullptr} {}
        virtual ~TypeAliasStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::TypeAliasStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return (generic == nullptr ? 0 : 1) + (type == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (generic)
                    return *generic;
            default:
                return *type;
            }
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            if (type)
                return type->GetEnd();
            else if (generic)
                return generic->GetEnd();
            else
                return identifier.GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetEq() const { return eq; }
        const auto &GetTypeAlias() const { return *type; }
        const bool IsSpecAlias() const { return type == nullptr; }
    };

    class MatchEntry : public SyntaxNode
    {
    private:
        Expression expr;
        const Token &arrow;
        Statement stmt;

    public:
        MatchEntry(Expression expr,
                   const Token &arrow,
                   Statement stmt) : expr{expr}, arrow{arrow}, stmt{stmt}
        {
        }
        virtual ~MatchEntry() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::MatchEntry; }

        virtual const uint8_t NumChildren() const override
        {
            return expr == nullptr ? 1 : 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (expr)
                    return *expr;
            default:
                return *stmt;
            }
        }

        virtual const Position &GetStart() const override
        {
            return expr->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return stmt->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const {}

        const auto &GetExpression() const { return *expr; }
        const auto &GetArrow() const { return arrow; }
        const auto &GetSatement() const { return *stmt; }
        const bool IsElse() const { return expr == nullptr; }
    };

    class MatchExpression : public ExpressionSyntax
    {
    private:
        const Token &keyword;
        Expression expr;
        BlockStatement<MatchEntry> *entries;

    public:
        MatchExpression(const Token &keyword,
                        Expression expr,
                        BlockStatement<MatchEntry> *entries) : keyword{keyword}, expr{expr}, entries{entries}
        {
        }
        virtual ~MatchExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::MatchExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expr;
            default:
                return *entries;
            }
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return entries->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override {}

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expr; }
        const auto &GetEntries() const { return *entries; }
    };

    class ExportDecleration : public StatementSyntax
    {
    private:
        const Token &keyword;
        Statement statement;

    public:
        ExportDecleration(const Token &keyword,
                          Statement statement) : keyword{keyword}, statement{statement}
        {
        }
        virtual ~ExportDecleration() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::ExportDecleration; }

        virtual const uint8_t NumChildren() const override
        {
            return 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *statement;
        }

        virtual const Position &GetStart() const override
        {
            return statement->GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return statement->GetEnd();
        }

        virtual const CodeValue *CodeGen(CodeGeneration &gen) const override;

        const auto &GetKeyword() const { return keyword; }
        const auto &GetStatement() const { return *statement; }
    };

    class Parser
    {
    private:
        enum class Using
        {
            If,
        };

    private:
        const TokenList &tokenList;
        const FileIterator &fptr;
        TokenList::iterator tokenIterator;
        bool keepGoing = true;
        std::bitset<64> usings;

        static ErrorList errors;
        static FileIterator *globalFptr;

    public:
        Parser(const TokenList &tokenList, const FileIterator &fptr) : tokenList{tokenList}, fptr{fptr}, tokenIterator{nullptr} { globalFptr = (FileIterator *)&fptr; }
        ~Parser() {}

        ModuleUnit *ParseModule(const std::string &moduleName);

        Statement ParseStatement();
        Statement ParseTopLevelScopeStatement();
        Statement ParseTemplate();
        Statement ParseTemplateScopeStatement();
        Statement ParseSpec();
        Statement ParseSpecScopeStatement();
        Statement ParseBlockStatement();
        Statement ParseTemplateVariableDecleration();
        Statement ParseActionExpressionBody();
        Statement ParseSpecVariableDecleration();
        Statement ParseVariableDecleration();
        Statement ParseConst();
        Statement ParseConstVariableDecleration(const Token &keyword, const Token &ident);
        Statement ParseFunctionDecleration(const Token &keyword, const Token &ident);
        Statement ParseSpecFunctionDecleration(const Token &keyword, const Token &ident);
        Statement ParseIfStatement();
        Statement ParseElif();
        Statement ParseLoopStatement();
        Statement ParseReturnStatement();
        Statement ParseYieldStatement();
        Statement ParseAction();
        Statement ParseActionBody();
        Statement ParseActionScopeStatement();
        Statement ParseEnum();
        Statement ParseTypeAlias();
        Statement ParseSpecTypeAlias();
        Statement ParseExport();

        GenericParameter *ParseGenericParameter();
        GenericParameterEntry *ParseGenericParameterEntry();
        ObjectInitializer *ParseObjectInitializer();
        ArrayLiteralEntry *ParseArrayLiteralEntry();
        MatchEntry *ParseMatchEntry();

        Expression ParseExpression(uint8_t parentPrecedence = 0, Expression left = nullptr);
        Expression ParsePrimaryExpression();
        Expression ParseLiteral();
        Expression ParseIdentifier();
        Expression ParseFunctionCall(Expression fn);
        Expression ParseAnonymousFunction();
        Expression ParseSubscript(Expression expr);
        Expression ParseTemplateInitializer(const Token &);
        Expression ParseArrayLiteral();
        Expression ParseMatch();

        TypeSyntax *ParseType();

        uint8_t UnaryPrecedence(TokenType type);
        uint8_t PostfixPrecedence(TokenType type);
        uint8_t BinaryPrecedence(TokenType type);
        bool IsBinaryRightAssociative(TokenType type);
        const Token &Expect(TokenType type);
        void PrintNode(const SyntaxNode &node, int index = 0, const std::wstring &indent = L"", bool last = false);
        void RecurseNode(const SyntaxNode &node);

        static auto &GetFptr() { return *globalFptr; }
        static auto &GetErrorList() { return errors; }

        void Use(Using toUse) { usings.set((uint64_t)toUse); }
        bool IsUsed(Using toUse) { return usings.test((uint64_t)toUse); }
        void UnUse(Using toUnUse) { usings.reset((uint64_t)toUnUse); }
    };
} // namespace Parsing

std::ostream &
operator<<(std::ostream &stream, const Parsing::SyntaxType &e);
std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxNode &node);