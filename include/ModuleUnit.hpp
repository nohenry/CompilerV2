#pragma once
#include <Parser.hpp>
#include <CodeGen.hpp>
#include <string>

using namespace Parsing;

class ModuleUnit
{
private:
    BlockStatement<> syntaxTree;
    CodeGeneration generation;

public:
    ModuleUnit(const std::string &moduleName, const Token &start, const std::vector<Statement> statements, const Token &eof) : syntaxTree{start, statements, eof}, generation{moduleName} {}

    const auto &GetSyntaxTree() const { return syntaxTree; }
    virtual const CodeValue *CodeGen()
    {
        generation.Use(CodeGeneration::Using::NoBlock);
        return syntaxTree.CodeGen(generation);
    }

    const auto &GetGen() const { return generation; }
};