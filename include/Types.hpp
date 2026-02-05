#pragma once

#include "MachineIR.hpp"

#include <string>
#include <vector>
#include <memory>

namespace mina
{

enum class Type
{
    INTEGER,
    BOOLEAN,
    UNDEFINED
};

enum class IdentType
{
    VARIABLE,
    ARRAY
};

enum class FType
{
    PROC,
    FUNC,
};

std::string typeToStr(Type type);

}  // namespace mina
