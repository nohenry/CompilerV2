#pragma once

#include <Log.hpp>
#include <stdint.h>
#include <Token.hpp>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

// extern FileIterator *globalFptr;

namespace Parsing
{
    class ExpressionSyntax : public SyntaxNode
    {
    public:
        virtual ~ExpressionSyntax() {}
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

        const auto &GetBaseType() const { return *type; }
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

        const auto &GetIdentifier() const { return identifier; }
        const auto &GetBody() const { return *body; }
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

        const auto &GetIdentiferToken() const { return identifierToken; }
    };

    class CallExpression : public ExpressionSyntax
    {
    private:
        const Token &functionName;
        const Token &leftParen;
        const Token &rightParen;
        std::vector<Expression> arguments;

    public:
        CallExpression(const Token &functionName, const Token &leftParen,
                       const Token &rightParen, std::vector<Expression> arguments) : functionName{functionName}, leftParen{leftParen}, rightParen{rightParen}, arguments{arguments}
        {
        }
        virtual ~CallExpression() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::CallExpression; }

        virtual const uint8_t NumChildren() const override
        {
            return arguments.size();
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            return *arguments[index];
        }

        virtual const Position &GetStart() const override
        {
            return functionName.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return arguments.back()->GetEnd();
        }

        const auto &GetFunctionNameToken() const { return functionName; }
        const auto &GetFunctionName() const { return functionName.raw; }
        const auto &GetFunctionArgs() const { return arguments; }
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

