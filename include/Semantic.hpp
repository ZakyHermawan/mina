#pragma once

#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>

#include "Symbol.hpp"

class Semantic
{
 public:
  Semantic() : m_lexicalLevel{-1}, m_errorCount{0} {}

  Semantic(Semantic&&) = default;
  Semantic(const Semantic&) = default;
  Semantic& operator=(Semantic&&) = default;
  Semantic& operator=(const Semantic&) = default;
  ~Semantic() = default;

  void printSymbolTable(int ll) const
  {
    for (auto it = m_symbol.begin(); it != m_symbol.end(); ++it)
    {
      auto& bucket = it->second;
      if (bucket.getLexicalLevel() == ll)
      {
        std::cout << "Identifier: " << bucket.getName() << std::endl;
        std::cout << "Lexical Level: " << bucket.getLexicalLevel() << std::endl;
        std::cout << "Order Number: " << bucket.getOrderNum() << std::endl;
        std::cout << "Type: " << bucket.getTypeStr() << std::endl;
        std::cout << "Kind: " << bucket.getKindStr() << std::endl;
        std::cout << std::endl;
      }
    }
  }

  void setPrintOpt(bool opt) { m_printOpt = opt; }

  void setCurrentStr(std::string currStr) { m_currStr = currStr; }

  void setCurrentLine(int line) { m_currLine = line; }

  // Check if identifier m_currStr with lexical level  m_lexicalLevel exist
  // in symbol table
  bool isExistSymLL()
  {
    auto it = m_symbol.find(m_currStr);
    if (it != m_symbol.end())
    {
      auto& bucket = it->second;
      if (bucket.getLexicalLevel() == m_lexicalLevel)
      {
        return true;
      }
    }
    return false;
  }

  // Enter scope
  void S0()
  {
    ++m_lexicalLevel;
    m_orderNum = 0;
  }

  // Print symbol table at current scope if print option is enabled
  void S1()
  {
    if (m_printOpt)
    {
      printSymbolTable(m_lexicalLevel);
    }
  }

  // Exit scope
  void S2()
  {
    --m_lexicalLevel;
    m_orderNum = 0;
  }

  // Check if current identifier is never declared in current scope
  // print error message if the identifier is already being declared before
  void S3()
  {
    if (isExistSymLL())
    {
      std::cout << "Variable already being declared at line " << m_currLine
                << " : " << m_currStr << std::endl;
      ++m_errorCount;
      std::cout << m_errorCount << " errorrs detected, Exiting..." << std::endl;
      exit(1);
    }
    else
    {
      m_symbol.emplace(m_currStr, Symbol(m_currStr));
    }
    m_symbolStack.push(m_currStr);
  }

  // Set lexical level
  void S4()
  {
    auto it = m_symbol.find(m_currStr);
    if (it != m_symbol.end())
    {
      it->second.setLLON(m_lexicalLevel, m_orderNum);
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_currStr << " did not exist\n";
    }
  }

  // Insert type to symbol table
  void S5()
  {
    auto it = m_symbol.find(m_currStr);
    if (it != m_symbol.end())
    {
      it->second.setType(m_typeStack.top());
    }
  }

  // Make sure identifier is already being declared,
  // push m_currStr to symbol table
  void S6()
  {
    auto it = m_symbol.find(m_currStr);
    if (it != m_symbol.end())
    {
      m_symbolStack.push(m_currStr);
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_currStr << " did not exist\n";
    }
  }

  // Pop index symbol table
  void S7() { m_symbolStack.pop(); }

  // Push type of current identifier into stack
  void S8()
  {
    auto type = m_symbol[m_currStr].getType();
    m_typeStack.push(type);
  }

  // Push INTEGER type
  void S9() { m_typeStack.push(Type::INTEGER); }

  // Push BOOLEAN type
  void S10() { m_typeStack.push(Type::BOOLEAN); }

  // Pop type
  void S11() { m_typeStack.pop(); }

  // Check if the type is INTEGER
  void S12()
  {
    if (m_typeStack.top() == Type::BOOLEAN)
    {
      ++m_errorCount;
      std::cout << "Error: Expected INTEGER type at line " << m_currLine
                << " : " << m_currStr;
    }
    else if (m_typeStack.top() == Type::UNDEFINED)
    {
      ++m_errorCount;
      std::cout << "Error: Undefined type at line " << m_currLine << " : "
                << m_currStr;
    }
  }

