#include "Parser.h"
#include "CodeGenerator.h"
#include "Compiler.h"

// https://llvm.org/docs/GettingStarted.html#llvm-examples

int main() {

    Hunter::Compiler::Parser parser;
    Hunter::Compiler::CodeGenerator codeGenerator;

    std::string input = R"(
        fun hunt()
            const num = 8

            if num eq 8 then
                print("Hello 8\n")
            else
                print("Not hello 8\n")
    )";

    Hunter::Compiler::AbstractSyntaxTree * ast = parser.Parse(input);
    ast->Dump();

    llvm::Module * module = codeGenerator.GenerateCode(ast);
    Hunter::Compiler::CompileModule(module);

    return 0;
}
