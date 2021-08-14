#pragma once

// #include <stdexcept>
#include <exception>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>

#include <Token.hpp>

enum class ErrorType
{
    Expression,
    FunctionDecleration,
    Expect,
    ActionStatement,
    ReferenceType,
    Literal,
    TopLevelScope,
    TemplateScope,
    SpecScope,
    ActionScope,
    TemplateVariableDecleration,
    SpecVariableDecleration,
    SampleSnippet,
    IfStatement,

    Cast
};

enum class ErrorCode
{
    NoLeft,
    NotDefault,
    NoLeftParen,
    ExpectedType,
    NoVarInAction,
    ReferenceToReference,
    UnknownLiteral,
    InvalidStatement,
    VariableInitializer,
    SampleSnippet,
    ExprBodyOnly,
    ElseAfterElse,
    NoType,

    NoImplicitCast
};

class BaseException
{
public:
    virtual ~BaseException() {}

    template <typename T>
    const T &As() const { return *dynamic_cast<const T *>(this); }
};

class CompilerError : public BaseException
{
private:
    ErrorType type;
    ErrorCode code;
    std::string message;
    FileIterator iterator;
    Range range;
    std::string fileName;
    int lineNumber;
    bool leaf;

public:
    CompilerError(ErrorType type,
                  ErrorCode code,
                  const std::string &message,
                  const FileIterator &iter,
                  Range range,
                  std::string fileName,
                  int lineNumber,
                  bool leaf) noexcept : type{type}, code{code}, message{message}, iterator{iter}, range{range}, fileName{fileName}, lineNumber{lineNumber}, leaf{leaf}
    {
    }

    virtual ~CompilerError() {}

    const auto &GetFileIterator() const noexcept { return iterator; }
    const auto &GetRange() const noexcept { return range; }
    const auto &GetMessage() const noexcept { return message; }
    const auto &GetFileName() const noexcept { return fileName; }
    const auto GetLineNumber() const noexcept { return lineNumber; }
    const auto GetErrorType() const noexcept { return type; }
    const auto GetErrorCode() const noexcept { return code; }
    const bool IsLeaf() const noexcept { return leaf; }

    static const char *ErrorCodeString(ErrorType type, ErrorCode code)
    {
        switch (type)
        {
        case ErrorType::ActionStatement:
            switch (code)
            {
            case ErrorCode::NoVarInAction:
                return "Variable cannot be declared in action statement!";
            default:
                break;
            }
            break;
        default:
            break;
        }
        return "";
    }

    const std::string GetFullMessage() const
    {
        std::stringstream ss;
        ss << "[" << (uint64_t)code << "]: " << ErrorCodeString(type, code);
#ifndef NDEBUG

#endif
        return ss.str();
    }
};

class ExpectedTypeError : public CompilerError
{
private:
    TokenType tokenType;
    const Token found;

public:
    ExpectedTypeError(ErrorType type,
                      ErrorCode code,
                      const std::string &message,
                      const FileIterator &iter,
                      Range range,
                      std::string fileName,
                      int lineNumber,
                      bool leaf,
                      TokenType tokenType,
                      const Token &found) noexcept : CompilerError(type, code, message, iter, range, fileName, lineNumber, leaf), tokenType{tokenType}, found{found}
    {
    }
    virtual ~ExpectedTypeError() {}

    const TokenType GetTokenType() const noexcept { return tokenType; }
    const Token &GetFoundToken() const noexcept { return found; }
};

class SampleSuggestion : public CompilerError
{
private:
    Range pos;
    std::string insert;

public:
    SampleSuggestion(ErrorType type,
                     ErrorCode code,
                     const std::string &message,
                     const FileIterator &iter,
                     Range range,
                     std::string fileName,
                     int lineNumber,
                     bool leaf,
                     Range pos,
                     const std::string &insert) noexcept : CompilerError(type, code, message, iter, range, fileName, lineNumber, leaf), pos{pos}, insert{insert}
    {
    }
    virtual ~SampleSuggestion() {}

    const Range GetPosition() const noexcept { return pos; }
    const std::string &GetInsert() const noexcept { return insert; }
};

class ErrorList
{
public:
    using iterator = std::vector<BaseException *>::const_iterator;

private:
    std::vector<BaseException *> excpetions;

public:
    ErrorList()
    {
    }

    const iterator begin() const
    {
        return excpetions.begin();
    }

    const iterator end() const
    {
        return excpetions.end();
    }

    const uint32_t size() const { return excpetions.size(); }

