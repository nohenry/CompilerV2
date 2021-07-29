#pragma once
#include <stdint.h>
#include <string>
#include <ostream>
#include <fstream>
#include <iostream>
#include <cassert>

struct Position;

namespace Parsing
{
    enum class SyntaxType
    {
        None,
        Integer,
        Floating,
        Boolean,
        String,
        ObjectKeyValue,
        ObjectInitializer,
        TemplateInitializer,
        ArrayLiteralExpressionEntry,
        ArrayLiteralBoundaryEntry,
        ArrayLiteral,
        BinaryExpression,
        UnaryExpression,
        PostfixExpression,
        CallExpression,
        SubscriptExpression,
        IdentifierExpression,
        CastExpression,
        AnonymousFunctionExpression,

        GenericParameterEntry,
        GenericParameter,

        ExpressionBodyStatement,
        ExpressionBodySpecStatement,
        TemplateStatement,
        SpecStatement,
        BlockStatement,
        ExpressionStatement,
        VariableDeclerationStatement,
        FunctionDeclerationStatement,
        IfStatement,
        ElseStatement,
        LoopStatement,
        ReturnStatement,
        YieldStatement,
        ActionBaseStatement,
        ActionSpecStatement,
        EnumStatement,
        EnumIdentifierStatement,
        TypeAliasStatement,
        MatchEntry,
        MatchExpression,

        PrimitiveType,
        IdentifierType,
        ArrayType,
        FunctionType,
        ReferenceType,
        TypeExpression,
        GenericType,

        ExportDecleration,
    };

    class SyntaxNode
    {

    public:
        virtual ~SyntaxNode() {}

        const virtual SyntaxType GetType() const = 0;

        const virtual uint8_t NumChildren() const = 0;
        const virtual SyntaxNode &operator[](int index) const = 0;

        virtual const Position &GetStart() const = 0;
        virtual const Position &GetEnd() const = 0;

        template <typename T>
        const T &As() const { return *dynamic_cast<const T *>(this); }
    };
} // namespace Parsing

enum class TokenType
{
    Eof,
    Newline,
    Disregard,
    Whitespace, // space, newline
    Semicolon,  // ;
    Integer,    // Integer value
    HexInt,
    OctInt,
    BinInt,
    Floating, // Floating point value
    String,   // String literal

    Plus,                   // +
    DoublePlus,             // ++
    PlusEqual,              // +=
    Minus,                  // -
    DoubleMinus,            // --
    MinusEqual,             // -=
    Star,                   // *
    StarEqual,              // *=
    ForwardSlash,           // /
    SlashEqual,             // /=
    LeftParen,              // (
    RightParen,             // )
    LeftCurly,              // {
    RightCurly,             // }
    LeftAngle,              // <
    RightAngle,             // >
    LeftSquare,             // [
    RightSquare,            // ]
    BiggerEqual,            // >=
    SmallerEqual,           // <=
    NotBigger,              // !>
    NotSmaller,             // !<
    Comma,                  // ,
    Equal,                  // =
    DoubleEqual,            // ==
    NotEqual,               // !=
    Dot,                    // .
    Spread,                 // ..
    Ampersand,              // &
    AmpersandEquals,        // &=
    Percent,                // %,
    PercentEqual,           // %=
    At,                     // @
    Colon,                  // :
    LeftShift,              // <<
    RightShift,             // >>
    TripleLeftShift,        // <<>
    TripleRightShift,       // <>>
    LeftShiftEquals,        // <<=
    RightShiftEquals,       // >>=
    TripleLeftShiftEquals,  // <<>=
    TripleRightShiftEquals, // <>>=
    Tilda,                  // ~
    Carrot,                 // ^
    CarrotEquals,           // ^=
    Pipe,                   // |
    PipeEquals,             // |=
    Not,                    // !
    FuncArrow,              // =>

    /*
     *   Keywords
     */

