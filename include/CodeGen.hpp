#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <map>
#include <string>
#include <bitset>

// std::string gen_random(const int len);
// #include <Parser.hpp>
namespace Parsing
{
    class SyntaxNode;
} // namespace Parsing

struct CodeType
{
    llvm::Type *type;
    bool isSigned;

    CodeType(llvm::Type *type, bool isSigned = false) : type{type}, isSigned{isSigned}
    {
    }
};

struct CodeValue
{
    llvm::Value *value;
    CodeType *type;

    CodeValue(llvm::Value *value, CodeType *type) : value{value}, type{type}
    {
    }
};

struct FunctionCodeValue : public CodeValue
{
private:
    llvm::AllocaInst *retLoc;
    llvm::BasicBlock *retLabel;
    int numRets = 0;

public:
    llvm::Value *lastStoreValue = nullptr;
    llvm::StoreInst *lastStore = nullptr;
    llvm::BranchInst *lastbr = nullptr;

public:
    FunctionCodeValue(llvm::Value *value, CodeType *type, llvm::AllocaInst *retLoc = nullptr, llvm::BasicBlock *retLabel = nullptr) : CodeValue(value, type), retLoc{retLoc}, retLabel{retLabel}
    {
    }

    llvm::Function *Function()
    {
        return static_cast<llvm::Function *>(value);
    }

    llvm::Function *Function() const
    {
        return static_cast<llvm::Function *>(value);
    }

    auto GetReturnLocation() { return retLoc; }
    void SetReturnLocation(llvm::AllocaInst *ret) { retLoc = ret; }
    auto GetNumRets() { return numRets; }
    auto GetReturnLabel() { return retLabel; }
    void IncNumRets() { numRets++; }
};

class CodeGeneration;

enum class SymbolNodeType
{
    SymbolNode,
    PackageNode,
    ModuleNode,
    FunctionNode,
    VariableNode,
    TemplateNode,
    TypeAliasNode,
    ScopeNode,
};

class SymbolNode;

template <class T>
concept IsSymbolNode = std::is_base_of<SymbolNode, T>::value;

class SymbolNode
{
protected:
    std::map<std::string, SymbolNode *> children;
    SymbolNode *parent;
    bool isExported = false;

public:
    SymbolNode(SymbolNode *parent) : parent{parent} {}
    virtual ~SymbolNode()
    {
        for (auto &c : children)
            delete c.second;
    }

    const virtual SymbolNodeType GetType() const { return SymbolNodeType::SymbolNode; }

    const auto findSymbol(std::string name) const
    {
        return children.find(name);
    }

    const auto findSymbol(SymbolNode *child) const
    {
        for (auto &c : children)
        {
            if (c.second == child)
                return c.first;
        }
        return std::string("");
    }

    const auto GetParent() const { return parent; }
    const auto &GetChildren() const { return children; }

    void AddChild(std::string symbolName, SymbolNode *child)
    {
        children.insert(std::pair<std::string, SymbolNode *>(symbolName, child));
    }

    template <IsSymbolNode T, typename... Args>
    T *AddChild(std::string symbolName, Args... args)
    {
        if (findSymbol(symbolName) != children.end())
        {
            // TODO throw error symbol already exists
        }
        auto child = new T(this, args...);
        children.insert(std::pair<std::string, SymbolNode *>(symbolName, child));
        return child;
    }

    std::string GenerateName()
    {
        std::string s;
        uint64_t scope;
        do
        {
            s = "$";
            s += std::to_string(scope++);
        } while (findSymbol(s) != children.end());
        return s;
    }

    template <typename T>
    const T &As() const { return *dynamic_cast<const T *>(this); }

    void Export() { isExported = true; }
    void NoExport() { isExported = false; }
    bool IsExported() const { return isExported; }
};

class PackageNode : public SymbolNode
{

public:
    PackageNode(SymbolNode *parent) : SymbolNode{parent} {}
    virtual ~PackageNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::PackageNode; }
};

class ModuleNode : public SymbolNode
{
private:
    CodeGeneration *moduleInfo;

public:
    ModuleNode(SymbolNode *parent) : SymbolNode{parent} {}
    virtual ~ModuleNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::ModuleNode; }

    auto GetModuleInfo() { return moduleInfo; }
};

class FunctionNode : public SymbolNode
{
private:
    CodeValue *function;

public:
    FunctionNode(SymbolNode *parent, CodeValue *function) : SymbolNode{parent}, function{function} {}
    virtual ~FunctionNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::FunctionNode; }

    const auto GetFunction() const { return function; }
};

class VariableNode : public SymbolNode
{
private:
    CodeValue *variable;

public:
    VariableNode(SymbolNode *parent, CodeValue *variable) : SymbolNode{parent}, variable{variable} {}
    virtual ~VariableNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::VariableNode; }

    const auto GetVariable() const { return variable; }
};

