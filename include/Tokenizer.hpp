#pragma once

#include <stdint.h>
#include <Token.hpp>
#include <string>
#include <cstdio>
#include <tuple>

#include <Trie.hpp>

class Tokenizer
{
private:
    using IterateType = std::tuple<Token, uint8_t>;

private:
    TokenList tokenList;
    std::string filePath;
    FileIterator fptr;
    bool dirty = false;

public:
    Tokenizer(std::string filePath) : filePath{filePath}, fptr{filePath} {}
    ~Tokenizer() {}

    const TokenList &Tokenize();
    IterateType Identifier();
    IterateType Integer();
    IterateType Float();
    bool CheckPrimitiveTypeSize(const Token &integer);
    IterateType String();
    bool Comment();
    IterateType IterateTrie();
    IterateType IterateTrieRec(const TrieNode &node, bool use, Position startPosition);
    const bool IsDirty() const { return dirty; }
    const auto &GetFileIterator() const { return fptr; }
};