    Typeof,   // sizeof
    Asm,      // asm
    Match,    // match
    When,     // when
    In,       // in
    If,       // if
    Elif,     // elif
    Else,     // else
    Loop,     // loop
    Return,   // return
    Int,      // int
    Uint,     // uint
    Float,    // float
    Char,     // char
    Bool,     // bool
    Template, // template
    Spec,     // spec
    True,     // true
    False,    // false
    Import,   // import
    Let,      // let
    Persist,  // persist
    Or,       // or
    And,      // and
    Null,     // null
    Module,   // module
    Function, // function
    Type,     // type
    Export,   // export
    Yield,    // yield
    As,       // as
    Const,    // const
    Action,   // action
    Enum,     // enum

    Identifier,

    Tokens,
};

struct Position
{
    uint32_t line;
    uint32_t character;

    bool operator==(const Position &other) const
    {
        return line == other.line && character == other.character;
    }
    bool operator!=(const Position &other) const
    {
        return !operator==(other);
    }
};

struct Range
{
    union
    {
        uint32_t positions[4];
        struct
        {
            Position start;
            Position end;
        };
    };

    Range() : start{0}, end{0}
    {
    }

    Range(Position start, Position end) : start{start}, end{end}
    {
    }

    bool operator==(const Range &other) const
    {
        return start == other.start && end == other.end;
    }
    bool operator!=(const Range &other) const
    {
        return !operator==(other);
    }
};

struct Token : public Parsing::SyntaxNode
{
    TokenType type;
    Range position;
    std::string raw;
    union
    {
        uint64_t ivalue;
        double fvalue;
    };

    Token() : type{TokenType::Eof}, position{}, raw{""}, ivalue{0}
    {
    }

    Token(TokenType type) : type{type}, position{}, raw{""}, ivalue{0}
    {
    }

    Token(TokenType type, Range position) : type{type}, position{position}, raw{""}, ivalue{0}
    {
    }

    Token(TokenType type, Range position, std::string raw) : type{type}, position{position}, raw{raw}, ivalue{0}
    {
        switch (type)
        {
        case TokenType::Integer:
        {
            ivalue = std::stoi(raw);
            break;
        }
        case TokenType::HexInt:
        {
            ivalue = std::stoi(raw, nullptr, 16);
            this->type = TokenType::Integer;
            break;
        }
        case TokenType::OctInt:
        {
            ivalue = std::stoi(raw, nullptr, 8);
            this->type = TokenType::Integer;
            break;
        }
        case TokenType::BinInt:
        {
            ivalue = std::stoi(raw, nullptr, 2);
            this->type = TokenType::Integer;
            break;
        }
        case TokenType::Floating:
        {
            fvalue = std::stod(raw);
            break;
        }
        default:
            break;
        }
    }

    bool operator==(const Token &other) const
    {
        return type == other.type && raw == other.raw && position == other.position;
    }
    bool operator!=(const Token &other) const
    {
        return !operator==(other);
    }

    const virtual Parsing::SyntaxType GetType() const override
    {
        return Parsing::SyntaxType::None;
    }

    const virtual uint8_t NumChildren() const override
    {
        return 0;
    }

    const virtual SyntaxNode &operator[](int index) const override
    {
        return *this;
    }

    virtual const Position &GetStart() const override
    {
        return position.start;
    }

    virtual const Position &GetEnd() const override
    {
        return position.end;
    }
};

class TokenIterator
{
public:
    using ValueType = Token;
    using PointerType = ValueType *;
    using ReferenceType = ValueType &;

public:
    TokenIterator(PointerType token) : token{token}
    {
    }

    TokenIterator &operator++()
    {
        token++;
        return *this;
    }

    TokenIterator operator++(int)
    {
        TokenIterator iterator = *this;
        ++(*this);
        return iterator;
    }

    bool operator==(const TokenIterator &other) const
    {
        return token == other.token;
    }

    bool operator!=(const TokenIterator &other) const
    {
        return token != other.token;
    }

    TokenIterator &operator--()
    {
        token--;
        return *this;
    }

    TokenIterator operator--(int)
    {
        TokenIterator iterator = *this;
        --(*this);
        return iterator;
    }

    ValueType &operator[](int index)
    {
        return *(token + index);
    }

    const ValueType &operator[](int index) const
    {
        return *(token + index);
    }

