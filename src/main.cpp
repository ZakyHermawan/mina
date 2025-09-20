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
int main(int argc, char* argv[])
{
    CodeHolder code;
    JitRuntime rt;
    asmjit::StringLogger logger;
    code.init(rt.environment(), rt.cpu_features());
    code.set_logger(&logger);
    auto cc = x86::Compiler(&code);
    
    FuncNode* dummy = cc.new_func(FuncSignature::build<int>());
    
    FuncNode* empty = cc.new_func(FuncSignature::build<int>());
    cc.add_func(empty);
    
    x86::Gp g = cc.new_gp64("a");
    
    InvokeNode* invoke_node;
    cc.invoke(Out(invoke_node), dummy->label(),
              FuncSignature::build<int>());

    invoke_node->set_ret(0, g);
    cc.mov(x86::rax, g);

    cc.end_func();
    
    cc.add_func(dummy);
    x86::Gp u = cc.new_gp64("a");
    cc.mov(u, 12);
    cc.ret(u);
    cc.end_func();

    cc.finalize();

    using eF = int (*)();
    eF f;
    rt.add(&f, &code);
    std::cout << "generated code: " << logger.content().data() << std::endl;
    auto a = f();
    std::cout << a << std::endl;

    runFile("C:\\Users\\zakyh\\source\\repos\\mina\\samples\\tes5.txt");
    return 0;
}
