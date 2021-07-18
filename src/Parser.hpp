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
        virtual const Position &GetEnd() const = 0;
    };

    class TypeExpression : public ExpressionSyntax
    {
    private:
        TypeSyntax *type;

    public:
        TypeExpression(TypeSyntax *type) : type{type} {}
        virtual ~TypeExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::TypeExpression; }

        const virtual uint8_t NumChildren() const override
        {
            return 1;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *type;
        }
    };

    class PrimitiveType : public TypeSyntax
    {
    private:
        const Token token;

    public:
        PrimitiveType(const Token &token) : token{token} {}
        virtual ~PrimitiveType() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::PrimitiveType; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        const virtual Position &GetEnd() const override
        {
            return token.position.end;
        }

        const auto &GetToken() const { return token; }
    };

    class IdentifierType : public TypeSyntax
    {
    private:
        const Token token;

    public:
        IdentifierType(const Token &token) : token{token} {}
        virtual ~IdentifierType() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::IdentifierType; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        const virtual Position &GetEnd() const override
        {
            return token.position.end;
        }

        const auto &GetToken() const { return token; }
    };

    class FunctionType : public TypeSyntax
    {
        const Token left;
        std::vector<TypeSyntax *> parameters;
        const Token right;
        const Token arrow;
        TypeSyntax *retType;

    public:
        FunctionType(const Token &left,
                     const std::vector<TypeSyntax *> &parameters,
                     const Token &right,
                     const Token &arrow,
                     TypeSyntax *retType) : left{left}, parameters{parameters}, right{right}, arrow{arrow}, retType{retType} {}
        virtual ~FunctionType() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::FunctionType; }

        const virtual uint8_t NumChildren() const override
        {
            return parameters.size() + (retType != nullptr ? 1 : 0);
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            if (index < parameters.size())
                return *parameters[index];
            else
                return *retType;
        }

        const virtual Position &GetEnd() const override
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
        const Token token;
        TypeSyntax *type;

    public:
        ReferenceType(const Token &token, TypeSyntax *type);
        virtual ~ReferenceType() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::ReferenceType; }

        const virtual uint8_t NumChildren() const override
        {
            return 1;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *type;
        }

        const virtual Position &GetEnd() const override
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
        const Token left;
        std::vector<TypeSyntax *> arguments;
        const Token right;

    public:
        GenericType(TypeSyntax *type,
                    const Token &left,
                    const std::vector<TypeSyntax *> &arguments,
                    const Token &right) : type{type}, left{left}, arguments{arguments}, right{right} {}
        virtual ~GenericType() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::GenericType; }

        const virtual uint8_t NumChildren() const override
        {
            return arguments.size() + 1;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            if (index == 0)
                return *type;
            else
                return *arguments[index - 1];
        }

        const virtual Position &GetEnd() const override
        {
            return right.position.end;
        }

        const auto &GetBaseType() const { return *type; }
        const auto &GetLeft() const { return left; }
        const auto &GetArguments() const { return arguments; }
        const auto &GetRight() const { return right; }
    };

    class IntegerSyntax : public ExpressionSyntax
    {
    private:
        const Token valueToken;

    public:
        IntegerSyntax(const Token &token) : valueToken{token}
        {
            assert(valueToken.type == TokenType::Integer && "Token should be an integer token");
        }
        virtual ~IntegerSyntax() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::Integer; }

        const uint64_t GetValue() const { return valueToken.ivalue; }
        const std::string &GetRawValue() const { return valueToken.raw; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return valueToken;
        }
    };

    class FloatingSyntax : public ExpressionSyntax
    {
    private:
        const Token valueToken;

    public:
        FloatingSyntax(const Token &token) : valueToken{token}
        {
            assert(valueToken.type == TokenType::Floating && "Token should be an integer token");
        }
        virtual ~FloatingSyntax() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::Integer; }

        const double GetValue() const { return valueToken.fvalue; }
        const std::string &GetRawValue() const { return valueToken.raw; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return valueToken;
        }
    };

    class BooleanSyntax : public ExpressionSyntax
    {
    private:
        const Token boolToken;
        bool value;

    public:
        BooleanSyntax(const Token &token) : boolToken{token}, value{boolToken.type == TokenType::True}
        {
            assert((boolToken.type == TokenType::True || boolToken.type == TokenType::False) && "Token should be an boolean token");
        }
        virtual ~BooleanSyntax() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::Boolean; }

        const auto &GetBoolToken() const { return boolToken; }
        const auto &GetValue() const { return value; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return boolToken;
        }
    };

    class StringSyntax : public ExpressionSyntax
    {
    private:
        const Token token;

    public:
        StringSyntax(const Token &token) : token{token}
        {
            assert(token.type == TokenType::String && "Token should be an boolean token");
        }
        virtual ~StringSyntax() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::String; }

        const auto &GetToken() const { return token; }
        const auto &GetValue() const { return token.raw; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *this;
        }
    };

    class BinaryExpression : public ExpressionSyntax
    {
    private:
        Expression LHS;
        Expression RHS;
        const Token op;

    public:
        BinaryExpression(ExpressionSyntax *LHS, ExpressionSyntax *RHS, const Token &op) : LHS{LHS}, RHS{RHS}, op{op}
        {
        }
        virtual ~BinaryExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::BinaryExpression; }

        const auto GetLHS() const { return LHS; }
        const auto GetRHS() const { return RHS; }
        const auto GetOperator() const { return op; }

        const virtual uint8_t NumChildren() const override
        {
            return 2;
        }

        const virtual SyntaxNode &operator[](int index) const override
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
    };

    class UnaryExpression : public ExpressionSyntax
    {
    private:
        ExpressionSyntax *expression;
        const Token op;

    public:
        UnaryExpression(ExpressionSyntax *expression, const Token &op) : expression{expression}, op{op}
        {
        }
        virtual ~UnaryExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::UnaryExpression; }

        const auto GetExpression() const { return expression; }
        const auto GetOperator() const { return op; }

        const virtual uint8_t NumChildren() const override
        {
            return 1;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *expression;
            }
            return *this;
        }
    };

    class IdentifierExpression : public ExpressionSyntax
    {
    private:
        const Token &identifierToken;

    public:
        IdentifierExpression(const Token &identifierToken) : identifierToken{identifierToken} {}
        virtual ~IdentifierExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::IdentifierExpression; }

        const virtual uint8_t NumChildren() const override
        {
            return 0;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *this;
        }

        const auto &GetIdentiferToken() const { return identifierToken; }
    };

    class CallExpression : public ExpressionSyntax
    {
    private:
        const Token functionName;
        const Token leftParen;
        const Token rightParen;
        std::vector<Expression> arguments;

    public:
        CallExpression(const Token &functionName, const Token &leftParen,
                       const Token &rightParen, std::vector<Expression> arguments) : functionName{functionName}, leftParen{leftParen}, rightParen{rightParen}, arguments{arguments}
        {
        }
        virtual ~CallExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::CallExpression; }

        const virtual uint8_t NumChildren() const override
        {
            return arguments.size();
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *arguments[index];
        }

        const auto &GetFunctionNameToken() const { return functionName; }
        const auto &GetFunctionName() const { return functionName.raw; }
        const auto &GetFunctionArgs() const { return arguments; }
    };

    class CastExpression : public ExpressionSyntax
    {
    private:
        Expression LHS;
        const Token keyword;
        TypeSyntax *type;

    public:
        CastExpression(Expression LHS,
                       const Token &keyword,
                       TypeSyntax *type) : LHS{LHS}, keyword{keyword}, type{type}
        {
        }
        virtual ~CastExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::CastExpression; }

        const virtual uint8_t NumChildren() const override
        {
            return 2;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            switch (index)
            {
            case 0:
                return *LHS;
            default:
                return *type;
            }
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

    class BlockStatement : public StatementSyntax
    {
    private:
        std::vector<Statement> statements;

    public:
        BlockStatement(const std::vector<Statement> &statements) : statements{statements} {}
        virtual ~BlockStatement() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::BlockStatement; }

        const virtual uint8_t NumChildren() const override
        {
            return statements.size();
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *statements[index];
        }
    };

    class ExpressionStatement : public StatementSyntax
    {
    private:
        Expression expression;

    public:
        ExpressionStatement(Expression expression) : expression{expression} {}
        virtual ~ExpressionStatement() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::ExpressionStatement; }

        const virtual uint8_t NumChildren() const override
        {
            return 1;
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            return *expression;
        }
    };

    class VariableDeclerationStatement : public StatementSyntax
    {
    private:
        const Token keyword;
        const Token identifier;
        TypeSyntax *type;
        Expression initializer;

    public:
        VariableDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     TypeSyntax *type = nullptr, Expression initializer = nullptr) : keyword{keyword}, identifier{identifier}, type{type}, initializer{initializer} {}
        virtual ~VariableDeclerationStatement() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::VariableDeclerationStatement; }

        const virtual uint8_t NumChildren() const override
        {
            return (type == nullptr ? 0 : 1) + (initializer == nullptr ? 0 : 1);
        }

        const virtual SyntaxNode &operator[](int index) const override
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

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto GetVariableType() const { return type; }
        bool HasInitializer() const { return initializer != nullptr; }
    };

    class FunctionDeclerationStatement : public StatementSyntax
    {
    private:
        const Token keyword;
        const Token identifier;
        const Token left;
        std::vector<VariableDeclerationStatement *> parameters;
        const Token right;
        const Token arrow;
        TypeSyntax *retType;
        Statement body;

    public:
        FunctionDeclerationStatement(const Token &keyword,
                                     const Token &identifier,
                                     const Token &left,
                                     const std::vector<VariableDeclerationStatement *> &parameters,
                                     const Token &right,
                                     const Token &arrow,
                                     TypeSyntax *retType,
                                     Statement body) : keyword{keyword}, identifier{identifier}, left{left}, parameters{parameters}, right{right}, arrow{arrow}, retType{retType}, body{body} {}
        virtual ~FunctionDeclerationStatement() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::FunctionDeclerationStatement; }

        const virtual uint8_t NumChildren() const override
        {
            return parameters.size() + (body == nullptr ? 0 : 1) + (retType == nullptr ? 0 : 1);
        }

        const virtual SyntaxNode &operator[](int index) const override
        {
            if (index < parameters.size())
                return *parameters[index];
            else if (index == parameters.size() && body)
                return *body;
            else if (index == parameters.size() && retType)
                return *retType;
            else if (index == parameters.size() + 1 && body)
                return *retType;
            return *this;
        }

        const auto &GetKeyword() const { return keyword; }
        const auto &GetIdentifier() const { return identifier; }
        const auto &GetLeftParen() const { return left; }
        const auto &GetParameters() const { return parameters; }
        const auto &GetRightParen() const { return right; }
        const auto &GetFuncArrow() const { return arrow; }
        const auto &GetRetType() const { return retType; }
        const auto &GetBody() const { return body; }
    };

    class ModuleUnit
    {
    private:
        BlockStatement syntaxTree;

    public:
        ModuleUnit(const std::vector<Statement> statements) : syntaxTree{statements} {}

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
        Statement ParseBlockStatement();
        Statement ParseVariableDecleration();
        Statement ParseConst();
        Statement ParseConstVariableDecleration(const Token &keyword, const Token &ident);
        Statement ParseFunctionDecleration(const Token &keyword, const Token &ident);

        Expression ParseExpression(uint8_t parentPrecedence = 0, Expression left = nullptr);
        Expression ParsePrimaryExpression();
        Expression ParseLiteral();
        Expression ParseIdentifier();
        Expression ParseFunctionCall(const Token &);

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

std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxType &e);
std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxNode &node);