  // Check if the type is BOOLEAN
  void S13()
  {
    if (m_typeStack.top() == Type::INTEGER)
    {
      ++m_errorCount;
      std::cout << "Error: Expected BOOLEAN type at line " << m_currLine
                << " : " << m_currStr;
    }
    else if (m_typeStack.top() == Type::UNDEFINED)
    {
      ++m_errorCount;
      std::cout << "Error: Undefined type at line " << m_currLine << " : "
                << m_currStr;
    }
  }

  // Check type for equality comparison
  void S14()
  {
    auto t1 = m_typeStack.top();
    m_typeStack.pop();
    auto t2 = m_typeStack.top();
    if (t1 != t2)
    {
      ++m_errorCount;
      std::cout << "Error: Type mismatch between " << typeToStr(t1) << " and "
                << typeToStr(t2) << std::endl;
    }
    m_typeStack.push(t1);
  }

  // Check type for order comparison (example: GT, GTE, LESS)
  // Only allow integer for this comparison
  void S15()
  {
    auto t1 = m_typeStack.top();
    m_typeStack.pop();
    auto t2 = m_typeStack.top();
    if (t1 != Type::INTEGER || t2 != Type::INTEGER)
    {
      ++m_errorCount;
      std::cout << "Error: Type mismatch between " << typeToStr(t1) << " and "
                << typeToStr(t2) << std::endl;
    }
    m_typeStack.push(t1);
  }

  // Check type for assignment
  void S16()
  {
    auto it = m_symbol.find(m_symbolStack.top());
    if (it != m_symbol.end())
    {
      auto bucket = it->second;
      if (bucket.getType() != m_typeStack.top())
      {
        ++m_errorCount;
        std::cout << "Error: Type mismatch between "
                  << typeToStr(bucket.getType()) << " and "
                  << typeToStr(m_typeStack.top()) << std::endl;
      }
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_symbolStack.top() << " did not exist\n";
    }
  }

  // Check if type is INTEGER
  void S17()
  {
    auto it = m_symbol.find(m_symbolStack.top());
    if (it != m_symbol.end())
    {
      if (it->second.getType() != Type::INTEGER)
      {
        ++m_errorCount;
        std::cout << "Error: Type of integer expected at line " << m_currLine
                  << " : " << m_currStr << std::endl;
      }
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_symbolStack.top() << " did not exist\n";
    }
  }

  // Set identifier at symbol table with SCALAR
  void S18()
  {
    auto it = m_symbol.find(m_currStr);
    if (it != m_symbol.end())
    {
      auto& bucket = it->second;
      bucket.setKind(Kind::SCALAR);
      ++m_orderNum;
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_symbolStack.top() << " did not exist\n";
    }
  }

  // Set identifier at symbol table with ARRAY
  void S19()
  {
    auto it = m_symbol.find(m_currStr);
    if (it != m_symbol.end())
    {
      auto& bucket = it->second;
      bucket.setKind(Kind::ARRAY);
      m_orderNum += 3;
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_symbolStack.top() << " did not exist\n";
    }
  }

  // Check if the type is SCALAR
  void S20()
  {
    auto it = m_symbol.find(m_symbolStack.top());
    if (it != m_symbol.end())
    {
      auto& bucket = it->second;
      auto kind = bucket.getKind();

      if (kind == Kind::UNDEFINED)
      {
        ++m_errorCount;
        std::cout << "Variable undefined at line " << m_currLine << " : "
                  << m_currStr << std::endl;
      }
      else if (kind != Kind::SCALAR)
      {
        ++m_errorCount;
        std::cout << "Scalar variable expected at line " << m_currLine << " : "
                  << m_currStr << std::endl;
      }
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_symbolStack.top() << " did not exist\n";
    }
  }

  // Check if the type is ARRAY
  void S21()
  {
    auto it = m_symbol.find(m_symbolStack.top());
    if (it != m_symbol.end())
    {
      auto& bucket = it->second;
      auto kind = bucket.getKind();

      switch (kind)
      {
        case Kind::ARRAY:
          break;
        case Kind::UNDEFINED:
          ++m_errorCount;
          std::cout << "Variable undefined at line " << m_currLine << " : "
                    << m_currStr << std::endl;
          break;
        default:
          ++m_errorCount;
          std::cout << "Array variable expected at line " << m_currLine << " : "
                    << m_currStr << std::endl;
          break;
      }
    }
    else
    {
      ++m_errorCount;
      std::cout << "Variable " << m_symbolStack.top() << " did not exist\n";
    }
  }

 private:
  bool m_printOpt;
  int m_lexicalLevel;
  int m_orderNum;
  int m_errorCount;
  int m_currLine;
  std::string m_currStr;
  std::stack<std::string> m_symbolStack;
  std::stack<Type> m_typeStack;
  std::unordered_map<std::string, Symbol> m_symbol;
};
