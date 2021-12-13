#include "ImportResolver.h"
#include "Expressions.h"
#include "./utils/logger.h"
#include "./utils/strings.h"

namespace Hunter::Compiler {

    void ImportResolver::ResolveImports(const std::string & basePath, AbstractSyntaxTree *tree, std::vector<Expression *> & instructions) {
        auto treeInstructions = tree->GetInstructions();
        std::string currentModule = "";

        for (int i = 0; i < treeInstructions.size(); ++i) {
            Expression * instruction = treeInstructions.at(i);

            if (auto * moduleExpr = dynamic_cast<ModuleExpression *>(instruction)) {
                currentModule = moduleExpr->GetModule();
                COMPILER_INFO("Current module: \"{0}\"", currentModule);
            } else if (auto * importExpr = dynamic_cast<ImportExpression *>(instruction)) {
                std::string module = importExpr->GetModule();
                if (HasModule(module)) {
                    continue;
                }

                AddModule(module);
                COMPILER_INFO("Import module: \"{0}\"", module);

                Parser parser;
                replaceAll(module, ".", "/");
                std::string moduleFilePath = basePath + "/" + module + ".hunt";
                AbstractSyntaxTree * ast = parser.Parse(moduleFilePath);
                ResolveImports(basePath, ast, instructions);
            } else {
                instructions.push_back(instruction);
            }
        }

        if (!currentModule.empty()) {
            for (const auto &instruction : treeInstructions) {
                if (auto * funcExpr = dynamic_cast<FunctionExpression *>(instruction)) {
                    funcExpr->SetName(currentModule + "." + funcExpr->GetName());
                }
            }
        }


//        for (int i = 0; i < instructions.size(); ++i) {
//            Expression * instruction = instructions.at(i);
//
//            if (auto * importExpr = dynamic_cast<ImportExpression *>(instruction)) {
//                instructions.erase(instructions.begin() + i);
//
//                std::string module = importExpr->GetModule();
//                COMPILER_INFO("Found module import: \"{0}\"", module);
//
//                if (tree->HasModule(module)) {
//                    continue;
//                }
//
//                tree->AddModule(module);
//
//                replaceAll(module, ".", "/");
//                std::string moduleFilePath = basePath + "/" + module + ".hunt";
//
//                Parser parser;
//                AbstractSyntaxTree * ast = parser.Parse(moduleFilePath);
//                auto & importedInstructions = ast->GetInstructions();
//
//                auto * moduleInstr = dynamic_cast<ModuleExpression *>(importedInstructions.at(0));
//                if (!moduleInstr) {
//                    std::cerr << "First instruction is not the module" << std::endl;
//                    exit(1);
//                }
//
//                auto storedModuleName = moduleInstr->GetModule();
//
//                for (int j = 0; j < importedInstructions.size(); ++j) {
//                    auto * importedInstr = importedInstructions.at(j);
//                    if (auto * funcExpr = dynamic_cast<FunctionExpression *>(importedInstr)) {
//                        funcExpr->SetName(storedModuleName + "." + funcExpr->GetName());
//                    } else if (dynamic_cast<ImportExpression *>(importedInstr)) {
//                        ResolveImports(basePath, tree, ast->GetInstructions());
//                    }
//
//                    // todo: support function calls of the module function in its own functions
//                }
//
//                instructions.insert(instructions.begin(), importedInstructions.begin()+1, importedInstructions.end());
//            }
//        }
    }

}