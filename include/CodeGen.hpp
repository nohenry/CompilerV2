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

class TemplateNode;

struct TemplateCodeValue : public CodeType
{
    const TemplateNode &node;

    TemplateCodeValue(llvm::StructType *type, const TemplateNode &node) : CodeType(type), node{node}
    {
    }

    ~TemplateCodeValue()
    {
    }

    llvm::StructType *Template() const
    {
        return static_cast<llvm::StructType *>(type);
    }

    const TemplateNode &GetNode() { return node; }
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
    ActionNode,
};

class SymbolNode;

template <class T>
concept IsSymbolNode = std::is_base_of<SymbolNode, T>::value;

class SymbolNode
{
public:
    std::unordered_map<std::string, SymbolNode *> children;
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

    const auto findSymbol(const std::string &name) const
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

    auto IndexOf(std::unordered_map<std::string, SymbolNode *>::const_iterator node) const
    {
        std::unordered_map<std::string, SymbolNode *>::const_iterator begin = children.begin();
        return std::distance(begin, node);
    }

    auto GetParent() { return parent; }
    const auto &GetChildren() const { return children; }

    void AddChild(const std::string &symbolName, SymbolNode *child)
    {
        children.insert(std::pair<std::string, SymbolNode *>(symbolName, child));
    }

    template <IsSymbolNode T, typename... Args>
    T *AddChild(const std::string &symbolName, Args... args)
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
        uint64_t scope = 0;
        do
        {
            s = "$";
            s += std::to_string(scope++);
        } while (findSymbol(s) != children.end());
        return s;
    }

    template <typename T>
    const T &As() const { return *dynamic_cast<const T *>(this); }

    template <typename T>
    T &As() { return *dynamic_cast<T *>(this); }

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
    const CodeValue *variable;

public:
    VariableNode(SymbolNode *parent, const CodeValue *variable) : SymbolNode{parent}, variable{variable} {}
    virtual ~VariableNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::VariableNode; }

    const auto GetVariable() const { return variable; }
};

class TemplateNode : public SymbolNode
{
private:
    TemplateCodeValue *templ;
    std::vector<llvm::Type *> members;

public:
    TemplateNode(SymbolNode *parent, llvm::StructType *templ) : SymbolNode{parent}, templ{new TemplateCodeValue(templ, *this)} {}
    virtual ~TemplateNode()
    {
        delete templ;
    }

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

class ActionNode : public SymbolNode
{
private:
    std::string type;

public:
    ActionNode(SymbolNode *parent, const std::string &type) : SymbolNode{parent}, type{type} {}
    virtual ~ActionNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::ActionNode; }
    const auto &GetTypename() const { return type; }
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
    FunctionCodeValue *currentFunction = nullptr;
    CodeValue *currentVar = nullptr;

    std::bitset<64> usings;
    SymbolNode *syms; 

public:
    CodeGeneration(const std::string &moduleName) : builder{context}, module{new llvm::Module(moduleName, context)}
    {
        insertPoint = rootSymbols.AddChild<ModuleNode>(moduleName);
        syms = &rootSymbols;
    }

public:
    auto &GetContext() { return context; }
    auto &GetBuilder() { return builder; }
    auto &GetModule() const { return *module; }

    CodeType *LiteralType(const Parsing::SyntaxNode &node);
    CodeType *TypeType(const Parsing::SyntaxNode &node);
    const CodeValue *Cast(const CodeValue *value, CodeType *toType, bool implicit = true);
    static uint8_t GetNumBits(uint64_t val);
    TemplateCodeValue *TypeFromObjectInitializer(const Parsing::SyntaxNode &object);
    CodeValue *FollowDotChain(const Parsing::SyntaxNode &);

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
    SymbolNode *FindSymbolInScope(const std::string &name)
    {
        for (auto look = insertPoint; look != nullptr; look = look->parent)
        {
            auto current = look->findSymbol(name);
            if (current != look->GetChildren().end())
                return current->second;
        }
        return nullptr;
    }

    template <IsSymbolNode T>
    T *FindSymbolInScope(const std::string &name)
    {
        if (auto sym = FindSymbolInScope(name))
            if (auto tsym = dynamic_cast<T *>(sym))
                return tsym;
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
            std::string nname = p->GetType() == SymbolNodeType::ActionNode ? p->As<ActionNode>().GetTypename() : p->GetParent()->findSymbol(p);
            s = std::to_string(nname.size()) + nname + s;
            p = p->GetParent();
        }
        s = "_ZN" + s;
        return s;
    }

    std::string GenerateMangledTypeName(const std::string &name)
    {
        std::string s = name;
        for (auto p = insertPoint; p != nullptr && p->GetParent() != nullptr;)
        {
            std::string nname = p->GetParent()->findSymbol(insertPoint);
            if (nname != "")
                s = nname + "." + s;
            p = p->GetParent();
        }
        return s;
    }

    void Return(llvm::Value *val = nullptr)
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