#include <ModuleUnit.hpp>

ErrorList ModuleUnit::errors;
FileIterator *ModuleUnit::globalFptr = nullptr;

ModuleUnit::ModuleUnit(const std::string &filename, const std::string &moduleName) : filename{filename},
                                                                                     moduleName{moduleName},
                                                                                     fptr{filename},
                                                                                     tokenizer{std::make_unique<Tokenizer>(fptr)},
                                                                                     parser{nullptr},
                                                                                     generation{nullptr}
{
    globalFptr = &fptr;
}

ModuleUnit::~ModuleUnit()
{
    // delete generation.rootSymbols;
}

void ModuleUnit::Compile()
{
    Logging::Log("    {} {}", color::bold(color::green("Compiling")), filename);

    auto &tokenList = tokenizer->Tokenize();
    // for (const auto &token : tokenList)
    // {
    //     std::cout << token << std::endl;
    // }

    if (tokenizer->IsDirty())
        return;

    parser = std::make_unique<Parser>(tokenList);
    auto syntaxTree = parser->Parse();

    for (const auto &token : tokenList)
    {
        std::cout << token << " " << &token << std::endl;
    }

    parser->PrintNode(*syntaxTree);

    generation = std::make_unique<CodeGeneration>(moduleName);
    generation->Use(CodeGeneration::Using::NoBlock);

    generation->SetPreCodeGenPass(0); // First pre gen pass (All types)
    syntaxTree->PreCodeGen(*generation);

    generation->SetPreCodeGenPass(1); // Second pre gen pass (All functions)
    syntaxTree->PreCodeGen(*generation);

    auto gen = syntaxTree->CodeGen(*generation); // Generate the llvm code from the syntax tree
    generation->GenerateMain();                  // Generate libc main

    PrintSymbols(generation->rootSymbols);
}

void ModuleUnit::DumpIR()
{
    std::error_code error;
    llvm::raw_fd_ostream outputFile("out.ll", error);

    if (error)
    {
        llvm::errs() << "Could not open file: " << error.message();
        return;
    }

    generation->GetModule().print(outputFile, nullptr);

    outputFile.close();
}