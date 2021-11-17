#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <string>
#include <unordered_map>

namespace llvm {
    class BasicBlock;
    class Module;
    class Value;
}

namespace Hunter::Compiler {

    class Expression;
    class FunctionExpression;
    class IfExpression;
    class ConstExpression;
    class PrintExpression;
    class AbstractSyntaxTree;

    class CodeGenerator {
    public:
        llvm::Module * GenerateCode(AbstractSyntaxTree * ast);

    protected:
        void InsertExpression(llvm::IRBuilder<> *builder, Expression * expr);
        void InsertFunctionExpression(llvm::IRBuilder<> *builder, FunctionExpression *funcExpr);
        void InsertIfExpression(llvm::IRBuilder<> *builder, IfExpression *ifExpr);
        void InsertPrintExpression(llvm::IRBuilder<> *builder, PrintExpression *constExpr);
        void InsertConstExpression(llvm::IRBuilder<> *builder, ConstExpression *constExpr);

        llvm::Value * GetValueFromExpression(llvm::IRBuilder<> *builder, Expression * expr);

    private:
        // make sure it lives as long as the module is used
        llvm::Module * m_Module;
        llvm::LLVMContext m_Context;

        std::unordered_map<std::string, llvm::Function *> m_Functions;
        std::unordered_map<std::string, llvm::Value *> m_Variables;
        std::unordered_map<std::string, Expression *> m_VariablesExpression;
    };
}

