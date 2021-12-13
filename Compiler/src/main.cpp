#include "Parser.h"
#include "ImportResolver.h"
#include "CodeGenerator.h"
#include "Compiler.h"
#include "./utils/files.h"
#include "./utils/logger.h"

#include <filesystem>

int main(int argc, const char ** argv) {

    if (argc < 2) {
        std::cerr << "Did not find parameter for file" << std::endl;
        exit(1);
    }

    Hunter::Compiler::Logger::Init();

    Hunter::Compiler::Parser parser;
    Hunter::Compiler::ImportResolver importResolver;
    Hunter::Compiler::CodeGenerator codeGenerator;

    if (argc > 3) {
        if (strcmp(argv[2], "--output-ir") == 0) {
            codeGenerator.SetDebugOutputFile(argv[3]);
        }
    }

    // todo: handle 1 character variable

    std::string filePath = std::string(argv[1]);
    Hunter::Compiler::AbstractSyntaxTree * ast = parser.Parse(filePath);
    std::vector<Hunter::Compiler::Expression *> emptyInstructionList;
    importResolver.ResolveImports(std::filesystem::path(filePath).parent_path(), ast, emptyInstructionList);
    ast->SetInstructions(emptyInstructionList);

    ast->Dump();

    // todo: validate ast -> like return values matching return type

    llvm::Module * module = codeGenerator.GenerateCode(ast);
    Hunter::Compiler::CompileModule(module);

    return 0;
}
