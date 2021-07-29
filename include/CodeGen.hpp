#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <map>
#include <string>
#include <bitset>

std::string gen_random(const int len);
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
        auto child = new T(this, args...);
        children.insert(std::pair<std::string, SymbolNode *>(symbolName, child));
        return child;
    }

    std::string GenerateName()
    {
        std::string s;
        do
        {
            s = "$";
            s += gen_random(5);
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
public:
    TemplateNode(SymbolNode *parent) : SymbolNode{parent} {}
    virtual ~TemplateNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::TemplateNode; }
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
        NoBlock
    };

public:
    static SymbolNode rootSymbols;

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module *module;

    SymbolNode *insertPoint = nullptr;
    CodeValue *currentFunction;

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
    auto NewScope(std::string name, Args... args)
    {
        return (insertPoint = insertPoint->AddChild<T>(name, args...));
    }

    // Same as previous function but generators symbol name
    template <IsSymbolNode T, typename... Args>
    auto NewScope()
    {
        auto name = insertPoint->GenerateName();
        return (insertPoint = insertPoint->AddChild<T>(name));
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

    void SetCurrentFunction(CodeValue *func)
    {
        currentFunction = func;
    }

    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Type *type, const std::string &VarName)
    {
        auto func = static_cast<llvm::Function *>(currentFunction->value);
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

    void Use(Using toUse) { usings.set((uint64_t)toUse); }
    bool IsUsed(Using toUse) { return usings.test((uint64_t)toUse); }
    void UnUse(Using toUnUse) { usings.reset((uint64_t)toUnUse); }
};

std::ostream &operator<<(std::ostream &stream, const SymbolNodeType &e);
std::ostream &operator<<(std::ostream &stream, const SymbolNode &node);