#include <Parser.hpp>
#include <Log.hpp>
#include <Trie.hpp>
#include <io.h>
#include <fcntl.h>

namespace Parsing
{

    void Parser::Parse()
    {
        tokenIterator = tokenList.begin();
        while (tokenIterator->type != TokenType::Eof)
        {
            auto node = ParseExpression();
            PrintNode(*node);
        }
    }

    Expression Parser::ParseExpression(uint8_t parentPrecedence)
    {
        Expression left;
        auto unaryPrecedence = UnaryPrecedence(tokenIterator->type);
        if (unaryPrecedence != 0 && unaryPrecedence >= parentPrecedence)
        {
            auto op = tokenIterator++;
            auto right = ParseExpression(unaryPrecedence);
            left = new UnaryExpression(right, op);
        }
        else
        {
            left = ParsePrimaryExpression();
        }

        if (!left)
        {
            Logging::Error(color::bold(color::white("Unexpected token `{}` in expression!")), tokenIterator->raw.c_str());
            Logging::CharacterSnippet(fptr, tokenIterator->position);
            return nullptr;
        }

        while (true)
        {
            auto precedence = BinaryPrecedence(tokenIterator->type);
            auto isRight = IsBinaryRightAssociative(tokenIterator->type);
            if (precedence == 0 || precedence <= parentPrecedence)
                break;

            auto op = tokenIterator++;
            auto right = ParseExpression(precedence);
            if (isRight)
            {
                
                left = new BinaryExpression(left, right, op);
            }
            else
            {
                left = new BinaryExpression(left, right, op);
            }
        }
        return left;
    }

    Expression Parser::ParsePrimaryExpression()
    {
        if (tokenIterator->type == TokenType::LeftParen)
        {
            const Token &left = tokenIterator++;
            auto expression = ParseExpression();
            const Token &right = Expect(TokenType::RightParen);
            if (right == TokenNull)
            {
                return nullptr;
            }

            return expression;
        }
        return ParseLiteral();
    }

    Expression Parser::ParseLiteral()
    {
        switch (tokenIterator->type)
        {
        case TokenType::Integer:
        {
            const Token &token = tokenIterator++;
            return new IntegerSyntax(token);
        }
        case TokenType::Identifier:
        {
            return ParseIdentifier();
        }
        // case TokenType::Floating:
        //     return std::move(std::make_unique<IntegerSyntax>(tokenIterator++));
        default:
            break;
        }
        return nullptr;
    }

    Expression Parser::ParseIdentifier()
    {
        const Token &token = tokenIterator++;
        if (tokenIterator->type == TokenType::LeftParen)
            return ParseFunctionCall(token);
        else
            return new IdentifierExpression(token);
    }

    Expression Parser::ParseFunctionCall(const Token &identifier)
    {
        auto &left = Expect(TokenType::LeftParen);
        std::vector<Parsing::Expression> args;

        while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
        {
            args.push_back(ParseExpression());
            if (tokenIterator->type == TokenType::Comma)
                tokenIterator++;
        }

        auto &right = Expect(TokenType::RightParen);
        return new CallExpression(identifier, left, right, args);
    }

    uint8_t Parser::UnaryPrecedence(TokenType type)
    {
        switch (type)
        {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::DoublePlus:
        case TokenType::DoubleMinus:
        case TokenType::Not:
        case TokenType::Tilda:
        case TokenType::Star:
        case TokenType::Ampersand:
            return 14;
        default:
            return 0;
        }
    }