        const auto &GetExpression() const { return *LHS; }
        const auto &GetKeyword() const { return keyword; }
        const auto &GetCastType() const { return *type; }
    };

    class StatementSyntax : public SyntaxNode
    {
    public:
        virtual ~StatementSyntax() {}
    };

    using Statement = StatementSyntax *;

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

    class TemplateStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        GenericParameter *generic;
        const Token &open;
        std::vector<Statement> statements;
        const Token &close;

    public:
        TemplateStatement(const Token &keyword,
                          const Token &identifier,
                          GenericParameter *generic,
                          const Token &open,
                          const std::vector<Statement> &statements,
                          const Token &close) : keyword{keyword}, identifier{identifier}, generic{generic}, open{open}, statements{statements}, close{close} {}
        virtual ~TemplateStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::TemplateStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return statements.size() + generic == nullptr ? 0 : 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (generic)
                    return *generic;
                else
                    return *statements[index];
            default:
                if (generic)
                    return *statements[index - 1];
                else
                    return *statements[index];
                break;
            }
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return close.GetStart();
        }

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetOpen() const { return open; }
        const auto &GetStatements() const { return statements; }
        const auto &GetClose() const { return close; }
    };

    class SpecStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        GenericParameter *generic;
        const Token &open;
        std::vector<Statement> statements;
        const Token &close;

    public:
        SpecStatement(const Token &keyword,
                      const Token &identifier,
                      GenericParameter *generic,
                      const Token &open,
                      const std::vector<Statement> &statements,
                      const Token &close) : keyword{keyword}, identifier{identifier}, generic{generic}, open{open}, statements{statements}, close{close} {}
        virtual ~SpecStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::SpecStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return statements.size() + generic == nullptr ? 0 : 1;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (generic)
                    return *generic;
                else
                    return *statements[index];
            default:
                if (generic)
                    return *statements[index - 1];
                else
                    return *statements[index];
                break;
            }
        }

        virtual const Position &GetStart() const override
        {
            return keyword.GetStart();
        }

        virtual const Position &GetEnd() const override
        {
            return close.GetStart();
        }

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetOpen() const { return open; }
        const auto &GetStatements() const { return statements; }
        const auto &GetClose() const { return close; }
    };

    class BlockStatement : public StatementSyntax
    {
    private:
        const Token &open;
        std::vector<Statement> statements;
        const Token &close;

    public:
        BlockStatement(const Token &open,
                       const std::vector<Statement> &statements,
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

        const auto &GetOpen() const { return open; }
        const auto &GetStatements() const { return statements; }
        const auto &GetClose() const { return close; }
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
    };

    class VariableDeclerationStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        const Token &identifier;
        TypeSyntax *type;
        Expression initializer;

    public:
        VariableDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     TypeSyntax *type = nullptr, Expression initializer = nullptr) : keyword{keyword}, identifier{identifier}, type{type}, initializer{initializer} {}
        virtual ~VariableDeclerationStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::VariableDeclerationStatement; }

        virtual const uint8_t NumChildren() const override
        {
            return (type == nullptr ? 0 : 1) + (initializer == nullptr ? 0 : 1);
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                if (type)
                    return *type;
                else
                    return *initializer;
            case 1:
                return *initializer;
            }
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

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto GetVariableType() const { return type; }
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
                                     const Token &arrow,
                                     TypeSyntax *retType,
                                     Statement body) : keyword{keyword}, identifier{identifier}, generic{generic}, left{left}, parameters{parameters}, right{right}, arrow{arrow}, retType{retType}, body{body} {}
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
            else if (inindex >= pIndex && inindex < rIndex)
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

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetGeneric() const { return *generic; }
        const auto &GetLeftParen() const { return left; }
        const auto &GetParameters() const { return parameters; }
        const auto &GetRightParen() const { return right; }
        const auto &GetFuncArrow() const { return arrow; }
        const auto &GetRetType() const { return retType; }
        const auto &GetBody() const { return body; }
    };

    class IfStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        Expression expression;
        Statement body;

    public:
        IfStatement(const Token &keyword,
                    Expression expression,
                    Statement body) : keyword{keyword}, expression{expression}, body{body} {}
        virtual ~IfStatement() {}

        virtual const SyntaxType GetType() const override { return SyntaxType::IfStatement; }

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

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expression; }
        const auto &GetBody() const { return *body; }
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
            return 2;
        }

        virtual const SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expression;
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

        const auto &GetKeyword() const { return keyword; }
        const auto &GetExpression() const { return *expression; }
    };

    class ActionBaseStatement : public StatementSyntax
    {
    private:
        const Token &keyword;
        TypeSyntax *templateType;
        BlockStatement *body;

    public:
        ActionBaseStatement(const Token &keyword,
                            TypeSyntax *templateType,
                            BlockStatement *body) : keyword{keyword}, templateType{templateType}, body{body} {}
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
        BlockStatement *body;

    public:
        ActionSpecStatement(const Token &keyword,
                            TypeSyntax *specType,
                            const Token &in,
                            TypeSyntax *templateType,
                            BlockStatement *body) : keyword{keyword}, specType{specType}, in{in}, templateType{templateType}, body{body} {}
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

        const auto &GetKeyword() const { return keyword; }
        const auto &GetSpecType() const { return *specType; }
        const auto &GetInKeyword() const { return in; }
        const auto &GetTemplateType() const { return *templateType; }
        const auto &GetBody() const { return *body; }
    };

    class ModuleUnit
    {
    private:
        BlockStatement syntaxTree;

    public:
        ModuleUnit(const Token &start, const std::vector<Statement> statements, const Token &eof) : syntaxTree{start, statements, eof} {}

        const auto &GetSyntaxTree() const { return syntaxTree; }
    };

    class Parser
    {
    private:
        const TokenList &tokenList;
        const FileIterator &fptr;
        TokenList::iterator tokenIterator;

        static FileIterator *globalFptr;

    public:
        Parser(const TokenList &tokenList, const FileIterator &fptr) : tokenList{tokenList}, fptr{fptr}, tokenIterator{nullptr} { globalFptr = (FileIterator *)&fptr; }
        ~Parser() {}

        ModuleUnit *ParseModule();

        Statement ParseStatement();
        Statement ParseTopLevelScopeStatement();
        Statement ParseTemplate();
        Statement ParseTemplateScopeStatement();
        Statement ParseSpec();
        Statement ParseSpecScopeStatement();
        Statement ParseBlockStatement();
        Statement ParseVariableDecleration();
        Statement ParseConst();
        Statement ParseConstVariableDecleration(const Token &keyword, const Token &ident);
        Statement ParseFunctionDecleration(const Token &keyword, const Token &ident);
        Statement ParseSpecFunctionDecleration(const Token &keyword, const Token &ident);
        Statement ParseIfStatement();
        Statement ParseLoopStatement();
        Statement ParseReturnStatement();
        Statement ParseYieldStatement();
        Statement ParseAction();
        Statement ParseActionBody();
        Statement ParseActionScopeStatement();

        GenericParameter *ParseGenericParameter();
        GenericParameterEntry *ParseGenericParameterEntry();
        ObjectInitializer *ParseObjectInitializer();

        Expression ParseExpression(uint8_t parentPrecedence = 0, Expression left = nullptr);
        Expression ParsePrimaryExpression();
        Expression ParseLiteral();
        Expression ParseIdentifier();
        Expression ParseFunctionCall(const Token &);
        Expression ParseTemplateInitializer(const Token &);

        TypeSyntax *ParseType();

        uint8_t UnaryPrecedence(TokenType type);
        uint8_t BinaryPrecedence(TokenType type);
        bool IsBinaryRightAssociative(TokenType type);
        const Token &Expect(TokenType type);
        void PrintNode(const SyntaxNode &node, int index = 0, const std::wstring &indent = L"", bool last = false);
        void RecurseNode(const SyntaxNode &node);

        static auto &GetFptr() { return *globalFptr; }
    };
} // namespace Parsing

std::ostream &
operator<<(std::ostream &stream, const Parsing::SyntaxType &e);
std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxNode &node);