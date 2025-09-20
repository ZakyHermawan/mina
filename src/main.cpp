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

int main(int argc, char* argv[]) {
    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes5.txt");
    return 0;
}


/*
#include <iostream>
#include <stdio.h> // For printf
#include <asmjit/asmjit.h>

// Define the signature of the JIT-compiled function we're creating.
typedef int (*MyFunc)();

int main() {
    using namespace asmjit;

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());
    x86::Assembler a(&code);

    // 1. Prepare arguments in C++
    // We need the address of printf and our format string.
    uint64_t printf_address = (uint64_t)&printf;
    const char* format_string = "The number is: %d\n";
    int number_to_print = 42;

    // 2. Set up arguments according to the x86-64 System V ABI (Linux)
    // First argument (the format string) goes into RDI.
    a.mov(x86::rdi, (uint64_t)format_string);
    // Second argument (the number) goes into RSI.
    a.mov(x86::rsi, number_to_print);
    
    // For variadic functions like printf, RAX should be 0 (number of vector registers used).
    a.xor_(x86::rax, x86::rax); 

    // 3. Align the stack to 16 bytes (critical for ABI compliance!)
    // The `call` instruction pushes an 8-byte return address, un-aligning the stack.
    // We subtract 8 bytes beforehand to ensure it's aligned inside the called function.
    a.sub(x86::rsp, 8);

    // 4. Make the call
    // We can't call by name, so we load the address into a register and call that.
    a.mov(x86::r10, printf_address); // Use a temporary register like r10
    a.call(x86::r10);

    // 5. Restore the stack
    a.add(x86::rsp, 8);
    
    a.ret(); // Return from our JIT function

    // Finalize and execute the code
    MyFunc jit_func;
    Error err = rt.add(&jit_func, &code);
    if (err != asmjit::Error::kOk)
    {
        std::cerr << "AsmJit failed: " << DebugUtils::error_as_string(err) << std::endl;
        return 1;
    }

    std::cout << "Executing JIT code..." << std::endl;
    jit_func(); // This will execute the printf call
    std::cout << "JIT code finished." << std::endl;

    rt.release(jit_func);
    return 0;
}

*/