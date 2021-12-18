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
    }

}