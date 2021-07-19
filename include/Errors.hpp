#pragma once

// #include <stdexcept>
#include <exception>
#include <string>
#include <sstream>
#include <Token.hpp>

enum class ErrorType
{
    Expression,
    FunctionDecleration
};

enum class ErrorCode
{
    NoLeft,
    NotDefault,
    NoLeftParen,
    ExpectedType
};

class ParsingError : public std::exception
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
    ParsingError(ErrorType type,
                 ErrorCode code,
                 const std::string &message,
                 const FileIterator &iter,
                 Range range,
                 std::string fileName,
                 int lineNumber,
                 bool leaf) noexcept : type{type}, code{code}, message{message}, iterator{iter}, range{range}, fileName{fileName}, lineNumber{lineNumber}, leaf{leaf}
    {
    }

    const auto &GetFileIterator() const noexcept { return iterator; }
    const auto &GetRange() const noexcept { return range; }
    const auto &GetFileName() const noexcept { return fileName; }
    const auto GetLineNumber() const noexcept { return lineNumber; }
    const auto GetErrorType() const noexcept { return type; }
    const auto GetErrorCode() const noexcept { return code; }
    const bool IsLeaf() const noexcept { return leaf; }

    static const char *ErrorCodeString(ErrorCode code)
    {
        switch (code)
        {
        default:
            break;
        }
        return "";
    }

    const std::string GetFullMessage() const
    {
        std::stringstream ss;
        ss << "[" << (uint64_t)code << "]: " << ErrorCodeString(code);
#ifndef NDEBUG

#endif
        return ss.str();
    }
};

class ExpectedTypeError : public ParsingError
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
                      const Token &found) noexcept : ParsingError(type, code, message, iter, range, fileName, lineNumber, leaf), tokenType{tokenType}, found{found}
    {
    }

    const TokenType GetTokenType() const noexcept { return tokenType; }
    const Token &GetFoundToken() const noexcept { return found; }
};

#define ThrowParsingError(t, c, m, r) throw new ParsingError(t, c, m, fptr, r, __FILE__, __LINE__, true)
#define ThrowExpectedType(t, m, expected) throw new ExpectedTypeError(t, ErrorCode::ExpectedType, m, fptr, tokenIterator->position, __FILE__, __LINE__, true, expected, tokenIterator)