    ValueType *operator->()
    {
        return token;
    }

    const ValueType *operator->() const
    {
        return token;
    }

    ValueType &operator*()
    {
        return *token;
    }

    const ValueType &operator*() const
    {
        return *token;
    }

    TokenIterator &operator+=(int value)
    {
        token += value;
        return *this;
    }

    TokenIterator operator+(int value) const
    {
        TokenIterator iterator = *this;
        return iterator += value;
    }

    TokenIterator &operator-=(int value)
    {
        token -= value;
        return *this;
    }

    TokenIterator operator-(int value) const
    {
        TokenIterator iterator = *this;
        return iterator -= value;
    }

    bool operator<(const TokenIterator &other) const
    {
        return token < other.token;
    }

    bool operator>(const TokenIterator &other) const
    {
        return other < *this;
    }

    bool operator<=(const TokenIterator &other) const
    {
        return !(other < *this);
    }

    bool operator>=(const TokenIterator &other) const
    {
        return !(*this < other);
    }

    operator ValueType()
    {
        return *token;
    }

    operator ValueType &()
    {
        return *token;
    }

    operator const ValueType &() const
    {
        return *token;
    }

private:
    PointerType token;
};

class FileIterator
{
public:
    using ValueType = char;
    using PointerType = ValueType *;
    using ReferenceType = ValueType &;

public:
    FileIterator(PointerType token) : ptr{token}, fileName{""}
    {
    }

    FileIterator(std::string filename) : ptr{nullptr}, fileName{filename}
    {
        std::ifstream file(fileName, std::ios::binary | std::ios::ate);
        size = file.tellg();
        file.seekg(0, std::ios::beg);

        ptr = beg = new char[size + 32];
        memset(ptr + size, 0, 32);
        if (!file.read(ptr, size))
        {
            std::cerr << "Unable to read file: " << fileName << std::endl;
        }
    }

    FileIterator &operator++()
    {
        if (ptr[0] == '\r' && ptr[1] == '\n')
        {
            ptr += 2;
        }
        else
            ptr++;
        return *this;
    }

    FileIterator operator++(int)
    {
        FileIterator iterator = *this;
        ++(*this);
        return iterator;
    }

    FileIterator &operator--()
    {
        ptr--;
        if (ptr[0] == '\n' && ptr[-1] == '\r')
        {
            ptr--;
        }
        return *this;
    }

    FileIterator operator--(int)
    {
        FileIterator iterator = *this;
        --(*this);
        if (ptr[0] == '\n' && ptr[-1] == '\r')
            --(*this);
        return iterator;
    }

    ValueType &operator[](int index)
    {
        return *(ptr + index);
    }

    const ValueType &operator[](int index) const
    {
        return *(ptr + index);
    }

    ValueType *operator->()
    {
        return ptr;
    }

    const ValueType *operator->() const
    {
        return ptr;
    }

    ValueType &operator*()
    {
        return *ptr;
    }

    const ValueType &operator*() const
    {
        return *ptr;
    }

    bool operator==(const FileIterator &other) const
    {
        return ptr == other.ptr;
    }

    bool operator!=(const FileIterator &other) const
    {
        return ptr != other.ptr;
    }

    FileIterator &operator+=(int value)
    {
        if (ptr[0] == '\r' && ptr[1] == '\n')
            ptr++;
        ptr += value;
        return *this;
    }

    FileIterator operator+(int value) const
    {
        FileIterator iterator = *this;
        return iterator += value;
    }

    FileIterator &operator-=(int value)
    {
        ptr -= value;
        if (ptr[0] == '\n' && ptr[-1] == '\r')
            ptr--;
        return *this;
    }

    FileIterator operator-(int value) const
    {
        FileIterator iterator = *this;
        return iterator -= value;
    }

    bool operator<(const FileIterator &other) const
    {
        return ptr < other.ptr;
    }

    bool operator>(const FileIterator &other) const
    {
        return other < *this;
    }

    bool operator<=(const FileIterator &other) const
    {
        return !(other < *this);
    }

    bool operator>=(const FileIterator &other) const
    {
        return !(*this < other);
    }

