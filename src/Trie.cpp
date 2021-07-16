#include <stdint.h>
#include <iostream>
#include <type_traits>
#include <string>
#include <bitset>

#include <Token.hpp>
#include <Trie.hpp>
#include <TrieStructure.inc>

const TokenType TokenTypeNull = TokenType::Eof;
const Token TokenNull = Token();
const Token TokenDisregard = Token(TokenType::Disregard);

size_t trie_size = sizeof(trie) / sizeof(trie[0]);

bool operator==(const TrieNode &lhs, char rhs)
{
    return lhs.c == rhs;
}

bool operator!=(const TrieNode &lhs, char rhs)
{
    return lhs.c != rhs;
}

const char *TokenTypeString(TokenType e)
{

#define ets(type)         \
    case TokenType::type: \
        return #type;     \
        break;
    switch (e)
    {
        ets(Whitespace);
        ets(Disregard);
        ets(Semicolon);
        ets(Integer);
        ets(HexInt);
        ets(OctInt);
        ets(BinInt);
        ets(Floating);
        ets(String);
        ets(Plus);
        ets(DoublePlus);
        ets(PlusEqual);
        ets(Minus);
        ets(DoubleMinus);
        ets(MinusEqual);
        ets(Star);
        ets(StarEqual);
        ets(ForwardSlash);
        ets(SlashEqual);
        ets(LeftParen);
        ets(RightParen);
        ets(LeftCurly);
        ets(RightCurly);
        ets(LeftAngle);
        ets(RightAngle);
        ets(LeftSquare);
        ets(RightSquare);
        ets(BiggerEqual);
        ets(SmallerEqual);
        ets(NotBigger);
        ets(NotSmaller);
        ets(Comma);
        ets(Equal);
        ets(DoubleEqual);
        ets(NotEqual);
        ets(Dot);
        ets(Ampersand);
        ets(AmpersandEquals);
        ets(Percent);
        ets(PercentEqual);
        ets(At);
        ets(Colon);
        ets(LeftShift);
        ets(RightShift);
        ets(TripleShift);
        ets(LeftShiftEquals);
        ets(RightShiftEquals);
        ets(TripleShiftEquals);
        ets(Tilda);
        ets(Carrot);
        ets(CarrotEquals);
        ets(Pipe);
        ets(PipeEquals);
        ets(Not);
        ets(FuncArrow);
        ets(Typeof);
        ets(Asm);
        ets(Branch);
        ets(When);
        ets(In);
        ets(If);
        ets(Elif);
        ets(Else);
        ets(Loop);
        ets(Return);
        ets(Int);
        ets(Uint);
        ets(Uint8);
        ets(Int8);
        ets(Uint16);
        ets(Int16);
        ets(Uint32);
        ets(Int32);
        ets(Uint64);
        ets(Int64);
        ets(Float);
        ets(Float32);
        ets(Float64);
        ets(Char);
        ets(Bool);
        ets(Spec);
        ets(Template);
        ets(True);
        ets(False);
        ets(Import);
        ets(Let);
        ets(Persist);
        ets(Or);
        ets(And);
        ets(Null);
        ets(Module);
        ets(Function);
        ets(Type);
        ets(Identifier);
        ets(Tokens);
        ets(Eof);
    }
#undef ets
    return "";
}

std::ostream &operator<<(std::ostream &stream, const TokenType &e)
{

#define ets(type)         \
    case TokenType::type: \
        stream << #type;  \
        break;
    switch (e)
    {
        ets(Whitespace);
        ets(Disregard);
        ets(Semicolon);
        ets(Integer);
        ets(HexInt);
        ets(OctInt);
        ets(BinInt);
        ets(Floating);
        ets(String);
        ets(Plus);
        ets(DoublePlus);
        ets(PlusEqual);
        ets(Minus);
        ets(DoubleMinus);
        ets(MinusEqual);
        ets(Star);
        ets(StarEqual);
        ets(ForwardSlash);
        ets(SlashEqual);
        ets(LeftParen);
        ets(RightParen);
        ets(LeftCurly);
        ets(RightCurly);
        ets(LeftAngle);
        ets(RightAngle);
        ets(LeftSquare);
        ets(RightSquare);
        ets(BiggerEqual);
        ets(SmallerEqual);
        ets(NotBigger);
        ets(NotSmaller);
        ets(Comma);
        ets(Equal);
        ets(DoubleEqual);
        ets(NotEqual);
        ets(Dot);
        ets(Ampersand);
        ets(AmpersandEquals);
        ets(Percent);
        ets(PercentEqual);
        ets(At);
        ets(Colon);
        ets(LeftShift);
        ets(RightShift);
        ets(TripleShift);
        ets(LeftShiftEquals);
        ets(RightShiftEquals);
        ets(TripleShiftEquals);
        ets(Tilda);
        ets(Carrot);
        ets(CarrotEquals);
        ets(Pipe);
        ets(PipeEquals);
        ets(Not);
        ets(FuncArrow);
        ets(Typeof);
        ets(Asm);
        ets(Branch);
        ets(When);
        ets(In);
        ets(If);
        ets(Elif);
        ets(Else);
        ets(Loop);
        ets(Return);
        ets(Int);
        ets(Uint);
        ets(Uint8);
        ets(Int8);
        ets(Uint16);
        ets(Int16);
        ets(Uint32);
        ets(Int32);
        ets(Uint64);
        ets(Int64);
        ets(Float);
        ets(Float32);
        ets(Float64);
        ets(Char);
        ets(Bool);
        ets(Spec);
        ets(Template);
        ets(True);
        ets(False);
        ets(Import);
        ets(Let);
        ets(Persist);
        ets(Or);
        ets(And);
        ets(Null);
        ets(Module);
        ets(Function);
        ets(Type);
        ets(Identifier);
        ets(Tokens);
        ets(Eof);
    }
#undef ets
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const Token &e)
{
    stream << e.type;
    switch (e.type)
    {
    case TokenType::Integer:
    {
        stream << " " << e.ivalue;
        break;
    }
    case TokenType::Identifier:
    {
        stream << " " << e.raw;
        break;
    }
    case TokenType::HexInt:
    {
        stream << " 0x" << std::hex << e.ivalue << std::dec;
        break;
    }
    case TokenType::OctInt:
    {
        stream << " 0q" << std::oct << e.ivalue << std::dec;
        break;
    }
    case TokenType::BinInt:
    {

        stream << " 0b" << std::bitset<64>(e.ivalue);
        break;
    }
    case TokenType::Floating:
    {
        stream << " " << e.fvalue;
        break;
    }
    case TokenType::String:
    {
        stream << " '" << e.raw << "'";
        break;
    }
    default:
        break;
    }

    stream << " " << e.position.start.line << ":" << e.position.start.character << "-" << e.position.end.line << ":" << e.position.end.character;

    return stream;
}