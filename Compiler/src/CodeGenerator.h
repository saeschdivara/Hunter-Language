#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <string>
#include <unordered_map>

namespace llvm {
    class BasicBlock;
    class Module;
}

namespace Hunter::Compiler {

    class Expression;
    class AbstractSyntaxTree;

    class CodeGenerator {
    public:
        llvm::Module * GenerateCode(AbstractSyntaxTree * ast);

    protected:
        void InsertExpression(llvm::IRBuilder<> *builder, Expression * expr);

    private:
        // make sure it lives as long as the module is used
        llvm::LLVMContext m_Context;

        std::unordered_map<std::string, llvm::Function *> m_Functions;
        std::unordered_map<std::string, llvm::Value *> m_Variables;
    };
}