    FileIterator &operator=(const FileIterator &fraction)
    {
        ptr = fraction.ptr;
        return *this;
    }

    operator ValueType()
    {
        return *ptr;
    }

    uint64_t Offset() const
    {
        return (uint64_t)ptr - (uint64_t)beg;
    }

    Position CalulatePosition() const
    {
        Position position;
        position.line = 0;
        position.character = 0;
        for (size_t i = 0; i < Offset(); i++)
        {
            if (beg[i] == '\r' && beg[i + 1] == '\n')
            {
                position.line++;
                position.character = 0;
                i++;
            }
            else if (beg[i] == '\r' || beg[i] == '\n')
            {
                position.line++;
                position.character = 0;
                i++;
            }
            else
            {
                position.character++;
            }
        }
        return position;
    }

    char *FindLine(uint32_t line, uint32_t &len) const
    {
        uint32_t current = 0;
        size_t i;
        for (i = 0; i < size; i++)
        {
            if (beg[i] == '\r' && beg[i + 1] == '\n')
            {
                current++;
                i++;
            }
            else if (beg[i] == '\r' || beg[i] == '\n')
            {
                current++;
                i++;
            }
            else if (current == line)
            {
                uint32_t j;
                for (j = 0; *(beg + i + j) != '\r' && *(beg + i + j) != '\n' && *(beg + i + j) != '\0'; j++)
                    ;
                len = j;
                return beg + i;
            }
        }
        return nullptr;
    }

    bool End()
    {
        return (uint64_t)beg + size < (uint64_t)ptr;
    }

    const auto &GetFilename() const
    {
        return fileName;
    }

private:
    PointerType ptr;
    PointerType beg;
    std::streamsize size;
    std::string fileName;
};

class TokenList
{
public:
    using iterator = TokenIterator;

private:
    uint32_t tok_size;
    uint32_t tok_capacity;
    Token *tokens;

public:
    TokenList() : tok_size{0}, tok_capacity{0}, tokens{nullptr}
    {
    }

    TokenList(const TokenList &v)
    {
        tok_size = v.tok_size;
        tok_capacity = v.tok_capacity;
        tokens = new Token[tok_size];
        for (unsigned int i = 0; i < tok_size; i++)
            tokens[i] = v.tokens[i];
    }

    TokenList &operator=(const TokenList &v)
    {
        delete[] tokens;
        tok_size = v.tok_size;
        tok_capacity = v.tok_capacity;
        tokens = new Token[tok_size];
        for (unsigned int i = 0; i < tok_size; i++)
            tokens[i] = v.tokens[i];
        return *this;
    }

    ~TokenList()
    {
        delete[] tokens;
    }

    const iterator begin() const
    {
        return iterator(tokens);
    }

    const iterator end() const
    {
        return iterator(tokens + tok_size);
    }

    const Token &front() const
    {
        return *tokens;
    }

    const Token &back() const
    {
        return tokens[tok_size - 1];
    }

    const uint32_t size() const { return tok_size; }

    void add(const Token &v)
    {
        if (tok_size >= tok_capacity)
            reserve(tok_capacity + 5);
        tokens[tok_size++] = v;
    }

    void clear()
    {
        tok_size = 0;
        tok_capacity = 0;
        if (tokens != nullptr)
        {
            delete[] tokens;
            tokens = nullptr;
        }
    }

private:
    void reserve(uint32_t capacity)
    {
        if (tokens == nullptr)
        {
            tok_size = 0;
            tok_capacity = 0;
        }
        Token *Newbuffer = new Token[capacity];
        uint32_t l_Size = capacity < tok_size ? capacity : tok_size;

        for (uint32_t i = 0; i < l_Size; i++)
            Newbuffer[i] = tokens[i];

        tok_capacity = capacity;
        delete[] tokens;
        tokens = Newbuffer;
    }

    void resize(uint32_t size)
    {
        reserve(size);
        tok_size = size;
    }
};

const char *TokenTypeString(TokenType e);

std::ostream &operator<<(std::ostream &stream, const TokenType &e);
std::ostream &operator<<(std::ostream &stream, const Token &e);