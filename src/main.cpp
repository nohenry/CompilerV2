#include <llvm/IR/Type.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/Host.h>
#include <Tokenizer.hpp>
#include <Parser.hpp>
#include <ModuleUnit.hpp>
#include <llvm/IR/Mangler.h>

size_t numCodeType = 2;
size_t numCodeValue = 0;

int main()
{
    ModuleUnit module("examples/input.dsl", "input");
    module.Compile();
    module.DumpIR();

    // mod->GetGen().GetModule().print(outputFile, nullptr);
    // PrintSymbols(mod->GetGen().rootSymbols);

    // auto targetTriple = llvm::sys::getDefaultTargetTriple();

    // llvm::InitializeAllTargetInfos();
    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetMCs();
    // llvm::InitializeAllAsmParsers();
    // llvm::InitializeAllAsmPrinters();

    // std::string Error;
    // auto target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

    // if (!target)
    // {
    //     llvm::errs() << Error;
    //     return 1;
    // }

    // auto targetName = target->getName();
    // llvm::SubtargetFeatures features;
    // llvm::StringMap<bool> hostFeatures;

    // if (llvm::sys::getHostCPUFeatures(hostFeatures))
    //     for (auto &f : hostFeatures)
    //         features.AddFeature(f.first(), f.second);

    // llvm::TargetOptions opt;
    // auto RM = llvm::Optional<llvm::Reloc::Model>();
    // auto CM = llvm::Optional<llvm::CodeModel::Model>();
    // auto optimization = llvm::CodeGenOpt::Level::None;
    // auto targetMachine = target->createTargetMachine(targetTriple, targetName, features.getString(), opt, RM, CM, optimization);
    // auto DL = targetMachine->createDataLayout();

    // llvm::Mangler::getNameWithPrefix(llvm::outs(), "Function", DL);
    // // targetMachine.getDa
    // mod->GetGen().GetModule().setDataLayout(targetMachine->createDataLayout());
    // mod->GetGen().GetModule().setTargetTriple(targetTriple);

    // auto filename = "out.o";
    // llvm::raw_fd_ostream dest(filename, error);

    // if (error)
    // {
    //     llvm::errs() << "Could not open file: " << error.message();
    //     return 1;
    // }

    // llvm::legacy::PassManager pass;
    // auto FileType = llvm::CGFT_ObjectFile;

    // if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
    // {
    //     llvm::errs() << "TargetMachine can't emit a file of this type";
    //     return 1;
    // }

    // pass.run(mod->GetGen().GetModule());
    // dest.flush();

    // std::cout << "Code Value: " << numCodeValue << ", Code Type: " << numCodeType << std::endl;
    return 0;
}