class TemplateNode : public SymbolNode
{
private:
    llvm::StructType *templ;
    std::vector<llvm::Type *> members;

public:
    TemplateNode(SymbolNode *parent, llvm::StructType *templ) : SymbolNode{parent}, templ{templ} {}
    virtual ~TemplateNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::TemplateNode; }

    const auto GetTemplate() const { return templ; }
    void AddMember(llvm::Type *val) { members.push_back(val); }
    auto GetMembers() { return members; }
};

class TypeAliasNode : public SymbolNode
{
private:
public:
    TypeAliasNode(SymbolNode *parent) : SymbolNode{parent} {}
    virtual ~TypeAliasNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::TypeAliasNode; }
};

class ScopeNode : public SymbolNode
{
private:
public:
    ScopeNode(SymbolNode *parent) : SymbolNode{parent} {}
    virtual ~ScopeNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::ScopeNode; }
};

void PrintSymbols(const SymbolNode &node, const std::string &name = "root", int index = 0, const std::wstring &indent = L"", bool last = false);

class CodeGeneration
{

public:
    enum class Using
    {
        Export,
        NoBlock,
        Reference
    };

public:
    static SymbolNode rootSymbols;

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module *module;

    SymbolNode *insertPoint = nullptr;
    FunctionCodeValue *currentFunction;
    CodeValue *currentVar;

    std::bitset<64> usings;

public:
    CodeGeneration(const std::string &moduleName) : builder{context}, module{new llvm::Module(moduleName, context)}
    {
        insertPoint = rootSymbols.AddChild<ModuleNode>(moduleName);
    }

public:
    auto &GetContext() { return context; }
    auto &GetBuilder() { return builder; }
    auto &GetModule() const { return *module; }

    CodeType *LiteralType(const Parsing::SyntaxNode &node);
    CodeType *TypeType(const Parsing::SyntaxNode &node);
    const CodeValue *Cast(const CodeValue *value, CodeType *toType, bool implicit = true);
    static uint8_t GetNumBits(uint64_t val);

    // Creates a new scope and sets the insert point for new symbols
    template <IsSymbolNode T, typename... Args>
    T *NewScope(std::string name, Args... args)
    {
        return static_cast<T *>((insertPoint = insertPoint->AddChild<T>(name, args...)));
    }

    // Same as previous function but generators symbol name
    template <IsSymbolNode T, typename... Args>
    T *NewScope()
    {
        auto name = insertPoint->GenerateName();
        return static_cast<T *>((insertPoint = insertPoint->AddChild<T>(name)));
    }

    // Clean up a scope
    auto LastScope()
    {
        return (insertPoint = insertPoint->GetParent());
    }

    auto GetInsertPoint()
    {
        return insertPoint;
    }

    // Iterate backwards the symbol tree to see if the specified symbol name  exits
    SymbolNode *FindSymbolInScope(std::string name)
    {
        auto look = insertPoint;
        for (; look != nullptr; look = insertPoint->GetParent())
        {
            auto current = look->findSymbol(name);
            if (current != look->GetChildren().end())
                return current->second;
        }
        return nullptr;
    }

    const auto GetCurrentFunction() const
    {
        return currentFunction;
    }

    void SetCurrentFunction(FunctionCodeValue *func)
    {
        currentFunction = func;
    }

    const auto GetCurrentVar() const
    {
        return currentVar;
    }

    void SetCurrentVar(CodeValue *var)
    {
        currentVar = var;
    }

    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Type *type, const std::string &VarName, llvm::Function *func = nullptr)
    {
        if (!func)
            func = static_cast<llvm::Function *>(currentFunction->value);
        llvm::IRBuilder<> TmpB(&func->getEntryBlock(), func->getEntryBlock().begin());
        return TmpB.CreateAlloca(type, nullptr, VarName);
    }

    std::string GenerateMangledName(const std::string &name)
    {
        std::string s = std::to_string(name.size()) + name;
        for (auto p = insertPoint; p != nullptr && p->GetParent() != nullptr;)
        {
            std::string nname = insertPoint->GetParent()->findSymbol(insertPoint);
            s = std::to_string(nname.size()) + nname + s;
            p = p->GetParent();
        }
        s = "_ZN10" + s;
        return s;
    }

    void Return(llvm::Value *val)
    {
        if (currentFunction && currentFunction->GetReturnLocation())
        {
            currentFunction->lastStoreValue = val;
            currentFunction->lastStore = builder.CreateStore(val, currentFunction->GetReturnLocation());
            currentFunction->lastbr = builder.CreateBr(currentFunction->GetReturnLabel());
            currentFunction->IncNumRets();
        }
    }

    void Use(Using toUse) { usings.set((uint64_t)toUse); }
    bool IsUsed(Using toUse) { return usings.test((uint64_t)toUse); }
    void UnUse(Using toUnUse) { usings.reset((uint64_t)toUnUse); }
};

std::ostream &operator<<(std::ostream &stream, const SymbolNodeType &e);
std::ostream &operator<<(std::ostream &stream, const SymbolNode &node);