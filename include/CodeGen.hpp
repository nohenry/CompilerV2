#pragma once
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <map>
#include <string>
#include <bitset>
#include <memory>
#include <iostream>

#include <Token.hpp>
#include <Errors.hpp>

// std::string gen_random(const int len);
// #include <Parser.hpp>
namespace Parsing
{
    class SyntaxNode;
    class GenericParameter;
} // namespace Parsing

extern size_t numCodeType;
extern size_t numCodeValue;

struct CodeType
{
    llvm::Type *type;
    bool isSigned;

    CodeType(llvm::Type *type, bool isSigned = false) : type{type}, isSigned{isSigned}
    {
        numCodeType++;
    }

    virtual ~CodeType()
    {
        // type->print(llvm::outs());
        // llvm::outs() << "\n";
        numCodeType--;
    }

    bool operator==(const CodeType &right) const
    {
        return type == right.type && isSigned == right.isSigned;
    }

    bool operator!=(const CodeType &right) const
    {
        return !(*this == right);
    }
};

struct CodeValue
{
    llvm::Value *value;
    std::shared_ptr<CodeType> type;

    CodeValue(llvm::Value *value, std::shared_ptr<CodeType> type) : value{value}, type{type}
    {
        numCodeValue++;
    }

    virtual ~CodeValue()
    {
        // value->print(llvm::outs());
        // llvm::outs() << "\n";
        numCodeValue--;
    }
};

struct FunctionCodeType : public CodeType
{
private:
    std::shared_ptr<CodeType> returnType;
    std::vector<std::shared_ptr<CodeType>> parameters;
    bool isMember;

public:
    FunctionCodeType(
        std::shared_ptr<CodeType> returnType,
        std::vector<std::shared_ptr<CodeType>> parameters,
        bool isMember) : CodeType(nullptr), returnType{returnType}, parameters{parameters}, isMember{isMember}
    {
    }

    const auto &GetParameters() const { return parameters; }
    auto GetReturnType() const { return returnType; }
    const bool IsMember() const { return isMember; }
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
    FunctionCodeValue(
        llvm::Function *value,
        std::shared_ptr<FunctionCodeType> functionType,
        llvm::AllocaInst *retLoc = nullptr,
        llvm::BasicBlock *retLabel = nullptr) : CodeValue(value, functionType), retLoc{retLoc}, retLabel{retLabel}
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
    void SetReturnLabel(llvm::BasicBlock *ret) { retLabel = ret; }
    void IncNumRets() { numRets++; }

    auto GetFunctionType() { return std::static_pointer_cast<FunctionCodeType>(type); }
};

class TemplateNode;

struct TemplateCodeType : public CodeType
{
    TemplateNode &node;

    TemplateCodeType(llvm::Type *type, TemplateNode &node) : CodeType(type), node{node}
    {
    }

    virtual ~TemplateCodeType()
    {
    }

    llvm::StructType *Template() const
    {
        return static_cast<llvm::StructType *>(type);
    }

    TemplateNode &GetNode() { return node; }
};

class SpecNode;

struct SpecCodeType : public CodeType
{
    SpecNode &node;

    SpecCodeType(llvm::Type *type, SpecNode &node) : CodeType(type), node{node}
    {
    }

    virtual ~SpecCodeType()
    {
    }

    llvm::StructType *Spec() const
    {
        return static_cast<llvm::StructType *>(type);
    }

    SpecNode &GetNode() { return node; }
};

struct ArrayCodeType : public CodeType
{
    std::shared_ptr<CodeType> baseType;

    ArrayCodeType(llvm::Type *type, std::shared_ptr<CodeType> baseType) : CodeType(type), baseType{baseType}
    {
    }

    virtual ~ArrayCodeType()
    {
    }

    std::shared_ptr<CodeType> GetBaseType() { return baseType; }
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
    SpecNode
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

    virtual const std::unordered_map<std::string, SymbolNode *>::const_iterator findSymbol(const std::string &name) const
    {
        return children.find(name);
    }

    const std::string findSymbol(SymbolNode *child) const
    {
        for (auto &c : children)
        {
            if (c.second == child)
                return c.first;
        }
        return "";
    }

    auto IndexOf(std::unordered_map<std::string, SymbolNode *>::const_iterator node) const
    {
        std::unordered_map<std::string, SymbolNode *>::const_iterator begin = children.begin();
        int index = 0;
        for (auto &c : children)
        {
            if (c.second->GetType() == SymbolNodeType::VariableNode)
            {
                if (c == *node)
                {
                    break;
                }
                index++;
            }
        }
        return index;
        // return std::distance(begin, node);
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
    std::shared_ptr<FunctionCodeValue> function;

public:
    FunctionNode(SymbolNode *parent, std::shared_ptr<FunctionCodeValue> function) : SymbolNode{parent}, function{function} {}
    virtual ~FunctionNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::FunctionNode; }

    std::shared_ptr<FunctionCodeValue> GetFunction() const { return function; }
};

class VariableNode : public SymbolNode
{
private:
    std::shared_ptr<CodeValue> variable;

public:
    VariableNode(SymbolNode *parent, std::shared_ptr<CodeValue> variable) : SymbolNode{parent}, variable{variable} {}
    virtual ~VariableNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::VariableNode; }

    std::shared_ptr<CodeValue> GetVariable() const { return variable; }
};

// class ActionNode : public SymbolNode
// {
// private:
//     std::string type;

// public:
//     ActionNode(SymbolNode *parent, const std::string &type) : SymbolNode{parent}, type{type} {}
//     virtual ~ActionNode() {}

//     const virtual SymbolNodeType GetType() const override { return SymbolNodeType::ActionNode; }
//     const auto &GetTypename() const { return type; }
// };

