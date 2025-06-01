#pragma once

#include <string>

enum class Type
{
  INTEGER,
  BOOLEAN,
  UNDEFINED
};

enum class FType
{
  PROC,
  FUNC,
};

std::string typeToStr(Type type);
