#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <string>
#include <unordered_map>

namespace llvm {
    class BasicBlock;
    class Module;
    class Value;
    class DIBuilder;
}

namespace Hunter::Compiler {

    class Expression;
    class BooleanExpression;
    class FunctionExpression;
    class IfExpression;
    class ForLoopExpression;
    class WhileExpression;
    class ConstExpression;
    class LetExpression;
    class VariableMutationExpression;
    class PrintExpression;
    class FunctionCallExpression;
    class FunctionReturnExpression;
    class IntExpression;
    class AbstractSyntaxTree;

    class CodeGenerator {
    public:
        llvm::Module * GenerateCode(AbstractSyntaxTree * ast);

        void SetDebugOutputFile(const std::string & outputFileName) { m_DebugOutputFileName = outputFileName; }

    protected:
        void InsertExpression(llvm::IRBuilder<> *builder, Expression * expr);
        void InsertFunctionExpression(llvm::IRBuilder<> *builder, FunctionExpression *funcExpr);
        void InsertIfExpression(llvm::IRBuilder<> *builder, IfExpression *ifExpr);
        void InsertForLoopExpression(llvm::IRBuilder<> *builder, ForLoopExpression *forExpr);
        void InsertWhileLoopExpression(llvm::IRBuilder<> *builder, WhileExpression *whileExpr);
        void InsertPrintExpression(llvm::IRBuilder<> *builder, PrintExpression *constExpr);
        llvm::Value * InsertFunctionCallExpression(llvm::IRBuilder<> *builder, FunctionCallExpression *funcCallExpr);
        void InsertConstExpression(llvm::IRBuilder<> *builder, ConstExpression *constExpr);
        void InsertLetExpression(llvm::IRBuilder<> *builder, LetExpression *letExpr);
        void InsertVarMutationExpression(llvm::IRBuilder<> *builder, VariableMutationExpression *varMutExpr);
        void InsertFuncReturnExpression(llvm::IRBuilder<> *builder, FunctionReturnExpression * retExpr);
        void InsertIntExpression(llvm::IRBuilder<> *builder, const std::string & variableName, IntExpression *intExpr);

        llvm::Value * GetValueFromExpression(llvm::IRBuilder<> *builder, Expression * expr);
        llvm::Value * GetVariableValue(llvm::IRBuilder<> *builder, const std::string & variableName);
        llvm::Value * GetConditionFromExpression(llvm::IRBuilder<> *builder, BooleanExpression * condition);
        llvm::Value * GetEqualsCondition(llvm::IRBuilder<> *builder, BooleanExpression * condition);

        bool IsString(Expression * expr);
        bool IsInt(Expression * expr);

    private:
        std::string m_DebugOutputFileName;

        // make sure it lives as long as the module is used
        llvm::Module * m_Module;
        llvm::LLVMContext m_Context;

        llvm::DIBuilder * m_DebugInfoBuilder = nullptr;

        std::unordered_map<std::string, llvm::Function *> m_Functions;
        std::unordered_map<std::string, FunctionExpression *> m_FunctionsDefinitions;
        std::unordered_map<std::string, llvm::Value *> m_Variables;
        std::unordered_map<std::string, Expression *> m_VariablesExpression;
    };
}