class TemplateNode : public SymbolNode
{
private:
    std::shared_ptr<TemplateCodeType> templ;
    std::vector<llvm::Type *> members;
    bool isGeneric = false;
    Parsing::GenericParameter *generic = nullptr;

public:
    TemplateNode(SymbolNode *parent, llvm::StructType *templ) : SymbolNode{parent}, templ{std::make_shared<TemplateCodeType>(templ, *this)} {}
    TemplateNode(SymbolNode *parent, Parsing::GenericParameter *generic) : SymbolNode{parent}, templ{nullptr}, generic{generic}, isGeneric{true} {}

    virtual ~TemplateNode()
    {
    }

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::TemplateNode; }

    // const SymbolNode *findSymbolWithAction(const std::string &name) const
    // {
    //     auto c = children.find(name);
    //     if (c != children.end())
    //         return c->second;

    //     for (auto &action : actions)
    //     {
    //         auto c1 = action->findSymbol(name);
    //         if (c1 != action->children.end())
    //             return c1->second;
    //     }
    //     return nullptr;
    // }

    const auto GetTemplate() const { return templ; }
    void AddMember(llvm::Type *val) { members.push_back(val); }
    auto GetMembers() { return members; }
    bool IsGeneric() const { return isGeneric; }
    // const auto &GetAction() const { return actions; }
    // void AddAction(ActionNode *action) { actions.push_back(action); }
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

class SpecNode : public SymbolNode
{
private:
public:
    SpecNode(SymbolNode *parent) : SymbolNode{parent} {}
    virtual ~SpecNode() {}

    const virtual SymbolNodeType GetType() const override { return SymbolNodeType::SpecNode; }
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
    SymbolNode rootSymbols{nullptr};

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module *module;

    SymbolNode *insertPoint = nullptr;
    std::shared_ptr<FunctionCodeValue> currentFunction;
    std::shared_ptr<CodeValue> currentVar;
    std::shared_ptr<CodeType> currentType;
    std::shared_ptr<CodeValue> dotExprBase;
    Range currentRange;

    std::bitset<64> usings;

    uint8_t preCodeGenPass = 0;

    static ErrorList errors;

public:
    CodeGeneration(const std::string &moduleName) : builder{context}, module{new llvm::Module(moduleName, context)}
    {
        insertPoint = &rootSymbols;
        NewScope<ModuleNode>(moduleName);
    }

    ~CodeGeneration()
    {
    }

public:
    auto &GetContext() { return context; }
    auto &GetBuilder() { return builder; }
    auto &GetModule() const { return *module; }

    std::shared_ptr<CodeType> LiteralType(const Parsing::SyntaxNode &node);
    std::shared_ptr<CodeType> TypeType(const Parsing::SyntaxNode &node);
    std::shared_ptr<CodeValue> Cast(std::shared_ptr<CodeValue> value, std::shared_ptr<CodeType> toType, bool implicit = true);
    static uint8_t GetNumBits(uint64_t val);
    std::shared_ptr<TemplateCodeType> TypeFromObjectInitializer(const Parsing::SyntaxNode &object);
    std::shared_ptr<CodeType> TypeFromArrayInitializer(const Parsing::SyntaxNode &object);
    std::shared_ptr<CodeValue> FollowDotChain(const Parsing::SyntaxNode &);

    void GenerateMain();

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

    void SetInsertPoint(SymbolNode *newIns)
    {
        insertPoint = newIns;
    }

    auto &GetCurrentRange()
    {
        return currentRange;
    }

    void SetCurrentRange(Range range)
    {
        currentRange = range;
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

    // Iterate backwards the symbol tree to see if the specified symbol name  exits
    std::string FindSymbolInScope(SymbolNode *symbol)
    {
        for (auto look = insertPoint; look != nullptr; look = look->parent)
        {
            auto current = look->findSymbol(symbol);
            if (current != "")
                return current;
        }
        return nullptr;
    }

    // template <IsSymbolNode T>
    //  std::string  FindSymbolInScope(T *symbol)
    // {
    //     auto name = FindSymbolInScope(symbol)
    //     if ()

    //     return nullptr;
    // }

    const auto GetCurrentFunction() const
    {
        return currentFunction;
    }

    void SetCurrentFunction(std::shared_ptr<FunctionCodeValue> func)
    {
        currentFunction = func;
    }

    const auto GetCurrentVar() const
    {
        return currentVar;
    }

    void SetCurrentVar(std::shared_ptr<CodeValue> var)
    {
        currentVar = var;
    }

    const auto GetCurrentType() const
    {
        return currentType;
    }

    void SetCurrentType(std::shared_ptr<CodeType> var)
    {
        currentType = var;
    }

    const auto GetDotExprBase() const
    {
        return dotExprBase;
    }

    void SetDotExprBase(std::shared_ptr<CodeValue> var)
    {
        dotExprBase = var;
    }

    const auto GetPreCodeGenPass() { return preCodeGenPass; }
    void SetPreCodeGenPass(uint8_t pass) { preCodeGenPass = pass; }

    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Type *type, const std::string &VarName, llvm::Function *func = nullptr)
    {
        if (!func)
            func = currentFunction->Function();
        llvm::IRBuilder<> TmpB(&func->getEntryBlock(), func->getEntryBlock().begin());
        return TmpB.CreateAlloca(type, nullptr, VarName);
    }

    std::string GenerateMangledName(const std::string &name)
    {
        std::string s = std::to_string(name.size()) + name;
        for (auto p = insertPoint; p != nullptr && p->GetParent() != nullptr;)
        {
            std::string nname = p->GetParent()->findSymbol(p);
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