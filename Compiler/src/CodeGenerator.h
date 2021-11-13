#pragma once

#include <string>

namespace llvm {
    class Module;
}

namespace Hunter::Compiler {

    class CodeGenerator {
    public:
        llvm::Module * GenerateCodeFromString(const std::string & code);
    };
}

