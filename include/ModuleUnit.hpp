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
    ~ModuleUnit()
    {
        // delete generation.rootSymbols;
    }

    const auto &GetSyntaxTree() const { return syntaxTree; }
    virtual std::shared_ptr<CodeValue> CodeGen()
    {
        generation.Use(CodeGeneration::Using::NoBlock);

        generation.SetPreCodeGenPass(0);
        syntaxTree.PreCodeGen(generation);

        generation.SetPreCodeGenPass(1);
        syntaxTree.PreCodeGen(generation);

        // PrintSymbols(generation.rootSymbols);

        auto gen = syntaxTree.CodeGen(generation);
        generation.GenerateMain();

        return gen;
    }

    const auto &GetGen() const { return generation; }
};