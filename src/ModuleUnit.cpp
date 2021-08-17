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

    // for (const auto &token : tokenList)
    // {
    //     std::cout << token << " " << &token << std::endl;
    // }

    parser->PrintNode(*syntaxTree);

    try
    {
        generation = std::make_unique<CodeGeneration>(moduleName);

        generation->SetPreCodeGenPass(0); // First pre gen pass (All types)
        syntaxTree->PreCodeGen(*generation);

        generation->SetPreCodeGenPass(1);
        syntaxTree->PreCodeGen(*generation);

        generation->SetPreCodeGenPass(10); // Second pre gen pass (Template members)
        syntaxTree->PreCodeGen(*generation);

        generation->SetPreCodeGenPass(20); // Second pre gen pass (Spec functions)
        syntaxTree->PreCodeGen(*generation);

        generation->SetPreCodeGenPass(30); // Second pre gen pass (Action functions)
        syntaxTree->PreCodeGen(*generation);

        PrintSymbols(generation->rootSymbols);

        generation->Use(CodeGeneration::Using::NoBlock);
        auto gen = syntaxTree->CodeGen(*generation); // Generate the llvm code from the syntax tree
        generation->GenerateMain();                  // Generate libc main
    }
    catch (BaseException &excep)
    {
    }

    for (auto e : errors)
    {
        auto pe = dynamic_cast<CompilerError *>(e);
        if (pe->GetErrorCode() == ErrorCode::ExpectedType)
        {
            auto pet = dynamic_cast<ExpectedTypeError *>(e);
            Logging::Error(color::bold(color::white("Unexpected token {}. Expected {}")), pet->GetFoundToken().raw.c_str(), TokenTypeString(pet->GetTokenType()));
            if (pet->IsLeaf())
                Logging::CharacterSnippet(fptr, pet->GetFoundToken().position);
            Logging::Log("");
            continue;
        }
        else if (pe->GetErrorCode() == ErrorCode::SampleSnippet)
        {
            auto pet = dynamic_cast<SampleSuggestion *>(e);
            Logging::Log(color::bold(color::white("Try using the following:")));
            Logging::SampleSnippet(fptr, pet->GetPosition(), pet->GetInsert());
            Logging::Log("");
            continue;
        }
        switch (pe->GetErrorType())
        {
        default:
            if (pe->IsLeaf())
            {

                Logging::Error(color::bold(color::white(pe->GetMessage())));
                Logging::CharacterSnippet(fptr, pe->GetRange());
            }
            else
            {
                Logging::Error(pe->GetMessage());
            }
            Logging::Log("");
            break;
        }
    }

    // PrintSymbols(generation->rootSymbols);
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