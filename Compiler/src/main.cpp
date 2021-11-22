#include "Parser.h"
#include "CodeGenerator.h"
#include "Compiler.h"
#include <iostream>
#include <fstream>
#include <sstream>

// https://llvm.org/docs/GettingStarted.html#llvm-examples


std::string readFileIntoString(const std::string& path) {
    std::ifstream input_file(path);
    if (!input_file.is_open()) {
        std::cerr << "Could not open the file - '" << path << "'" << std::endl;
        exit(EXIT_FAILURE);
    }
    return std::string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
}

int main(int argc, const char ** argv) {

    if (argc < 2) {
        std::cerr << "Did not find parameter for file" << std::endl;
        exit(1);
    }

    Hunter::Compiler::Parser parser;
    Hunter::Compiler::CodeGenerator codeGenerator;

    // todo: handle 1 character variable

    std::string input = readFileIntoString(std::string(argv[1]));

    Hunter::Compiler::AbstractSyntaxTree * ast = parser.Parse(input);
    ast->Dump();

    llvm::Module * module = codeGenerator.GenerateCode(ast);
    Hunter::Compiler::CompileModule(module);

    return 0;
}
