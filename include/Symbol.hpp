#pragma once

#include <string>

enum class Type
{
  INTEGER,
  BOOLEAN,
  UNDEFINED
};

enum class Kind
{
  SCALAR,
  ARRAY,
  PROCEDURE,
  FUNCTION,
  UNDEFINED
};

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

std::string kindToStr(Kind kind)
{
  switch (kind)
  {
    case Kind::SCALAR:
      return "scalar";
    case Kind::ARRAY:
      return "array";
    case Kind::PROCEDURE:
      return "procedure";
    case Kind::FUNCTION:
      return "function";
    default:
      return "";
      break;
  }
}
class Symbol
{
 public:
  Symbol(std::string name)
      : m_name{std::move(name)},
        m_lexicalLevel{-1},
        m_orderNum{-1},
        m_type{Type::UNDEFINED},
        m_kind{Kind::UNDEFINED}
  {
  }

  Symbol() = default;
  Symbol(Symbol &&) = default;
  Symbol(const Symbol &) = default;
  Symbol &operator=(Symbol &&) = default;
  Symbol &operator=(const Symbol &) = default;
  ~Symbol() = default;

  void setName(std::string name) { m_name = std::move(name); }

  void setLexicalLevel(int ll) { m_lexicalLevel = ll; }

  // set lexical level and order number
  void setLLON(int ll, int on)
  {
    m_lexicalLevel = ll;
    m_orderNum = on;
  }

  void setType(Type type) { m_type = type; }

  void setKind(Kind kind) { m_kind = kind; }

  std::string getName() const { return m_name; }

  int getLexicalLevel() const { return m_lexicalLevel; }

  int getOrderNum() const { return m_orderNum; }

  Type getType() const { return m_type; }

  Kind getKind() const { return m_kind; }

  std::string getTypeStr() const { return typeToStr(m_type); }

  std::string getKindStr() const { return kindToStr(m_kind); }

 private:
  std::string m_name;
  int m_lexicalLevel;
  int m_orderNum;
  Type m_type;
  Kind m_kind;
};
