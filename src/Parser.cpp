#include <Parser.hpp>
#include <Trie.hpp>
#ifdef PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
#endif
#include <codecvt>
#include <Errors.hpp>
#include <stdexcept>

#include <ModuleUnit.hpp>

namespace Parsing
{
    ReferenceType::ReferenceType(const Token &token, TypeSyntax *type) : token{token}, type{type}
    {
        if (type->GetType() == SyntaxType::ReferenceType)
        {
            ThrowParsingError(
                ErrorType::ReferenceType, ErrorCode::ReferenceToReference,
                "A reference cannot reference a reference!",
                Range(token.position.start, type->As<ReferenceType>().token.position.end));
            // Logging::Error(color::bold(color::white("A reference cannot reference a reference!")));
            // Logging::CharacterSnippet(Parser::GetFptr(), Range(token.position.start, type->As<ReferenceType>().token.position.end));
        }
    }

    FileIterator *Parser::globalFptr = nullptr;
    ErrorList Parser::errors;

    ModuleUnit *Parser::ParseModule(const std::string &moduleName)
    {
        tokenIterator = tokenList.begin();
        const Token &start = tokenIterator;
        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto node = ParseTopLevelScopeStatement();
                if (node)
                    statements.push_back(node);
                else
                    break;
            }
            catch (const BaseException &e)
            {
                tokenIterator++;
                break;
            }
        }
        for (auto e : errors)
        {
            auto pe = dynamic_cast<ParsingError *>(e);
            if (pe->GetErrorCode() == ErrorCode::ExpectedType)
            {
                auto pet = dynamic_cast<ExpectedTypeError *>(e);
                Logging::Error(color::bold(color::white("Unexpected token {}. Expected {}")), pet->GetFoundToken().raw.c_str(), TokenTypeString(pet->GetTokenType()));
                if (pet->IsLeaf())
                    Logging::CharacterSnippet(fptr, pet->GetFoundToken().position);
                Logging::Log("");
                continue;
            }
            else if (pe->GetErrorCode() == ErrorCode::SampleSnippet)
            {
                auto pet = dynamic_cast<SampleSuggestion *>(e);
                Logging::Log(color::bold(color::white("Try using the following:")));
                Logging::SampleSnippet(fptr, pet->GetPosition(), pet->GetInsert());
                Logging::Log("");
                continue;
            }
            switch (pe->GetErrorType())
            {
            default:
                if (pe->IsLeaf())
                {

                    Logging::Error(color::bold(color::white(pe->GetMessage())));
                    Logging::CharacterSnippet(fptr, pe->GetRange());
                }
                else
                {
                    Logging::Error(pe->GetMessage());
                }
                Logging::Log("");
                break;
            }
        }

        auto module = new ModuleUnit(moduleName, start, statements, tokenIterator);
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
            return ParseConst();
        case TokenType::If:
            return ParseIfStatement();
        case TokenType::Loop:
            return ParseLoopStatement();
        case TokenType::Return:
            return ParseReturnStatement();
        case TokenType::Yield:
            return ParseLoopStatement();
        default:
            auto expr = ParseExpression();
            if (expr)
                return new ExpressionStatement(expr);
        }
        return nullptr;
    }

    Statement Parser::ParseTopLevelScopeStatement()
    {
        switch (tokenIterator->type)
        {
        case TokenType::Let:
            return ParseVariableDecleration();
        case TokenType::Const:
            return ParseConst();
        case TokenType::Template:
            return ParseTemplate();
        case TokenType::Spec:
            return ParseSpec();
        case TokenType::Action:
            return ParseAction();
        case TokenType::Enum:
            return ParseEnum();
        case TokenType::Type:
            return ParseTypeAlias();
        case TokenType::Export:
            return ParseExport();
        default:
            ThrowParsingError(
                ErrorType::TopLevelScope, ErrorCode::InvalidStatement,
                "Invalid statement for top level scope", tokenIterator->position);
            // Logging::Error(color::bold(color::white("Expecting item, found keyword {}")), tokenIterator->raw.c_str());
            // Logging::CharacterSnippet(fptr, tokenIterator->position);
            break;
        }
        return nullptr;
    }

    Statement Parser::ParseTemplate()
    {
        const Token &keyword = Expect(TokenType::Template);
        const Token &identifier = Expect(TokenType::Identifier);
        GenericParameter *generic = nullptr;

        if (tokenIterator->type == TokenType::LeftAngle)
            generic = ParseGenericParameter();

        const Token &open = Expect(TokenType::LeftCurly);

        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            if (tokenIterator->type == TokenType::Newline)
            {
                tokenIterator++;
                continue;
            }
            auto statement = ParseTemplateScopeStatement();
            if (statement)
            {
                statements.push_back(statement);
            }
            else
            {
                // Logging::Error(color::bold(color::white("Unknown token {} in statement")), tokenIterator->raw.c_str());
                // Logging::CharacterSnippet(fptr, tokenIterator->position);
                // tokenIterator++;
                // continue;
                break;
            }
        }

        const Token &close = Expect(TokenType::RightCurly);

        return new TemplateStatement(keyword, identifier, generic, open, statements, close);
    }

    Statement Parser::ParseTemplateScopeStatement()
    {
        switch (tokenIterator->type)
        {
        case TokenType::Let:
            return ParseTemplateVariableDecleration();
        case TokenType::Const:
            return ParseConst();
        case TokenType::Type:
            return ParseTypeAlias();
        default:
            ThrowParsingError(
                ErrorType::TemplateScope, ErrorCode::InvalidStatement,
                "Invalid statement for template body", tokenIterator->position);
            break;
        }
        return nullptr;
    }

    Statement Parser::ParseSpec()
    {
        const Token &keyword = Expect(TokenType::Spec);
        const Token &identifier = Expect(TokenType::Identifier);
        GenericParameter *generic = nullptr;

        if (tokenIterator->type == TokenType::LeftAngle)
            generic = ParseGenericParameter();

        const Token &open = Expect(TokenType::LeftCurly);

        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            if (tokenIterator->type == TokenType::Newline)
            {
                tokenIterator++;
                continue;
            }
            statements.push_back(ParseSpecScopeStatement());
        }

        const Token &close = Expect(TokenType::RightCurly);

        return new SpecStatement(keyword, identifier, generic, open, statements, close);
    }

    Statement Parser::ParseSpecScopeStatement()
    {
        switch (tokenIterator->type)
        {
        case TokenType::Const:
        {
            const Token &keyword = Expect(TokenType::Const);
            const Token &ident = Expect(TokenType::Identifier);
            return ParseSpecFunctionDecleration(keyword, ident);
        }
        case TokenType::Let:
        {
            return ParseSpecVariableDecleration();
        }
        case TokenType::Type:
            return ParseSpecTypeAlias();
        default:
            ThrowParsingError(
                ErrorType::TemplateScope, ErrorCode::InvalidStatement,
                "Invalid statement for spec body", tokenIterator->position);
            break;
        }
        return nullptr;
    }

    Statement Parser::ParseBlockStatement()
    {
        const Token &left = Expect(TokenType::LeftCurly);
        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto statement = ParseStatement();
                if (statement)
                    statements.push_back(statement);
                if (tokenIterator->type == TokenType::Newline)
                    tokenIterator++;
            }
            catch (const BaseException &e)
            {
                tokenIterator++;
                if (!keepGoing)
                    return nullptr;
            }
        }
        const Token &right = Expect(TokenType::RightCurly);
        return new BlockStatement(left, statements, right);
    }

    Statement Parser::ParseTemplateVariableDecleration()
    {
        const Token &let = Expect(TokenType::Let);
        const Token &ident = tokenIterator++;
        TypeSyntax *type = nullptr;

        switch (tokenIterator->type)
        {
        case TokenType::Colon:
            tokenIterator++;
            type = ParseType();
            break;
        case TokenType::Equal:
        {
            const Token &equal = tokenIterator++;
            auto expr = ParseExpression();
            ThrowParsingError(
                ErrorType::TemplateVariableDecleration, ErrorCode::VariableInitializer,
                "Variable cannot have initializer in template!",
                Range(equal.GetStart(), expr->GetEnd()));
            break;
        }
        case TokenType::FuncArrow:
        {
            auto getArrow = tokenIterator++;
            ExpressionBodyStatement *eb = nullptr;
            if (tokenIterator->type == TokenType::FuncArrow)
            {
                auto setArrow = tokenIterator++;
                auto set = ParseStatement();
                eb = new ExpressionBodyStatement(getArrow, setArrow, set);
            }
            else
            {
                auto get = ParseStatement();
                if (tokenIterator->type == TokenType::FuncArrow)
                {
                    auto setArrow = tokenIterator++;
                    auto set = ParseStatement();
                    eb = new ExpressionBodyStatement(getArrow, get, setArrow, set);
                }
                else
                {
                    eb = new ExpressionBodyStatement(getArrow, get);
                }
            }
            return new VariableDeclerationStatement(let, ident, eb, type);
        }

        default:
            break;
        }

        return new VariableDeclerationStatement(let, ident, type, nullptr);
    }

    Statement Parser::ParseActionExpressionBody()
    {
        const Token &let = Expect(TokenType::Let);
        const Token &ident = tokenIterator++;
        TypeSyntax *type = nullptr;

        switch (tokenIterator->type)
        {
        case TokenType::Colon:
            tokenIterator++;
            type = ParseType();
            break;
        case TokenType::Equal:
        {
            const Token &equal = tokenIterator++;
            auto expr = ParseExpression();
            ThrowParsingError(
                ErrorType::TemplateVariableDecleration, ErrorCode::VariableInitializer,
                "Variable cannot have initializer in template!",
                Range(equal.GetStart(), expr->GetEnd()));
            break;
        }

        default:
            break;
        }

        auto getArrow = Expect(TokenType::FuncArrow);
        ExpressionBodyStatement *eb = nullptr;
        if (tokenIterator->type == TokenType::FuncArrow)
        {
            auto setArrow = tokenIterator++;
            auto set = ParseStatement();
            eb = new ExpressionBodyStatement(getArrow, setArrow, set);
        }
        else
        {
            auto get = ParseStatement();
            if (tokenIterator->type == TokenType::FuncArrow)
            {
                auto setArrow = tokenIterator++;
                auto set = ParseStatement();
                eb = new ExpressionBodyStatement(getArrow, get, setArrow, set);
            }
            else
            {
                eb = new ExpressionBodyStatement(getArrow, get);
            }
        }
        return new VariableDeclerationStatement(let, ident, eb, type);
    }

    Statement Parser::ParseSpecVariableDecleration()
    {
        const Token &let = Expect(TokenType::Let);
        const Token &ident = tokenIterator++;
        TypeSyntax *type = nullptr;

        switch (tokenIterator->type)
        {
        case TokenType::Colon:
            tokenIterator++;
            type = ParseType();
            break;
        case TokenType::Equal:
        {
            const Token &equal = tokenIterator++;
            auto expr = ParseExpression();
            ThrowParsingError(
                ErrorType::SpecVariableDecleration, ErrorCode::VariableInitializer,
                "Variable cannot have initializer in spec!",
                Range(equal.GetStart(), expr->GetEnd()));
            break;
        }
        default:
            break;
        }
        const Token &left = Expect(TokenType::LeftCurly);

        if (tokenIterator->type == TokenType::Identifier && (tokenIterator->raw == "get" || tokenIterator->raw == "set"))
        {
            const Token &get = tokenIterator->raw == "get" ? tokenIterator++ : TokenNull;
            if (tokenIterator->type == TokenType::Identifier)
            {
                if (tokenIterator->raw != "set")
                    goto ERROR;
                const Token &set = tokenIterator->raw == "set" ? tokenIterator++ : TokenNull;
                const Token &right = Expect(TokenType::RightCurly);
                auto eb = new ExpressionBodySpecStatement(left, get, set, right);
                return new VariableDeclerationStatement(let, ident, eb, type);
            }
            else
            {
                const Token &set = tokenIterator->raw == "set" ? tokenIterator++ : TokenNull;
                const Token &right = Expect(TokenType::RightCurly);
                auto eb = new ExpressionBodySpecStatement(left, get, set, right);
                return new VariableDeclerationStatement(let, ident, eb, type);
            }
        }
    ERROR:
        while (tokenIterator->type != TokenType::RightCurly)
        {
            tokenIterator++;
        }
        ThrowParsingErrorSnippet(
            ErrorType::SpecVariableDecleration, ErrorCode::VariableInitializer,
            "Variable in spec needs an expression body specifier!",
            tokenIterator->position, CreateSampleSnippet(Range(left.position.start, tokenIterator->position.end), std::string("{ get }")));
    }

    Statement Parser::ParseVariableDecleration()
    {
        const Token &let = Expect(TokenType::Let);
        const Token &ident = tokenIterator++;
        TypeSyntax *type = nullptr;
        Expression initializer = nullptr;
        if (tokenIterator->type == TokenType::Colon)
        {
            const Token &colon = tokenIterator++;
            type = ParseType();
        }
        if (tokenIterator->type == TokenType::Equal)
        {
            const Token &equal = tokenIterator++;
            initializer = ParseExpression();
        }
        return new VariableDeclerationStatement(let, ident, type, initializer);
    }

    Statement Parser::ParseConst()
    {
        const Token &keyword = Expect(TokenType::Const);
        const Token &ident = Expect(TokenType::Identifier);
        if (ident == TokenNull)
            return nullptr;
        switch (tokenIterator->type)
        {
        case TokenType::LeftAngle:
        case TokenType::LeftParen:
            return ParseFunctionDecleration(keyword, ident);
        case TokenType::Equal:
            return ParseConstVariableDecleration(keyword, ident);
        default:
            // Logging::Error(color::bold(color::white("Expecting constant variable initilization or function!")));
            // Logging::CharacterSnippet(fptr, Range(tokenIterator->position.start, tokenIterator->position.end));
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
            const Token &colon = tokenIterator++;
            type = ParseType();
        }

        const Token &equal = Expect(TokenType::Equal);
        if (equal == TokenNull)
        {
            // Logging::Error("Constant variables require an initializer!");
            return nullptr;
        }
        initializer = ParseExpression();

        return new VariableDeclerationStatement(keyword, ident, type, initializer);
    }

    Statement Parser::ParseFunctionDecleration(const Token &keyword, const Token &ident)
    {
        GenericParameter *generic = nullptr;
        if (tokenIterator->type == TokenType::LeftAngle)
            generic = ParseGenericParameter();

        const Token &left = Expect(TokenType::LeftParen);

        std::vector<VariableDeclerationStatement *> parameters;
        bool defaultInit = false;
        while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
        {
            if (tokenIterator->type == TokenType::Newline)
            {
                tokenIterator++;
                continue;
            }
            auto var = dynamic_cast<VariableDeclerationStatement *>(ParseVariableDecleration());
            if (!defaultInit && var->HasInitializer())
                defaultInit = true;
            else if (defaultInit && !var->HasInitializer())
            {
                // // Logging::Error(color::bold(color::white("All parameters after one defualt parameter bust be default as well!")));
                Position end = var->GetIdentifier().position.end;
                if (var->GetVariableType())
                    end = var->GetVariableType()->GetEnd();
                ThrowParsingError(
                    ErrorType::FunctionDecleration, ErrorCode::NotDefault,
                    "All parameters after one defualt parameter must be default as well!",
                    Range(var->GetKeyword().position.start, end));
                // // Logging::CharacterSnippet(fptr, Range(var->GetKeyword().position.start, end));
                return nullptr;
            }
            parameters.push_back(var);
            if (tokenIterator->type != TokenType::RightParen)
                Expect(TokenType::Comma);
            else if (tokenIterator->type == TokenType::Comma)
                tokenIterator++;
        }
        const Token &right = Expect(TokenType::RightParen);
        if (right == TokenNull)
            return nullptr;
        const Token &arrow = Expect(TokenType::FuncArrow);
        if (right == TokenNull)
            return nullptr;
        auto type = ParseType();
        auto body = ParseStatement();
        return new FunctionDeclerationStatement(keyword, ident, generic, left, parameters, right, arrow, type, body);
    }

    Statement Parser::ParseSpecFunctionDecleration(const Token &keyword, const Token &ident)
    {
        GenericParameter *generic = nullptr;
        if (tokenIterator->type == TokenType::LeftAngle)
            generic = ParseGenericParameter();

        const Token &left = tokenIterator++;
        std::vector<VariableDeclerationStatement *> parameters;
        bool defaultInit = false;
        while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
        {
            if (tokenIterator->type == TokenType::Newline)
            {
                tokenIterator++;
                continue;
            }
            auto var = dynamic_cast<VariableDeclerationStatement *>(ParseVariableDecleration());
            if (!defaultInit && var->HasInitializer())
                defaultInit = true;
            else if (defaultInit && !var->HasInitializer())
            {
                // Logging::Error(color::bold(color::white("All parameters after one defualt parameter bust be default as well!")));
                Position end = var->GetIdentifier().position.end;
                if (var->GetVariableType())
                    end = var->GetVariableType()->GetEnd();
                // Logging::CharacterSnippet(fptr, Range(var->GetKeyword().position.start, end));
                return nullptr;
            }
            parameters.push_back(var);
            if (tokenIterator->type != TokenType::RightParen)
                Expect(TokenType::Comma);
            else if (tokenIterator->type == TokenType::Comma)
                tokenIterator++;
        }
        const Token &right = Expect(TokenType::RightParen);

        if (tokenIterator->type == TokenType::FuncArrow)
        {
            const Token &arrow = Expect(TokenType::FuncArrow);
            auto type = ParseType();
            return new FunctionDeclerationStatement(keyword, ident, generic, left, parameters, right, arrow, type);
        }
        return new FunctionDeclerationStatement(keyword, ident, generic, left, parameters, right);
    }

    Statement Parser::ParseIfStatement()
    {
        const Token &keyword = Expect(TokenType::If);
        Use(Using::If);
        auto expression = ParseExpression();
        UnUse(Using::If);

        auto body = ParseStatement();
        ElseStatement *elseClause = dynamic_cast<ElseStatement *>(ParseElif());
        if (elseClause && tokenIterator->type == TokenType::Elif)
        {
            auto bad = ParseElif();
            try
            {
                ThrowParsingError(ErrorType::IfStatement, ErrorCode::ElseAfterElse,
                                  "Cannot have else clause or elif clause after an else clause has already been declared!",
                                  Range(bad->GetStart(), bad->GetEnd()))
            }
            catch (const BaseException &e)
            {
            }
        }
        return new IfStatement(keyword, expression, body, elseClause);

        return nullptr;
    }

    Statement Parser::ParseElif()
    {
        if (tokenIterator->type == TokenType::Elif)
        {
            const Token &keyword = tokenIterator++;
            Use(Using::If);
            auto expression = ParseExpression();
            UnUse(Using::If);
            auto body = ParseStatement();
            auto ifs = new IfStatement(keyword, expression, body, dynamic_cast<ElseStatement *>(ParseElif()));
            return new ElseStatement(tokenIterator, ifs);
        }
        else if (tokenIterator->type == TokenType::Else)
        {
            const Token &keyword = tokenIterator++;
            auto body = ParseStatement();
            return new ElseStatement(keyword, body);
        }
        return nullptr;
    }

    Statement Parser::ParseLoopStatement()
    {
        const Token &keyword = Expect(TokenType::Loop);
        auto expression = ParseExpression();
        if (expression)
        {
            auto body = ParseStatement();
            if (body)
                return new LoopStatement(keyword, expression, body);
        }
        return nullptr;
    }

    Statement Parser::ParseReturnStatement()
    {
        const Token &keyword = Expect(TokenType::Return);
        auto expression = ParseExpression();
        if (expression)
            return new ReturnStatement(keyword, expression);
        return nullptr;
    }

    Statement Parser::ParseYieldStatement()
    {
        const Token &keyword = Expect(TokenType::Yield);
        auto expression = ParseExpression();
        if (expression)
            return new YieldStatement(keyword, expression);
        return nullptr;
    }

    Statement Parser::ParseAction()
    {
        const Token &keyword = Expect(TokenType::Action);
        auto typeA = ParseType();
        if (typeA)
        {

            if (tokenIterator->type == TokenType::Colon)
            {
                const Token &in = tokenIterator++;
                auto typeB = ParseType();
                if (typeB)
                {
                    auto body = dynamic_cast<BlockStatement<> *>(ParseActionBody());
                    if (body)
                        return new ActionSpecStatement(keyword, typeA, in, typeB, body);
                    else
                        return nullptr;
                }
                else
                {
                    ThrowParsingError(
                        ErrorType::ActionScope, ErrorCode::ExpectedType,
                        "Expected type for action", tokenIterator->position);
                    // Logging::Error(color::bold(color::white("Expected type, found token {}")), tokenIterator->raw.c_str());
                    // Logging::CharacterSnippet(fptr, tokenIterator->position);
                    return nullptr;
                }
            }
            else
            {
                auto body = dynamic_cast<BlockStatement<> *>(ParseActionBody());
                if (body)
                    return new ActionBaseStatement(keyword, typeA, body);
                else
                    return nullptr;
            }
        }
        else
        {
            ThrowParsingError(
                ErrorType::ActionScope, ErrorCode::ExpectedType,
                "Expected type for action", tokenIterator->position);
            // Logging::Error(color::bold(color::white("Expected type, found token {}")), tokenIterator->raw.c_str());
            // Logging::CharacterSnippet(fptr, tokenIterator->position);
            return nullptr;
        }
    }

    Statement Parser::ParseActionBody()
    {
        const Token &left = tokenIterator++;

        std::vector<Statement> statements;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto statement = ParseActionScopeStatement();
                if (statement)
                    statements.push_back(statement);
                else
                    return nullptr;
            }
            catch (const BaseException &e)
            {
                // tokenIterator++;
                if (!keepGoing)
                    return nullptr;
            }
        }
        const Token &right = Expect(TokenType::RightCurly);
        if (right == TokenNull)
            return nullptr;
        return new BlockStatement(left, statements, right);
    }

    Statement Parser::ParseActionScopeStatement()
    {
        switch (tokenIterator->type)

        {
        case TokenType::Const:
        {
            try
            {
                const Token &keyword = Expect(TokenType::Const);
                const Token &ident = Expect(TokenType::Identifier);
                return ParseFunctionDecleration(keyword, ident);
            }
            catch (const ExpectedTypeError &e)
            {
                if (e.GetFoundToken().type == TokenType::Equal)
                    ThrowMidParsingError(ErrorType::ActionStatement, ErrorCode::NoVarInAction, "A constant, non-expression body variable cannot be declared in action statement!", e.GetFoundToken().position);
                if (!keepGoing)
                    return nullptr;
            }
            break;
        }
        case TokenType::Let:
        {
            try
            {
                return ParseActionExpressionBody();
            }
            catch (const ExpectedTypeError &e)
            {
                if (e.GetTokenType() == TokenType::FuncArrow)
                    ThrowMidParsingError(ErrorType::ActionStatement, ErrorCode::ExprBodyOnly, "Only expression body variables are allowed", e.GetFoundToken().position);
                if (!keepGoing)
                    return nullptr;
            }
            break;
        }
        case TokenType::Type:
            return ParseTypeAlias();
        default:
            ThrowParsingError(
                ErrorType::ActionScope, ErrorCode::InvalidStatement,
                "Invalid statement for action body", tokenIterator->position);
            break;
        }
        return nullptr;
    }

    Statement Parser::ParseEnum()
    {
        const Token &keyword = Expect(TokenType::Enum);
        const Token &ident = Expect(TokenType::Identifier);
        const Token &left = Expect(TokenType::LeftCurly);

        std::vector<EnumIdentifierStatement *> statements;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            if (tokenIterator->type == TokenType::Newline)
            {
                tokenIterator++;
                continue;
            }
            statements.push_back(new EnumIdentifierStatement(Expect(TokenType::Identifier)));
            if (tokenIterator->type != TokenType::RightCurly)
                Expect(TokenType::Comma);
            else if (tokenIterator->type == TokenType::Comma)
                tokenIterator++;
        }

        const Token &right = Expect(TokenType::RightCurly);
        return new EnumStatement(keyword, ident, left, statements, right);
    }

    Statement Parser::ParseTypeAlias()
    {
        const Token &keyword = Expect(TokenType::Type);
        const Token &identifier = Expect(TokenType::Identifier);
        GenericParameter *generic = nullptr;

        if (tokenIterator->type == TokenType::LeftAngle)
            generic = ParseGenericParameter();

        const Token &eq = Expect(TokenType::Equal);
        TypeSyntax *type = ParseType();
        return new TypeAliasStatement(keyword, identifier, generic, eq, type);
    }

    Statement Parser::ParseSpecTypeAlias()
    {
        const Token &keyword = Expect(TokenType::Type);
        const Token &identifier = Expect(TokenType::Identifier);
        GenericParameter *generic = nullptr;

        if (tokenIterator->type == TokenType::LeftAngle)
            generic = ParseGenericParameter();

        return new TypeAliasStatement(keyword, identifier, generic);
    }

    Statement Parser::ParseExport()
    {
        const Token &keyword = Expect(TokenType::Export);
        Statement st = ParseTopLevelScopeStatement();
        return new ExportDecleration(keyword, st);
    }

    GenericParameter *Parser::ParseGenericParameter()
    {
        const Token &left = Expect(TokenType::LeftAngle);
        std::vector<GenericParameterEntry *> parameters;
        while (tokenIterator->type != TokenType::RightAngle && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto param = ParseGenericParameterEntry();
                parameters.push_back(param);
                if (tokenIterator->type != TokenType::RightAngle)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            catch (const ExpectedTypeError &e)
            {
                tokenIterator++;
                if (!keepGoing)
                    return nullptr;
            }
        }
        const Token &right = Expect(TokenType::RightAngle);
        return new GenericParameter(left, parameters, right);
    }

    GenericParameterEntry *Parser::ParseGenericParameterEntry()
    {
        const Token &identifier = Expect(TokenType::Identifier);
        std::vector<TypeSyntax *> constraints;
        if (tokenIterator->type == TokenType::Colon)
        {
            const Token &colon = tokenIterator++;
            while (tokenIterator->type != TokenType::RightAngle && tokenIterator->type != TokenType::Comma && tokenIterator->type != TokenType::Eof)
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto spec = ParseType();
                if (spec)
                    constraints.push_back(spec);
                if (tokenIterator->type != TokenType::Ampersand)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
        }
        return new GenericParameterEntry(identifier, constraints);
    }

    ObjectInitializer *Parser::ParseObjectInitializer()
    {
        auto left = Expect(TokenType::LeftCurly);
        std::vector<ObjectKeyValue *> values;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Comma && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                const Token &key = Expect(TokenType::Identifier);
                const Token &colon = Expect(TokenType::Colon);
                auto value = ParseExpression();
                values.push_back(new ObjectKeyValue(key, colon, value));
                if (tokenIterator->type != TokenType::RightCurly)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            catch (const ExpectedTypeError &e)
            {
                tokenIterator++;
                if (!keepGoing)
                    return nullptr;
            }
            catch (const BaseException &e)
            {
                if (!keepGoing)
                    return nullptr;
            }
        }
        auto right = Expect(TokenType::RightCurly);
        return new ObjectInitializer(left, values, right);
    }

    MatchEntry *Parser::ParseMatchEntry()
    {
        Expression expr = nullptr;
        if (tokenIterator->type == TokenType::Else)
            tokenIterator++;
        else
            expr = ParseExpression();
        const Token &arrow = Expect(TokenType::FuncArrow);
        auto stmt = ParseStatement();

        return new MatchEntry(expr, arrow, stmt);
    }

    Expression Parser::ParseExpression(uint8_t parentPrecedence, Expression left)
    {
        if (left == nullptr)
        {
            auto unaryPrecedence = UnaryPrecedence(tokenIterator->type);
            if (unaryPrecedence != 0 && unaryPrecedence >= parentPrecedence)
            {
                const Token &op = tokenIterator++;
                auto right = ParseExpression(unaryPrecedence);
                if (right)
                    left = new UnaryExpression(right, op);
            }
            else
            {
                left = ParsePrimaryExpression();
            }
        }

        if (!left)
        {
            ThrowParsingError(
                ErrorType::Expression, ErrorCode::NoLeft,
                std::string("Unexpected token `") + tokenIterator->raw + ("` in expression!"),
                tokenIterator->position);
            // // Logging::Error(color::bold(color::white("Unexpected token `{}` in expression!")), tokenIterator->raw.c_str());
            // // Logging::CharacterSnippet(fptr, tokenIterator->position);
            return nullptr;
        }

        uint8_t precedence = 0;
        while ((precedence = BinaryPrecedence(tokenIterator->type)) > parentPrecedence && precedence > 0)
        {
            const Token &op = tokenIterator++;
            if (op.type == TokenType::As)
            {
                auto right = ParseType();
                if (right)
                {
                    left = new CastExpression(left, op, right);
                    continue;
                }
                else
                {
                    // Logging::Error(color::bold(color::white("'{}' is not a type")), tokenIterator->raw.c_str());
                    // Logging::CharacterSnippet(fptr, Range(tokenIterator->position.start, tokenIterator->position.end));
                    return nullptr;
                }
            }
            try
            {
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
            catch (const BaseException &e)
            {
                return left;
            }
        }
        while ((precedence = PostfixPrecedence(tokenIterator->type)) > parentPrecedence && precedence > 0)
        {
            switch (tokenIterator->type)
            {
            case TokenType::LeftParen:
                if ((tokenIterator + 1)->type == TokenType::Let || ((tokenIterator + 1)->type == TokenType::RightParen && (tokenIterator + 2)->type == TokenType::FuncArrow))
                    return left;
                left = ParseFunctionCall(left);
                break;
            case TokenType::LeftSquare:
                left = ParseSubscript(left);
                break;

            default:
                const Token &op = tokenIterator++;
                left = new PostfixExpression(left, op);
                break;
            }
        }
        return left;
    }

    Expression Parser::ParsePrimaryExpression()
    {
        if (tokenIterator->type == TokenType::LeftParen)
        {
            const Token &left = tokenIterator++;
            if (tokenIterator->type == TokenType::Let || tokenIterator->type == TokenType::RightParen)
            {
                tokenIterator--;
                return ParseAnonymousFunction();
            }
            else
            {
                auto expression = ParseExpression();
                const Token &right = Expect(TokenType::RightParen);

                return expression;
            }
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
        case TokenType::LeftCurly:
        {
            return ParseObjectInitializer();
        }
        case TokenType::LeftSquare:
        {
            return ParseArrayLiteral();
        }
        case TokenType::Match:
        {
            return ParseMatch();
        }
        default:
            ThrowParsingError(
                ErrorType::Literal, ErrorCode::UnknownLiteral,
                std::string("Expected a literal value! Found token `") + tokenIterator->raw + std::string("` instead."),
                tokenIterator->position);
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
        switch (tokenIterator->type)
        {
        // case TokenType::LeftParen:
        //     return ParseFunctionCall(token);
        case TokenType::LeftCurly:
            if (!IsUsed(Using::If))
                return ParseTemplateInitializer(token);
        default:
            return new IdentifierExpression(token);
        }
    }

    Expression Parser::ParseFunctionCall(Expression fn)
    {
        const Token &left = Expect(TokenType::LeftParen);
        std::vector<Parsing::Expression> args;

        while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                args.push_back(ParseExpression());
                if (tokenIterator->type != TokenType::RightParen)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            catch (const BaseException &e)
            {
                if (!keepGoing)
                    return nullptr;
            }
        }

        const Token &right = Expect(TokenType::RightParen);
        return new CallExpression(fn, left, right, args);
    }

    Expression Parser::ParseAnonymousFunction()
    {
        const Token &left = Expect(TokenType::LeftParen);

        std::vector<VariableDeclerationStatement *> parameters;
        bool defaultInit = false;
        while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
        {
            if (tokenIterator->type == TokenType::Newline)
            {
                tokenIterator++;
                continue;
            }
            auto var = dynamic_cast<VariableDeclerationStatement *>(ParseVariableDecleration());
            if (!defaultInit && var->HasInitializer())
                defaultInit = true;
            else if (defaultInit && !var->HasInitializer())
            {
                Position end = var->GetIdentifier().position.end;
                if (var->GetVariableType())
                    end = var->GetVariableType()->GetEnd();
                ThrowParsingError(
                    ErrorType::FunctionDecleration, ErrorCode::NotDefault,
                    "All parameters after one defualt parameter must be default as well!",
                    Range(var->GetKeyword().position.start, end));
                return nullptr;
            }
            parameters.push_back(var);
            if (tokenIterator->type != TokenType::RightParen)
                Expect(TokenType::Comma);
            else if (tokenIterator->type == TokenType::Comma)
                tokenIterator++;
        }
        const Token &right = Expect(TokenType::RightParen);
        const Token &arrow = Expect(TokenType::FuncArrow);
        auto type = ParseType();
        auto body = ParseStatement();
        return new AnonymousFunctionExpression(left, parameters, right, arrow, type, body);
    }

    Expression Parser::ParseSubscript(Expression expr)
    {
        const Token &left = Expect(TokenType::LeftSquare);
        auto subsr = ParseExpression();
        const Token &right = Expect(TokenType::RightSquare);
        return new SubscriptExpression(expr, left, subsr, right);
    }

    Expression Parser::ParseTemplateInitializer(const Token &identifier)
    {
        auto initializer = ParseObjectInitializer();
        if (initializer->GetValues().empty())
            initializer = nullptr;
        return new TemplateInitializer(identifier, initializer);
    }

    ArrayLiteralEntry *Parser::ParseArrayLiteralEntry()
    {
        auto expression = ParseExpression();
        if (tokenIterator->type == TokenType::Colon)
        {
            const Token &colon = tokenIterator++;
            auto boundary = ParseExpression();
            return new ArrayLiteralBoundaryEntry(expression, colon, boundary);
        }
        return new ArrayLiteralExpressionEntry(expression);
    }

    Expression Parser::ParseArrayLiteral()
    {
        const Token &left = Expect(TokenType::LeftSquare);
        std::vector<ArrayLiteralEntry *> values;
        while (tokenIterator->type != TokenType::RightSquare && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                values.push_back(ParseArrayLiteralEntry());
                if (tokenIterator->type != TokenType::RightSquare)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            catch (const BaseException &e)
            {
                if (!keepGoing)
                    return nullptr;
            }
        }
        const Token &right = Expect(TokenType::RightSquare);
        return new ArrayLiteral(left, values, right);
    }

    Expression Parser::ParseMatch()
    {
        const Token &keyword = Expect(TokenType::Match);
        auto expr = ParseExpression();
        const Token &left = Expect(TokenType::LeftCurly);

        std::vector<MatchEntry *> entries;
        while (tokenIterator->type != TokenType::RightCurly && tokenIterator->type != TokenType::Eof)
        {
            try
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                entries.push_back(ParseMatchEntry());
                Expect(TokenType::Newline);
            }
            catch (const BaseException &e)
            {
                if (!keepGoing)
                    return nullptr;
            }
        }
        const Token &right = Expect(TokenType::RightCurly);

        return new MatchExpression(keyword, expr, new BlockStatement(left, entries, right));
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
            const Token &left = tokenIterator++;
            std::vector<TypeSyntax *> parameters;

            while (tokenIterator->type != TokenType::RightParen && tokenIterator->type != TokenType::Eof)
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto nextType = ParseType();
                if (!nextType)
                {
                    // Logging::Error(color::bold(color::white("'{}' is not a type")), tokenIterator->raw.c_str());
                    // Logging::CharacterSnippet(fptr, Range(tokenIterator->position.start, tokenIterator->position.end));
                    tokenIterator++;
                    return nullptr;
                }
                parameters.push_back(nextType);
                if (tokenIterator->type != TokenType::RightParen)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            const Token &right = Expect(TokenType::RightParen);

            auto retType = ParseType();
            baseType = new FunctionType(left, parameters, right, tokenIterator->type == TokenType::FuncArrow ? tokenIterator++ : TokenNull, retType);
            break;
        }
        case TokenType::Ampersand:
        {
            const Token &amp = tokenIterator++;
            baseType = ParseType();
            if (baseType)
                baseType = new ReferenceType(amp, baseType);
            break;
        }
        case TokenType::LeftSquare:
        {
            const Token &open = tokenIterator++;
            auto type = ParseType();
            if (tokenIterator->type == TokenType::Colon)
            {
                const Token &colon = tokenIterator++;
                auto size = ParseExpression();
                const Token &close = Expect(TokenType::RightSquare);
                baseType = new ArrayType(open, type, colon, size, close);
                break;
            }
            const Token &close = Expect(TokenType::RightSquare);
            baseType = new ArrayType(open, type, close);
            break;
        }
        default:
            return nullptr;
        }
        if (tokenIterator->type == TokenType::LeftAngle)
        {
            const Token &left = tokenIterator++;
            std::vector<TypeSyntax *> arguments;

            while (tokenIterator->type != TokenType::RightAngle && tokenIterator->type != TokenType::Eof)
            {
                if (tokenIterator->type == TokenType::Newline)
                {
                    tokenIterator++;
                    continue;
                }
                auto nextType = ParseType();
                if (!nextType)
                {
                    // Logging::Error(color::bold(color::white("'{}' is not a type")), tokenIterator->raw.c_str());
                    // Logging::CharacterSnippet(fptr, Range(tokenIterator->position.start, tokenIterator->position.end));
                    tokenIterator++;
                    return nullptr;
                }
                arguments.push_back(nextType);
                if (tokenIterator->type != TokenType::RightAngle)
                    Expect(TokenType::Comma);
                else if (tokenIterator->type == TokenType::Comma)
                    tokenIterator++;
            }
            const Token &right = Expect(TokenType::RightAngle);
            if (right == TokenNull)
                return nullptr;
            baseType = new GenericType(baseType, left, arguments, right);
        }
        return baseType;
    }

    uint8_t Parser::UnaryPrecedence(TokenType type)
    {
        switch (type)
        {
        // case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::DoublePlus:
        case TokenType::DoubleMinus:
        case TokenType::Not:
        case TokenType::Tilda:
        case TokenType::Star:
        case TokenType::Ampersand:
        case TokenType::Typeof:
            return 14;
        default:
            return 0;
        }
    }

    uint8_t Parser::PostfixPrecedence(TokenType type)
    {
        switch (type)
        {
        case TokenType::DoublePlus:
        case TokenType::DoubleMinus:
            return 14;
        case TokenType::LeftParen:
        case TokenType::LeftSquare:
            return 15;
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
        case TokenType::TripleLeftShiftEquals:
        case TokenType::TripleRightShiftEquals:
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
        case TokenType::TripleLeftShift:
        case TokenType::TripleRightShift:
            return 11;
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Percent:
            return 12;
        case TokenType::Star:
        case TokenType::ForwardSlash:
            return 13;
        case TokenType::Spread:
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
        case TokenType::TripleLeftShiftEquals:
        case TokenType::TripleRightShiftEquals:
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

            ThrowExpectedType(ErrorType::Expect, "", type);
            // // Logging::Error(color::bold(color::white("Unexpected token {}. Expected {}")), tokenIterator->raw.c_str(), TokenTypeString(type));
            // // Logging::CharacterSnippet(fptr, tokenIterator->position);
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
        ets(ArrayLiteralExpressionEntry);
        ets(ArrayLiteralBoundaryEntry);
        ets(ArrayLiteral);
        ets(TemplateInitializer);
        ets(BinaryExpression);
        ets(UnaryExpression);
        ets(PostfixExpression);
        ets(CallExpression);
        ets(SubscriptExpression);
        ets(IdentifierExpression);
        ets(CastExpression);
        ets(AnonymousFunctionExpression);

        ets(GenericParameterEntry);
        ets(GenericParameter);
        ets(ObjectKeyValue);
        ets(ObjectInitializer);

        ets(ExpressionBodyStatement);
        ets(ExpressionBodySpecStatement);
        ets(TemplateStatement);
        ets(SpecStatement);
        ets(BlockStatement);
        ets(ExpressionStatement);
        ets(VariableDeclerationStatement);
        ets(FunctionDeclerationStatement);
        ets(IfStatement);
        ets(ElseStatement);
        ets(LoopStatement);
        ets(ReturnStatement);
        ets(YieldStatement);
        ets(ActionBaseStatement);
        ets(ActionSpecStatement);
        ets(EnumStatement);
        ets(EnumIdentifierStatement);
        ets(TypeAliasStatement);
        ets(MatchEntry);
        ets(MatchExpression);

        ets(TypeExpression);
        ets(PrimitiveType);
        ets(IdentifierType);
        ets(ArrayType);
        ets(FunctionType);
        ets(ReferenceType);
        ets(GenericType);

        ets(ExportDecleration);

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
        case Parsing::SyntaxType::PostfixExpression:
        {
            stream << " " << node.As<Parsing::PostfixExpression>().GetOperator().type;
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
        case Parsing::SyntaxType::TemplateStatement:
        {
            stream << " `" << node.As<Parsing::TemplateStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::SpecStatement:
        {
            stream << " `" << node.As<Parsing::SpecStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::GenericParameterEntry:
        {
            stream << " `" << node.As<Parsing::GenericParameterEntry>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::TemplateInitializer:
        {
            stream << " `" << node.As<Parsing::TemplateInitializer>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::EnumStatement:
        {
            stream << " `" << node.As<Parsing::EnumStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::EnumIdentifierStatement:
        {
            stream << " `" << node.As<Parsing::EnumIdentifierStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::TypeAliasStatement:
        {
            stream << " `" << node.As<Parsing::TypeAliasStatement>().GetIdentifier().raw << "`";
            break;
        }
        case Parsing::SyntaxType::MatchEntry:
        {
            if (node.As<Parsing::MatchEntry>().IsElse())
                stream << " Default";
            break;
        }
        case Parsing::SyntaxType::ExpressionBodySpecStatement:
        {
            const auto &eb = node.As<Parsing::ExpressionBodySpecStatement>();
            if (eb.HasGet())
                stream << " get";
            if (eb.HasSet())
                stream << " set";
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