#pragma once

#include <stdint.h>
#include <Token.hpp>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace Parsing
{

    class ExpressionSyntax : public SyntaxNode
    {
    public:
        virtual ~ExpressionSyntax() {}
    };

    using Expression = ExpressionSyntax *;

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

    class BinaryExpression : public ExpressionSyntax
    {
    private:
        ExpressionSyntax *LHS;
        ExpressionSyntax *RHS;
        const Token &op;

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
        }
    };

    class UnaryExpression : public ExpressionSyntax
    {
    private:
        ExpressionSyntax *expression;
        const Token &op;

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

    class AssignmentExpression : public ExpressionSyntax
    {
    private:
        Expression LHS;
        const Token &op;
        Expression RHS;

    public:
        AssignmentExpression(Expression LHS, const Token &op, Expression RHS) : LHS{LHS}, op{op}, RHS{RHS} {}
        virtual ~AssignmentExpression() {}

        const virtual SyntaxType GetType() const override { return SyntaxType::AssignmentExpression; }

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
            case 2:
                return *RHS;
            }
        }

        const auto &GetLHS() const { return *LHS; }
        const auto &GetOperator() const { return op; }
        const auto &GetRHS() const { return *RHS; }
    };

    class Parser
    {
    private:
        const TokenList &tokenList;
        const FileIterator &fptr;
        TokenList::iterator tokenIterator;

    public:
        Parser(const TokenList &tokenList, const FileIterator &fptr) : tokenList{tokenList}, fptr{fptr}, tokenIterator{nullptr} {}
        ~Parser() {}

        void Parse();
        Expression ParseExpression(uint8_t parentPrecedence = 0);
        Expression ParsePrimaryExpression();
        Expression ParseLiteral();
        Expression ParseIdentifier();
        Expression ParseFunctionCall(const Token &);

        uint8_t UnaryPrecedence(TokenType type);
        uint8_t BinaryPrecedence(TokenType type);
        bool IsBinaryRightAssociative(TokenType type);
        const Token &Expect(TokenType type);
        void PrintNode(const SyntaxNode &node, int index = 0, const std::wstring &indent = L"", bool last = false);
        void RecurseNode(const SyntaxNode &node);
    };
} // namespace Parsing

std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxType &e);
std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxNode &node);