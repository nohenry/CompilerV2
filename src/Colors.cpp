#include <Colors.hpp>
#include <iostream>

std::ostream &operator<<(std::ostream &stream, const Colors &colors)
{
    stream << colors.value();
    return stream;
}