    void add(BaseException *v)
    {
        excpetions.push_back(v);
    }


    void clear()
    {
        excpetions.clear();
    }
};

/**
 * @brief This macro creates and throws a standard compiler error
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * @param r the range to be underlined when displayed
 * 
 * @throws CompilerError the created exception
 * 
 */
#define ThrowCompilerError(t, c, m, r)                                                                 \
    {                                                                                                  \
        auto ___errr = new CompilerError(t, c, m, ModuleUnit::GetFptr(), r, __FILE__, __LINE__, true); \
        ModuleUnit::errors.add(___errr);                                                               \
        throw *___errr;                                                                                \
    }

/**
 * @brief This macro creates and throws a compiler error with the ability
 *        to create a snippet to help the user fix their error
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * @param r the range to be underlined when displayed
 * @param s the snippet which should be created with `CreateSampleSnippet`
 * 
 * @throws CompilerError the created exception
 * 
 */
#define ThrowCompilerErrorSnippet(t, c, m, r, s)                                                       \
    {                                                                                                  \
        auto ___errr = new CompilerError(t, c, m, ModuleUnit::GetFptr(), r, __FILE__, __LINE__, true); \
        ModuleUnit::errors.add(___errr);                                                               \
        s;                                                                                             \
        throw *___errr;                                                                                \
    }

/**
 * @brief This macro creates and throws a compiler error that originates
 *        in code that doesn't terminate. As in it will call other functions
 *        that may throw excpetions
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * @param r the range to be underlined when displayed
 * 
 * @throws CompilerError the created exception
 * 
 */
#define ThrowMidCompilerError(t, c, m, r)                                                               \
    {                                                                                                   \
        auto ___errr = new CompilerError(t, c, m, ModuleUnit::GetFptr(), r, __FILE__, __LINE__, false); \
        ModuleUnit::errors.add(___errr);                                                                \
        throw *___errr;                                                                                 \
    }

/**
 * @brief This macro creates and throws a compiler error with a snippet that originates
 *        in code that doesn't terminate. As in it will call other functions
 *        that may throw excpetions
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * @param s the snippet which should be created with `CreateSampleSnippet`
 * 
 * @throws CompilerError the created exception
 * 
 */
#define ThrowMidCompilerErrorSnippet(t, c, m, r, s)                                                     \
    {                                                                                                   \
        auto ___errr = new CompilerError(t, c, m, ModuleUnit::GetFptr(), r, __FILE__, __LINE__, false); \
        ModuleUnit::errors.add(___errr);                                                                \
        s;                                                                                              \
        throw *___errr;                                                                                 \
    }

/**
 * @brief This macro creates and throws an expected type compiler error
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * 
 * @throws CompilerError the created exception
 * 
 */
#define ThrowExpectedType(t, m, expected)                                                                                                                                       \
    {                                                                                                                                                                           \
        auto ___errr = new ExpectedTypeError(t, ErrorCode::ExpectedType, m, ModuleUnit::GetFptr(), tokenIterator->position, __FILE__, __LINE__, true, expected, *tokenIterator); \
        ModuleUnit::errors.add(___errr);                                                                                                                                        \
        throw *___errr;                                                                                                                                                         \
    }

/**
 * @brief This macro creates and throws an expected type compiler error with snippet
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * 
 * @throws CompilerError the created exception
 * 
 */
#define ThrowExpectedTypeSnippet(t, m, expected, s)                                                                                                                             \
    {                                                                                                                                                                           \
        auto ___errr = new ExpectedTypeError(t, ErrorCode::ExpectedType, m, ModuleUnit::GetFptr(), tokenIterator->position, __FILE__, __LINE__, true, expected, *tokenIterator); \
        ModuleUnit::errors.add(___errr);                                                                                                                                        \
        s;                                                                                                                                                                      \
        throw *___errr;                                                                                                                                                         \
    }

/**
 * @brief This macro creates a snippet of code to be displayed to the user
 *        to help them understand what's wrong or to give them an exmple of what to do
 * 
 * @param t the type of error
 * @param c the error code
 * @param m the message of the error
 * 
 * @throws CompilerError the created exception
 * 
 */
#define CreateSampleSnippet(pos, ins) \
    ModuleUnit::errors.add(new SampleSuggestion(ErrorType::SampleSnippet, ErrorCode::SampleSnippet, "", ModuleUnit::GetFptr(), tokenIterator->position, __FILE__, __LINE__, true, pos, ins))