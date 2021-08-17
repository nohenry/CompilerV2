#include <Tokenizer.hpp>
#include <cstdio>
#include <iostream>
#include <Trie.hpp>
#include <tuple>
#include <Colors.hpp>
#include <Log.hpp>

void print();

#define Try(expr)                                   \
    if ((current = std::get<0>(expr)) != TokenNull) \
    {                                               \
        goto DONE_TRYING;                           \
    }
#define TryDisregard(expr)        \
    if (expr)                     \
    {                             \
        current = TokenDisregard; \
        goto DONE_TRYING;         \
    }

TokenList &Tokenizer::Tokenize()
{
    char c = fptr++;
    int i = 0;
    do
    {

        fptr--;
        Token current = TokenNull;
        if (c == ' ' || c == '\n' || c == '\r')
        {
            if (c == '\n' || c == '\r' && (tokenList.size() == 0 || tokenList.back().type != TokenType::Newline))
            {
                auto start = (tokenList.size() == 0 ? Position() : tokenList.back().position.end);
                auto end = start;
                end.character++;
                tokenList.push_back(Token(TokenType::Newline, Range(start, end)));
            }
            c = fptr++;
            current = TokenDisregard;
            goto DONE_TRYING;
        }
        TryDisregard(Comment());
        Try(IterateTrie());
        Try(Identifier());
        Try(Float());
        Try(Integer());
        Try(String());
    DONE_TRYING:
        if (current != TokenNull)
        {
            if (current != TokenDisregard)
                tokenList.push_back(current);
        }
        else if (*fptr)
        {
            Logging::Error(color::bold(color::white("expected item, found `{}`")), c);
            Logging::CharacterSnippet(fptr);
            fptr++;
        }
        c = fptr++;
        i++;
    } while (!fptr.End());
    this->tokenList.push_back(TokenNull);
    return this->tokenList;
}

Tokenizer::IterateType Tokenizer::IterateTrieRec(const TrieNode &node, bool use, Position startPosition)
{
    char c = fptr++;
    auto pos = fptr.CalulatePosition();

    uint8_t s = 0;
    for (int i = 0; i < node.n; i++)
    {
        auto f = fptr.Offset();
        auto [token, sum] = IterateTrieRec(*(trie + node.i + s), use && node == c, startPosition);
        s += sum;

        if (use && token != TokenNull && node == c)
            return std::make_tuple(token, s + 1);
    }
    auto f = fptr.Offset();
    if (use && node.term && node == c)
    {
        if (node.GetType() == TokenType::LeftAngle)
            angleIndex++;
        else if (node.GetType() == TokenType::RightAngle)
            angleIndex--;
        else if (angleIndex > 0 && node.GetType() == TokenType::RightShift)
        {
            fptr--;
            return std::make_tuple(TokenNull, s);
        }

        auto str = std::string(TokenTypeString(node.GetType()));
        str[0] = tolower(str[0]);
        Token token(node.GetType(), Range(startPosition, pos), str);
        return std::make_tuple(token, s + 1);
    }

    fptr--;
    f = fptr.Offset();
    return std::make_tuple(TokenNull, s + 1);
}

Tokenizer::IterateType Tokenizer::IterateTrie()
{
    uint8_t i = 0;
    for (i = 0; i < trie_size;)
    {
        auto f = fptr.CalulatePosition();
        auto [token, sum] = IterateTrieRec(*(trie + i), true, f);
        i += sum;
        if (token != TokenNull)
            return std::make_tuple(token, i);
    }

    return std::make_tuple(TokenNull, i);
}

Tokenizer::IterateType Tokenizer::Identifier()
{
    auto pos = fptr.CalulatePosition();
    char c = fptr++;
    if (isalpha(c) || c == '_')
    {
        Token token(TokenType::Identifier);
        do
        {
            token.raw += c;
            c = fptr++;
        } while (isalnum(c) || c == '_');
        fptr--;
        token.position.start = pos;
        token.position.end = fptr.CalulatePosition();
        return std::make_tuple(token, 0);
    }
    else
    {
        fptr--;
        return std::make_tuple(TokenNull, 0);
    }
}

Tokenizer::IterateType Tokenizer::Integer()
{
    auto pos = fptr.CalulatePosition();
    char c = fptr++;
    std::string str = "";
    if (c == '0')
    {
        switch (*fptr)
        {
        case 'x':
        {
            fptr++;
            while (isxdigit(c) || c == '_')
            {
                str += c;
                c = *fptr;
                if (isxdigit(c) || c == '_')
                    fptr++;
            }
            Token token(TokenType::HexInt, Range(pos, fptr.CalulatePosition()), str);
            return std::make_tuple(token, 0);
        }
        case 'q':
        {
            fptr++;
            while ((c >= '0' && c <= '7') || c == '_')
            {
                str += c;
                c = *fptr;
                if ((c >= '0' && c <= '7') || c == '_')
                    fptr++;
            }
            Token token(TokenType::OctInt, Range(pos, fptr.CalulatePosition()), str);
            return std::make_tuple(token, 0);
        }
        case 'b':
        {
            fptr++;
            while ((c >= '0' && c <= '1') || c == '_')
            {
                str += c;
                c = *fptr;
                if ((c >= '0' && c <= '1') || c == '_')
                    fptr++;
            }
            Token token(TokenType::BinInt, Range(pos, fptr.CalulatePosition()), str);
            return std::make_tuple(token, 0);
        }
        }
    }
    if (isdigit(c))
    {

        while (isdigit(c) || c == '_')
        {
            str += c;
            c = fptr++;
        }
        fptr--;
        Token token(TokenType::Integer, Range(pos, fptr.CalulatePosition()), str);
        if (CheckPrimitiveTypeSize(token))
        {
            return std::make_tuple(TokenDisregard, 0);
        }
        return std::make_tuple(token, 0);
    }
    else
    {
        fptr--;
        return std::make_tuple(TokenNull, 0);
    }
}

