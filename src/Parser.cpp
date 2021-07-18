#include <Parser.hpp>
#include <Trie.hpp>
// #include <io.h>
// #include <fcntl.h>
#include <codecvt>

namespace Parsing
{
    ReferenceType::ReferenceType(const Token &token, TypeSyntax *type) : token{token}, type{type}
    {
        if (type->GetType() == SyntaxType::ReferenceType)
        {
            Logging::Error(color::bold(color::white("A reference cannot reference a reference!")));
            Logging::CharacterSnippet(Parser::GetFptr(), TokenPosition(token.position.start, type->As<ReferenceType>().token.position.end));
        }
    }

    FileIterator *Parser::globalFptr = nullptr;

    ModuleUnit *Parser::ParseModule()
    {
        tokenIterator = tokenList.begin();
        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::Eof)
        {
            auto node = ParseStatement();

            if (node)
                statements.push_back(node);
            else
                tokenIterator++;
        }
        auto module = new ModuleUnit(statements);
        PrintNode(module->GetSyntaxTree());
        return module;
    }

    Statement Parser::ParseStatement()
    {
        switch (tokenIterator->type)
        {
        case TokenType::LeftCurly:
            return ParseBlockStatement();
        case TokenType::Let:
            return ParseVariableDecleration();
        case TokenType::Const:
        {

            auto c = ParseConst();
            return c;
        }
        default:
            auto expr = ParseExpression();
            if (expr)
                return new ExpressionStatement(expr);
        }
        return nullptr;
    }

    Statement Parser::ParseBlockStatement()
    {
        auto left = tokenIterator++;

        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            statements.push_back(ParseStatement());
        }
        auto right = Expect(TokenType::RightCurly);

        return new BlockStatement(statements);
    }

    Statement Parser::ParseVariableDecleration()
    {
        auto let = Expect(TokenType::Let);
        auto ident = tokenIterator++;
        TypeSyntax *type = nullptr;
        Expression initializer = nullptr;
        if (tokenIterator->type == TokenType::Colon)
        {
            auto colon = tokenIterator++;
            type = ParseType();
            // return new VariableDeclerationStatement(let, ident, type);
        }
        if (tokenIterator->type == TokenType::Equal)
        {
            auto equal = tokenIterator++;
            initializer = ParseExpression();
        }
        return new VariableDeclerationStatement(let, ident, type, initializer);
    }

    Statement Parser::ParseConst()
    {
        auto keyword = Expect(TokenType::Const);
        auto ident = Expect(TokenType::Identifier);
        switch (tokenIterator->type)
        {
        case TokenType::LeftParen:

        {
            auto c = ParseFunctionDecleration(keyword, ident);
            return c;
            break;
        }
        case TokenType::Equal:

        {
            auto c = ParseConstVariableDecleration(keyword, ident);
            return c;
            break;
        }
        default:
            Logging::Error(color::bold(color::white("Expecting constant variable initilization or function!")));
            Logging::CharacterSnippet(fptr, TokenPosition(tokenIterator->position.start, tokenIterator->position.end));
            return nullptr;
        }
        return nullptr;
    }

    Statement Parser::ParseConstVariableDecleration(const Token &keyword, const Token &ident)
    {
        TypeSyntax *type = nullptr;
        Expression initializer = nullptr;
        if (tokenIterator->type == TokenType::Colon)
        {
            auto colon = tokenIterator++;
            type = ParseType();
        }

        auto equal = Expect(TokenType::Equal);
        initializer = ParseExpression();

        return new VariableDeclerationStatement(keyword, ident, type, initializer);
    }

    Statement Parser::ParseFunctionDecleration(const Token &keyword, const Token &ident)
    {
        auto left = tokenIterator++;
        std::vector<VariableDeclerationStatement *> parameters;
        bool defaultInit = false;
        while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
        {
            auto var = dynamic_cast<VariableDeclerationStatement *>(ParseVariableDecleration());
            if (!defaultInit && var->HasInitializer())
                defaultInit = true;
            else if (defaultInit && !var->HasInitializer())
            {
                Logging::Error(color::bold(color::white("All parameters after one defualt parameter bust be default as well!")));
                Position end = var->GetIdentifier().position.end;
                if (var->GetVariableType())
                    end = var->GetVariableType()->GetEnd();
                Logging::CharacterSnippet(fptr, TokenPosition(var->GetKeyword().position.start, end));
                return nullptr;
            }
            parameters.push_back(var);
            if (tokenIterator->type == TokenType::Comma)
                tokenIterator++;
        }
        auto right = Expect(TokenType::RightParen);
        auto arrow = Expect(TokenType::FuncArrow);
        auto type = ParseType();
        auto body = ParseStatement();
        return new FunctionDeclerationStatement(keyword, ident, left, parameters, right, arrow, type, body);
    }

    Expression Parser::ParseExpression(uint8_t parentPrecedence, Expression left)
    {
        if (left == nullptr)
        {
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
        }

        if (!left)
        {
            Logging::Error(color::bold(color::white("Unexpected token `{}` in expression!")), tokenIterator->raw.c_str());
            Logging::CharacterSnippet(fptr, tokenIterator->position);
            return nullptr;
        }

        uint8_t precedence = 0;
        while ((precedence = BinaryPrecedence(tokenIterator->type)) > parentPrecedence && precedence > 0)
        {
            auto op = tokenIterator++;
            if (op->type == TokenType::As)
            {
                auto right = ParseType();
                if (right)
                {
                    left = new CastExpression(left, op, right);
                    continue;
                }
                else
                {
                    Logging::Error(color::bold(color::white("'{}' is not a type")), tokenIterator->raw.c_str());
                    Logging::CharacterSnippet(fptr, TokenPosition(tokenIterator->position.start, tokenIterator->position.end));
                    return nullptr;
                }
            }
            auto right = ParseExpression(precedence);

            while (
                BinaryPrecedence(tokenIterator->type) > precedence ||
                (IsBinaryRightAssociative(tokenIterator->type) &&
                 BinaryPrecedence(tokenIterator->type) == precedence))
            {
                right = ParseExpression(parentPrecedence + 1, right);
            }

            left = new BinaryExpression(left, right, op);
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
        case TokenType::Floating:
        {
            const Token &token = tokenIterator++;
            return new FloatingSyntax(token);
        }
        case TokenType::True:
        case TokenType::False:
        {
            const Token &token = tokenIterator++;
            return new BooleanSyntax(token);
        }
        case TokenType::String:
        {
            const Token &token = tokenIterator++;
            return new StringSyntax(token);
        }
        case TokenType::Identifier:
        {
            return ParseIdentifier();
        }
        default:
            // auto type = ParseType();
            // if (type)
            //     return new TypeExpression(type);
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

    TypeSyntax *Parser::ParseType()
    {
        TypeSyntax *baseType;
        switch (tokenIterator->type)
        {
        case TokenType::Identifier:
            baseType = new IdentifierType(tokenIterator++);
            break;
        case TokenType::Int:
        case TokenType::Uint:
        case TokenType::Bool:
        case TokenType::Float:
        case TokenType::Char:
            baseType = new PrimitiveType(tokenIterator++);
            break;
        case TokenType::LeftParen:
        {
            auto left = tokenIterator++;
            std::vector<TypeSyntax *> parameters;

            while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
            {
                auto nextType = ParseType();
                if (!nextType)
                {
                    Logging::Error(color::bold(color::white("'{}' is not a type")), tokenIterator->raw.c_str());
                    Logging::CharacterSnippet(fptr, TokenPosition(tokenIterator->position.start, tokenIterator->position.end));
                    tokenIterator++;
                    return nullptr;
                }
                parameters.push_back(nextType);
                if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            auto right = Expect(TokenType::RightParen);
            auto arrow = Expect(TokenType::FuncArrow);
            auto retType = ParseType();
            baseType = new FunctionType(left, parameters, right, arrow, retType);
            break;
        }
        case TokenType::Ampersand:
        {
            auto amp = tokenIterator++;
            auto baseType = ParseType();
            if (baseType)
                baseType = new ReferenceType(amp, baseType);
            break;
        }
        default:
            return nullptr;
        }
        if (tokenIterator->type == TokenType::LeftAngle)
        {
            auto left = tokenIterator++;
            std::vector<TypeSyntax *> arguments;

            while (tokenIterator->type != TokenType::RightAngle && tokenIterator->type != TokenType::Eof)
            {
                auto nextType = ParseType();
                if (!nextType)
                {
                    Logging::Error(color::bold(color::white("'{}' is not a type")), tokenIterator->raw.c_str());
                    Logging::CharacterSnippet(fptr, TokenPosition(tokenIterator->position.start, tokenIterator->position.end));
                    tokenIterator++;
                    return nullptr;
                }
                arguments.push_back(nextType);
                if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            auto right = Expect(TokenType::RightAngle);
            baseType = new GenericType(baseType, left, arguments, right);
        }
        return baseType;
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
        // case TokenType::Star:
        // case TokenType::Ampersand:
        case TokenType::Typeof:
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
        case TokenType::As:
            return 14;
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
            // case TokenType::As:
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
        // _setmode(_fileno(stdout), _O_U16TEXT);
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        std::cout << convert.to_bytes(indent);

        if (index != 0)
        {
            std::cout << convert.to_bytes((last ? L"└── " : L"├── "));
            // std::wcout << (last ? L"└── " : L"├── ");
        }
        // _setmode(_fileno(stdout), _O_TEXT);

        std::cout << node << std::endl;

        std::wstring nindent = indent + (index == 0 ? L"" : (last ? L"    " : L"│   "));
        auto len = node.NumChildren();
        for (int i = 0; i < len; i++)
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
        ets(Floating);
        ets(Boolean);
        ets(String);
        ets(BinaryExpression);
        ets(UnaryExpression);
        ets(CallExpression);
        ets(IdentifierExpression);
        ets(CastExpression);

        ets(BlockStatement);
        ets(ExpressionStatement);
        ets(VariableDeclerationStatement);
        ets(FunctionDeclerationStatement);

        ets(TypeExpression);
        ets(PrimitiveType);
        ets(IdentifierType);
        ets(FunctionType);
        ets(ReferenceType);
        ets(GenericType);

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
        case Parsing::SyntaxType::Floating:
        {
            stream << " " << node.As<Parsing::FloatingSyntax>().GetValue();
            break;
        }
        case Parsing::SyntaxType::Boolean:
        {
            stream << " " << (node.As<Parsing::BooleanSyntax>().GetValue() ? "true" : "false");
            break;
        }
        case Parsing::SyntaxType::String:
        {
            stream << " " << node.As<Parsing::StringSyntax>().GetValue();
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
        case Parsing::SyntaxType::VariableDeclerationStatement:
        {
            stream << " `" << node.As<Parsing::VariableDeclerationStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::FunctionDeclerationStatement:
        {
            stream << " `" << node.As<Parsing::FunctionDeclerationStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::PrimitiveType:
        {
            auto ptoken = node.As<Parsing::PrimitiveType>().GetToken();
            stream << " `" << ptoken.raw << "`";
            if (ptoken.ivalue > 0)
                stream << " size: " << ptoken.ivalue << "";
            break;
        }
        case Parsing::SyntaxType::IdentifierType:
        {
            stream << " `" << node.As<Parsing::IdentifierType>().GetToken().raw << "`";
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