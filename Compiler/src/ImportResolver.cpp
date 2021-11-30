#include "ImportResolver.h"
#include "Expressions.h"
#include "./utils/files.h"
#include "./utils/strings.h"

namespace Hunter::Compiler {

    void ImportResolver::ResolveImports(const std::string & basePath, AbstractSyntaxTree *tree) {
        auto & instructions = tree->GetInstructions();
        for (auto i = instructions.begin(); i != instructions.end(); ++i) {
            Expression * instruction = *i;
            if (auto * importExpr = dynamic_cast<ImportExpression *>(instruction)) {
                instructions.erase(i);

                std::string module = importExpr->GetModule();
                replaceAll(module, ".", "/");
                std::string moduleFilePath = basePath + "/" + module + ".hunt";

                std::string input = readFileIntoString(moduleFilePath);

                Parser parser;
                AbstractSyntaxTree * ast = parser.Parse(input);
                auto & importedInstructions = ast->GetInstructions();
                instructions.insert(instructions.begin(), importedInstructions.begin(), importedInstructions.end());

                std::cout << "Import ast:: " << std::endl;
                ast->Dump();
            }
        }
    }

}