Tokenizer::IterateType Tokenizer::Float()
{
    auto pos = fptr.CalulatePosition();
    char c = fptr++;
    bool decimal = false;
    bool exponent = false;
    std::string str = "";

    if (isdigit(c))
    {

        while (isdigit(c) || c == '_')
        {
            str += c;
            c = fptr++;
            if (c == '.')
            {
                if (!decimal)
                {
                    str += c;
                    c = fptr++;
                    decimal = true;
                }
                else
                {
                    fptr--;
                    Token token(TokenType::Floating, Range(pos, fptr.CalulatePosition()), str);
                    return std::make_tuple(token, 0);
                }
            }
            else if (c == 'e' || c == 'E')
            {
                if (!exponent)
                {
                    str += c;
                    c = fptr++;
                    exponent = true;
                    if (!(isdigit(c) || c == '-'))
                    {
                        Logging::Error(color::bold(color::white("expected at least one digit in exponent")));
                        Logging::CharacterSnippet(fptr, Range(pos, (fptr - 1).CalulatePosition()));
                        return std::make_tuple(TokenNull, 0);
                    }
                    else
                    {
                        str += c;
                        c = fptr++;
                    }
                }
                else
                {
                    fptr--;
                    Token token(TokenType::Floating, Range(pos, fptr.CalulatePosition()), str);
                    return std::make_tuple(token, 0);
                }
            }
        }
        fptr--;
        if (!decimal && !exponent)
        {
            Token token(TokenType::Integer, Range(pos, fptr.CalulatePosition()), str);
            if (CheckPrimitiveTypeSize(token))
            {
                return std::make_tuple(TokenDisregard, 0);
            }
            return std::make_tuple(token, 0);
        }
        else if (decimal && c == '.')
        {
            fptr--;
            Token token(TokenType::Integer, Range(pos, fptr.CalulatePosition()), str.substr(0, str.size() - 1));
            return std::make_tuple(token, 0);
        }
        Token token(TokenType::Floating, Range(pos, fptr.CalulatePosition()), str);
        return std::make_tuple(token, 0);
    }
    else
    {
        fptr--;
        return std::make_tuple(TokenNull, 0);
    }
}

bool Tokenizer::CheckPrimitiveTypeSize(const Token &integer)
{
    if (tokenList.end() == tokenList.begin())
        return false;
    auto type = (tokenList.end() - 1)->type;
    if ((type == TokenType::Uint || type == TokenType::Int || type == TokenType::Float || type == TokenType::Char) && ((tokenList.end() - 1)->position.end == integer.position.start))
    {
        (tokenList.end() - 1)->ivalue = integer.ivalue;
        (tokenList.end() - 1)->position.end = integer.position.end;
        (tokenList.end() - 1)->raw += std::to_string(integer.ivalue);
        return true;
    }
    return false;
}

Tokenizer::IterateType Tokenizer::String()
{
    auto pos = fptr.CalulatePosition();
    char c = fptr++;
    switch (c)
    {
    case '"':
    case '\'':
    {
        Token token(TokenType::String, Range(pos, fptr.CalulatePosition()));
        c = fptr++;
        do
        {
            token.raw += c;
            c = fptr++;
        } while (c != '"' && c != '\'');
        token.position.end = fptr.CalulatePosition();
        return std::make_tuple(token, 0);
    }
    }
    fptr--;
    return std::make_tuple(TokenNull, 0);
}

bool Tokenizer::Comment()
{
    char c = fptr++;
    switch (c)
    {
    case '/':
    {
        switch (*fptr)
        {
        case '/':
        {
            while (fptr != '\n' && fptr != '\r')
            {
                fptr++;
            }
            return true;
        }
        case '*':
        {
            while (!(fptr == '*' && fptr + 1 == '/'))
            {
                fptr++;
            }
            fptr += 2;
            return true;
        }
        }
    }
    case '#':
    {
        while (fptr != '\n' && fptr != '\r')
        {
            fptr++;
        }
        return true;
    }
    }
    fptr--;
    return false;
}

uint8_t print_trie(const TrieNode *const trie_node, int n)
{
    if (!trie_node)
        return 0;
    for (int i = 0; i < n; i++)
        printf(" ");

    printf("%c\n", trie_node->c + 97);

    int s = 0;

    for (int i = 0; i < trie_node->n; i++)
    {
        uint8_t index = trie_node->i;
        s += print_trie(trie + index + s, n + 1);
    }
    return s + 1;
}

void print()
{
    for (size_t i = 0; i < trie_size;)
    {
        i += print_trie(trie + i, 0);
    }
}