#pragma once
#include <iostream>
#include <iomanip>
#include <Colors.hpp>
#include <Token.hpp>
#include <string>
#include <utility>

enum class FormatMode
{
    Hex,
    Dec
};

struct FormatOptions
{
    FormatMode mode;
    uint8_t padding;
    uint8_t precision;
    FormatOptions() : mode{FormatMode::Dec}, padding(0), precision(0) {}
    FormatOptions(FormatMode mode) : mode{mode}, padding(0), precision(0) {}
};

std::pair<uint64_t, char *> StringToValue(const char *str);

template <size_t Stream = 0>
void PrintFormat(std::string s)
{
    if (Stream)
        std::cerr << s;
    else
        std::cout << s;
}

template <size_t Stream = 0, typename T, typename... Args>
void PrintFormat(std::string s, T value, Args... args)
{
    FormatOptions op;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '{' && s[i + 1] != '{')
        {
            FormatOptions options;
            uint8_t n = 0;
            for (; s[i] != '}'; i++)
            {
                switch (s[i])
                {
                case 'x':
                    options.mode = FormatMode::Hex;
                    break;
                case '.':
                    n++;
                    break;
                case '0' ... '9':
                {
                    switch (n)
                    {
                    case 0:
                    {
                        auto val = StringToValue(s.substr(i).c_str());
                        options.padding = static_cast<uint8_t>(val.first);
                        s = val.second - 1;
                        break;
                    }
                    case 1:
                    {
                        auto val = StringToValue(s.substr(i).c_str());
                        options.precision = static_cast<uint8_t>(val.first);
                        s = val.second - 1;
                        break;
                    }
                    }
                    break;
                }
                }
            }
            if (Stream)
                std::cerr << value;
            else
                std::cout << value;
            i++;
            PrintFormat<Stream>(s.substr(i), args...);
            return;
        }
        if (Stream)
            std::cerr << s[i];
        else
            std::cout << s[i];
    }
}

class Logging
{
public:
    static void Log(std::string str)
    {
        PrintFormat(str);
        PrintFormat("\n");
    }

    template <typename T, typename... Args>
    static void Log(std::string str, T value, Args... args)
    {
        PrintFormat(str, value, args...);
        PrintFormat("\n");
    }

    static void Warn(std::string str)
    {
        PrintFormat<1>(color::yellow("warning: "));
        PrintFormat<1>(str);
        PrintFormat<1>("\n");
    }

    template <typename T, typename... Args>
    static void Warn(std::string str, T value, Args... args)
    {
        PrintFormat<1>(color::yellow("warning: "));
        PrintFormat<1>(str, value, args...);
        PrintFormat<1>("\n");
    }

    static void Error(std::string str)
    {
        PrintFormat<1>(color::bold(color::red("error")));
        PrintFormat<1>(color::bold(color::white(": ")));
        PrintFormat<1>(str);
        PrintFormat<1>("\n");
    }

    template <typename T, typename... Args>
    static void Error(std::string str, T value, Args... args)
    {
        PrintFormat<1>(color::bold(color::red("error")));
        PrintFormat<1>(color::bold(color::white(": ")));
        PrintFormat<1>(str, value, args...);
        PrintFormat<1>("\n");
    }

    static void CharacterSnippet(const FileIterator &file)
    {
        uint32_t len;
        auto pos = file.CalulatePosition();
        char *line = file.FindLine(pos.line, len);

        uint32_t lineN = pos.line;
        uint32_t nDigits = 0;

        do
        {
            nDigits++;
            lineN /= 10;
        } while (lineN);

        std::cerr << color::bold(color::cyan("  --> ")) << file.GetFilename() << ":" << (pos.line + 1) << ":" << (pos.character + 1) << std::endl;

        for (size_t i = 0; i < nDigits + 1; i++)
            std::cerr << " ";
        std::cerr << color::bold(color::cyan("|")) << std::endl;

        std::cerr << Colors::stylize(96) << Colors::stylize(1)
                  << std::left << std::setw(nDigits + 1) << (pos.line + 1)
                  << color::bold(color::cyan("| "))
                  << std::string(line, len)
                  << std::endl;
        for (size_t i = 0; i < nDigits + 1; i++)
            std::cerr << " ";

        std::cerr << color::bold(color::cyan("| "));
        for (size_t i = 0; i < pos.character; i++)
        {
            std::cerr << " ";
        }
        std::cerr << color::bold(color::red("^")) << std::endl;
    }

    static void CharacterSnippet(const FileIterator &file, const Range position)
    {
        uint32_t len;
        char *line = file.FindLine(position.start.line, len);

        uint32_t lineN = position.start.line;
        uint32_t nDigits = 0;

        do
        {
            nDigits++;
            lineN /= 10;
        } while (lineN);

        std::cerr << color::bold(color::cyan("  --> ")) << file.GetFilename() << ":" << (position.start.line + 1) << ":" << (position.start.character + 1) << std::endl;

        for (size_t i = 0; i < nDigits + 1; i++)
            std::cerr << " ";
        std::cerr << color::bold(color::cyan("|")) << std::endl;

        std::cerr << Colors::stylize(96) << Colors::stylize(1)
                  << std::left << std::setw(nDigits + 1) << (position.start.line + 1)
                  << color::bold(color::cyan("| "))
                  << std::string(line, len)
                  << std::endl;
        for (size_t i = 0; i < nDigits + 1; i++)
            std::cerr << " ";

        std::cerr << color::bold(color::cyan("| "));
        for (size_t i = 0; i < position.start.character; i++)
        {
            std::cerr << " ";
        }
        for (size_t i = position.start.character; i < position.end.character; i++)
        {
            std::cerr << color::bold(color::red("^"));
        }
        std::cerr << std::endl;
    }

    static void SampleSnippet(const FileIterator &file, const Range editPosition, const std::string &insert)
    {
        uint32_t len;
        char *line = file.FindLine(editPosition.start.line, len);

        uint32_t lineN = editPosition.start.line;
        uint32_t nDigits = 0;

        do
        {
            nDigits++;
            lineN /= 10;
        } while (lineN);

        std::cerr << color::bold(color::green("  --> ")) << file.GetFilename() << std::endl;

        for (size_t i = 0; i < nDigits + 1; i++)
            std::cerr << " ";
        std::cerr << color::bold(color::green("|")) << std::endl;

        std::string fullLine(line, len);

        auto first = fullLine.substr(0, editPosition.start.character);
        auto second = fullLine.substr(editPosition.end.character);

        auto newline = first + insert + second;

        std::cerr << Colors::stylize(96) << Colors::stylize(1)
                  << std::left << std::setw(nDigits + 1) << (editPosition.start.line + 1)
                  << color::bold(color::green("| "))
                  << newline
                  << std::endl;

        for (size_t i = 0; i < nDigits + 1; i++)
            std::cerr << " ";

        std::cerr << color::bold(color::green("| "));
        // for (size_t i = 0; i < position.character; i++)
        // {
        //     std::cerr << " ";
        // }
        // for (size_t i = position.start.character; i < position.end.character; i++)
        // {
        //     std::cerr << color::bold(color::red("^"));
        // }
        std::cerr << std::endl;
    }
};