#include "Parser.h"
#include "CodeGenerator.h"
#include "Compiler.h"

// https://llvm.org/docs/GettingStarted.html#llvm-examples

int main() {

    Hunter::Compiler::Parser parser;
    Hunter::Compiler::CodeGenerator codeGenerator;

    // todo: handle 1 character variable

    std::string input = R"(
        fun hunt()
            for counter in 1..10
                print("Hello #", counter, "\n")
    )";

    Hunter::Compiler::AbstractSyntaxTree * ast = parser.Parse(input);
    ast->Dump();

    llvm::Module * module = codeGenerator.GenerateCode(ast);
    Hunter::Compiler::CompileModule(module);

    return 0;
}
