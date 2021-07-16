#include <llvm/IR/Type.h>
#include <Tokenizer.hpp>
#include <Parser.hpp>
#include <Windows.h>

int main()
{
    Tokenizer tokenizer("examples/input.dsl");
    // Tokenizer tokenizer("examples/test.dsl");
    const auto &tokenList = tokenizer.Tokenize();
    for (const auto &token : tokenList)
    {
        std::cout << token << std::endl;
    }
    
    if(!tokenizer.IsDirty()) {
        Parsing::Parser parser(tokenList, tokenizer.GetFileIterator());
        parser.Parse();
    }
    return 0;
}