#include "Parser.h"
#include "ImportResolver.h"
#include "CodeGenerator.h"
#include "Compiler.h"
#include "./utils/files.h"

#include <filesystem>

int main(int argc, const char ** argv) {

    if (argc < 2) {
        std::cerr << "Did not find parameter for file" << std::endl;
        exit(1);
    }

    Hunter::Compiler::Parser parser;
    Hunter::Compiler::ImportResolver importResolver;
    Hunter::Compiler::CodeGenerator codeGenerator;

    if (argc > 2) {
        if (strcmp(argv[2], "--output-ir") == 0) {
            codeGenerator.SetDebugOutputFile("output.txt");
        }
    }

    // todo: handle 1 character variable

    std::string filePath = std::string(argv[1]);
    std::string input = readFileIntoString(filePath);

    Hunter::Compiler::AbstractSyntaxTree * ast = parser.Parse(input);
    importResolver.ResolveImports(std::filesystem::path(filePath).parent_path(), ast);

    ast->Dump();

    llvm::Module * module = codeGenerator.GenerateCode(ast);
    Hunter::Compiler::CompileModule(module);

    return 0;
}
