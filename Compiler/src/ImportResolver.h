#include "Parser.h"

namespace Hunter::Compiler {

    class ImportResolver {
    public:
        void ResolveImports(const std::string & basePath, AbstractSyntaxTree * tree, std::vector<Expression *> & instructions);

        void AddModule(const std::string & module) {
            m_Modules.push_back(module);
        }

        std::vector<std::string> & GetModules() {
            return m_Modules;
        }

        bool HasModule(const std::string & module) {
            for (const auto &item : m_Modules) {
                if (item == module) {
                    return true;
                }
            }

            return false;
        }

    private:
        std::vector<std::string> m_Modules;
    };

}