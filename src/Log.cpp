#include <Log.hpp>

void PrintFormat(std::string s, std::ostream &stream)
{
    stream << s;
}

std::pair<uint64_t, char *> StringToValue(const char *str)
{
    uint64_t val = 0;
    if (*str == '\0' || *str > '9' || *str < '0')
    {
        return std::make_pair(val, (char *)str);
    }
    auto ret = StringToValue(str + 1);

    uint64_t distance = (uint64_t)ret.second - (uint64_t)str;
    if (distance == 1)
    {
        val = (*str - '0');
    }
    else
    {
        val += ret.first;
        val += (*str - '0') * pow(10, distance - 2);
    }

    return std::make_pair(val, ret.second);
}