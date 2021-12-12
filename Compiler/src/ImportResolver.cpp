#include "ImportResolver.h"
#include "Expressions.h"
#include "./utils/files.h"
#include "./utils/strings.h"

namespace Hunter::Compiler {

    void ImportResolver::ResolveImports(const std::string & basePath, AbstractSyntaxTree *tree) {
        auto & instructions = tree->GetInstructions();
        for (int i = 0; i < instructions.size(); ++i) {
            Expression * instruction = instructions.at(i);

            if (auto * importExpr = dynamic_cast<ImportExpression *>(instruction)) {
                instructions.erase(instructions.begin() + i);

                std::string module = importExpr->GetModule();
                replaceAll(module, ".", "/");
                std::string moduleFilePath = basePath + "/" + module + ".hunt";

                Parser parser;
                AbstractSyntaxTree * ast = parser.Parse(moduleFilePath);
                auto & importedInstructions = ast->GetInstructions();

                auto * moduleInstr = dynamic_cast<ModuleExpression *>(importedInstructions.at(0));
                if (!moduleInstr) {
                    std::cerr << "First instruction is not the module" << std::endl;
                    exit(1);
                }

                auto storedModuleName = moduleInstr->GetModule();

                for (const auto &importedInstr : importedInstructions) {
                    if (auto * funcExpr = dynamic_cast<FunctionExpression *>(importedInstr)) {
                        funcExpr->SetName(storedModuleName + "." + funcExpr->GetName());
                    }

                    // todo: support function calls of the module function in its own functions
                }

                instructions.insert(instructions.begin(), importedInstructions.begin()+1, importedInstructions.end());
            }
        }
    }

}