    uint8_t Parser::BinaryPrecedence(TokenType type)
    {
        switch (type)
        {
        // case TokenType::Comma:
        //     return 1;
        case TokenType::Equal:
        case TokenType::PlusEqual:
        case TokenType::MinusEqual:
        case TokenType::StarEqual:
        case TokenType::SlashEqual:
        case TokenType::PercentEqual:
        case TokenType::LeftShiftEquals:
        case TokenType::RightShiftEquals:
        case TokenType::TripleShiftEquals:
        case TokenType::AmpersandEquals:
        case TokenType::CarrotEquals:
        case TokenType::PipeEquals:
            return 2;
        case TokenType::Or:
            return 4;
        case TokenType::And:
            return 5;
        case TokenType::Pipe:
            return 6;
        case TokenType::Carrot:
            return 7;
        case TokenType::Ampersand:
            return 8;
        case TokenType::DoubleEqual:
        case TokenType::NotEqual:
            return 9;
        case TokenType::LeftAngle:
        case TokenType::SmallerEqual:
        case TokenType::RightAngle:
        case TokenType::BiggerEqual:
        case TokenType::NotSmaller:
        case TokenType::NotBigger:
            return 10;
        case TokenType::LeftShift:
        case TokenType::RightShift:
        case TokenType::TripleShift:
            return 11;
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Percent:
            return 12;
        case TokenType::Star:
        case TokenType::ForwardSlash:
            return 13;
        case TokenType::Dot:
            return 15;
        default:
            return 0;
        }
    }

    bool Parser::IsBinaryRightAssociative(TokenType type)
    {
        switch (type)
        {
        case TokenType::Equal:
        case TokenType::StarEqual:
        case TokenType::PlusEqual:
        case TokenType::MinusEqual:
        case TokenType::SlashEqual:
        case TokenType::PercentEqual:
        case TokenType::LeftShiftEquals:
        case TokenType::RightShiftEquals:
        case TokenType::TripleShiftEquals:
        case TokenType::AmpersandEquals:
        case TokenType::CarrotEquals:
        case TokenType::PipeEquals:
        case TokenType::Typeof:
        case TokenType::Tilda:
        case TokenType::Not:
            return true;
        default:
            return false;
        }
    }

    const Token &Parser::Expect(TokenType type)
    {
        if (tokenIterator->type != type)
        {
            Logging::Error(color::bold(color::white("Unexpected token {}. Expected {}")), tokenIterator->raw.c_str(), TokenTypeString(type));
            Logging::CharacterSnippet(fptr, tokenIterator->position);
            return TokenNull;
        }
        else
        {
            const auto &ret = *tokenIterator;
            ++tokenIterator;
            return ret;
        }
    }

    void Parser::PrintNode(const SyntaxNode &node, int index, const std::wstring &indent, bool last)
    {
        _setmode(_fileno(stdout), _O_U16TEXT);
        std::wcout << indent;
        if (index != 0)
        {
            std::wcout << (last ? L"└── " : L"├── ");
        }
        _setmode(_fileno(stdout), _O_TEXT);

        std::cout << node << std::endl;

        std::wstring nindent = indent + (index == 0 ? L"" : (last ? L"    " : L"│   "));
        for (int i = 0; i < node.NumChildren(); i++)
        {
            PrintNode(node[i], index + 1, nindent, i == node.NumChildren() - 1);
        }
    }

    void Parser::RecurseNode(const SyntaxNode &node)
    {
        std::cout << node << std::endl;

        for (int i = 0; i < node.NumChildren(); i++)
        {
            RecurseNode(node[i]);
        }
    }
} // namespace Parsing

std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxType &e)
{

#define ets(type)                   \
    case Parsing::SyntaxType::type: \
        stream << #type;            \
        break;
    switch (e)
    {
        ets(Integer);
        ets(BinaryExpression);
        ets(UnaryExpression);
        ets(CallExpression);
        ets(IdentifierExpression);
        ets(None);
    }
#undef ets
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const Parsing::SyntaxNode &node)
{
    stream << node.GetType();
    try
    {
        switch (node.GetType())
        {
        case Parsing::SyntaxType::BinaryExpression:
        {
            stream << " " << node.As<Parsing::BinaryExpression>().GetOperator().type;
            break;
        }
        case Parsing::SyntaxType::UnaryExpression:
        {
            stream << " " << node.As<Parsing::UnaryExpression>().GetOperator().type;
            break;
        }
        case Parsing::SyntaxType::Integer:
        {
            stream << " " << node.As<Parsing::IntegerSyntax>().GetRawValue();
            break;
        }
        case Parsing::SyntaxType::CallExpression:
        {
            stream << " " << node.As<Parsing::CallExpression>().GetFunctionName();
            break;
        }
        case Parsing::SyntaxType::IdentifierExpression:
        {
            stream << " `" << node.As<Parsing::IdentifierExpression>().GetIdentiferToken().raw << "`";
            break;
        }
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