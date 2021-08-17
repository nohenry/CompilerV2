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
    FileIterator &fptr;
    bool dirty = false;
    int angleIndex = 0;

public:
    Tokenizer(FileIterator &fptr) : fptr{fptr} {}
    ~Tokenizer() {}

    TokenList &Tokenize();
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