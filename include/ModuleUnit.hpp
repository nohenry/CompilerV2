#pragma once
#include <Parser.hpp>
#include <CodeGen.hpp>
#include <Tokenizer.hpp>

#include <string>

using namespace Parsing;

class ModuleUnit
{
public:
    static ErrorList errors;

private:
    static FileIterator *globalFptr;

private:
    std::string filename;
    std::string moduleName;

    FileIterator fptr;

    std::unique_ptr<Tokenizer> tokenizer;
    std::unique_ptr<Parser> parser;
    std::unique_ptr<CodeGeneration> generation;

public:
    ModuleUnit(const std::string &filename, const std::string &moduleName);
    ~ModuleUnit();

    void Compile();
    void DumpIR();
    
    static FileIterator &GetFptr() { return *globalFptr; }
};