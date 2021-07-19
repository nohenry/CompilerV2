#pragma once

#include <stdint.h>
#include <Token.hpp>

struct TrieNode
{
    unsigned char c;
    uint16_t i : 9;
    uint8_t n : 2;
    uint8_t type;
    bool top : 1;
    bool term : 1;

    TrieNode(unsigned char c, uint16_t i, uint8_t n, TokenType type, bool top, bool term) : c(c), i{i}, n{n}, type{static_cast<uint8_t>(type)}, top{top}, term{term}
    {
    }

    TrieNode(unsigned char c, uint16_t i, uint8_t n) : c(c), i{i}, n{n}, type{static_cast<uint8_t>(TokenType::Eof)}, top{false}, term{false}
    {
    }
    TrieNode(unsigned char c, uint16_t i, uint8_t n, bool top) : c(c), i{i}, n{n}, type{static_cast<uint8_t>(TokenType::Eof)}, top{top}, term{false}
    {
    }

    TrieNode(unsigned char c, TokenType type) : c(c), i{0}, n{0}, type{static_cast<uint8_t>(type)}, top{false}, term{true}
    {
    }

    TrieNode(unsigned char c, TokenType type, bool top) : c(c), i{0}, n{0}, type{static_cast<uint8_t>(type)}, top{top}, term{true}
    {
    }

    inline const TokenType GetType() const
    {
        return static_cast<TokenType>(type);
    }
};

extern const TrieNode trie[];
extern size_t trie_size;
extern const TokenType TokenTypeNull;
extern const Token TokenNull;
extern const Token TokenDisregard;

bool operator==(const TrieNode &lhs, char rhs);
bool operator!=(const TrieNode &lhs, char rhs);