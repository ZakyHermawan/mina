/*
#include <fstream>
#include <sstream>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Token.hpp"

void repl()
{
  while (1)
  {
    std::string source;
    std::cout << "repl> ";
    getline(std::cin, source);
    if (std::cin.eof())
    {
      break;
    }
    Parser parser(std::move(source));
    Token currToken;

    parser.program();
  }
}

void runFile(const char* fileName)
{
  std::ifstream fileStream;
  fileStream.open(fileName);
  std::string source;
  if (fileStream.is_open())
  {
    std::stringstream ss;
    ss << fileStream.rdbuf();
    source = ss.str();
  }

  Parser parser(std::move(source));
  Token currToken;
  parser.program();
}

int main(int argc, char* argv[])
{
  // uncomment this line and comment the line before return statement if you
  // dont want to hardcode the source code location.

  //if (argc == 1)
  //{
  //  repl();
  //}
  //else if (argc == 2)
  //{
  //  runFile(argv[1]);
  //}
  //else
  //{
  //  std::cerr << "Usage: ./mina [path]\n";
  //}

    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes.txt");
    return 0;
}
*/


#define ASMJIT_STATIC
#include <stdio.h>
#include <asmjit/asmjit.h>

#include <fstream>
#include <sstream>

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Token.hpp"

static void repl()
{
    while (1)
    {
        std::string source;
        std::cout << "repl> ";
        getline(std::cin, source);
        if (std::cin.eof())
        {
            break;
        }
        Parser parser(std::move(source));
        Token currToken;
        parser.program();
    }
}

static void runFile(const char* fileName)
{
    std::ifstream fileStream;
    fileStream.open(fileName);
    std::string source;
    if (fileStream.is_open())
    {
        std::stringstream ss;
        ss << fileStream.rdbuf();
        source = ss.str();
    }

    Parser parser(std::move(source));
    Token currToken;
    parser.program();
}

using namespace asmjit;

// Signature of the generated function.
// It takes no arguments and returns an integer.
typedef int (*Func)(void);
//void func_a(int val)
//{
//  printf("func_a(): Called %d\n", val);
//}

int main(int argc, char* argv[]) {
    FileLogger logger(stdout);  // Logger should always survive CodeHolder.
    JitRuntime rt;
    CodeHolder code;

    code.init(rt.environment(), rt.cpuFeatures());
    code.setLogger(&logger); // attach logger

    x86::Compiler cc(&code);
    cc.addFunc(FuncSignature::build<int>());
    x86::Gp i = cc.newIntPtr("i");
    //Imm func_a_addr = Imm((void*)func_a);
    //cc.mov(asmjit::x86::rcx, 's');
    // 2. Emit the call instruction.
    // Since func_a() takes no arguments, we don't need to set up
    // any registers like RDI or RCX.
    //cc.call(putchar);
    asmjit::x86::Mem stack_var = cc.newStack(8, 4);
    x86::Mem stackIdx(stack_var);  // Copy of stack with i added.
    //stackIdx.setIndex(i);      // stackIdx <- stack[i].
    //stackIdx.setSize(4);       // stackIdx <- byte ptr stack[i].

    //// Create a register to hold the pointer.
    asmjit::x86::Gp ptr_reg = cc.newGpq("ptr_reg");

    //// Use LEA to load the effective address of the stack variable
    //// into our pointer register.
    cc.lea(ptr_reg, stack_var);
    //cc.xor_(x86::rax, x86::rax);

    cc.mov(x86::ax, 10);
    cc.mov(x86::cl, 2);
    cc.idiv(x86::ax, x86::cl);

    cc.ret();
    cc.endFunc();
    cc.finalize();

    Func fn;
    Error err = rt.add(&fn, &code);

    if (err) {
        printf("AsmJit failed: %s\n", DebugUtils::errorAsString(err));
        return 1;
    }



    // Execute the generated function.
    int result = fn();
    printf("The function returned: %d\n", result); // Expected output: 100

    rt.release(fn);

    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes5.txt");

    return 0;
}
