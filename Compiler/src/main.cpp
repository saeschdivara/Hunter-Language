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
            let foo = 1
            while foo <= 10
                print("While #", foo, "\n")
                foo = foo + 1
    )";

    Hunter::Compiler::AbstractSyntaxTree * ast = parser.Parse(input);
    ast->Dump();

    llvm::Module * module = codeGenerator.GenerateCode(ast);
    Hunter::Compiler::CompileModule(module);

    return 0;
}
