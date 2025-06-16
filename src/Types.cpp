#include "Types.hpp"

#include <string>

std::string typeToStr(Type type)
{
    switch (type)
    {
        case Type::INTEGER:
            return "integer";
        case Type::BOOLEAN:
            return "boolean";
        case Type::UNDEFINED:
            return "undefined";
        default:
            break;
    }
}
