#include "Parser.h"

namespace Hunter::Compiler {

    class ImportResolver {
    public:
        void ResolveImports(const std::string & basePath, AbstractSyntaxTree * tree);
    };

}