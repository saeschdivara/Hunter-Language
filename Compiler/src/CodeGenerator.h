#pragma once

#include <llvm/IR/LLVMContext.h>
#include <string>

namespace llvm {
    class Module;
}

namespace Hunter::Compiler {

    class AbstractSyntaxTree;

    class CodeGenerator {
    public:
        llvm::Module * GenerateCode(AbstractSyntaxTree * ast);

    private:
        // make sure it lives as long as the module is used
        llvm::LLVMContext m_Context;
    };
}

