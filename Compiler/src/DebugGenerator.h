#pragma once

#include "Expressions.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>

namespace llvm {
    class Module;
    class Function;
    class DISubroutineType;
}

namespace Hunter::Parser::Debug {
    class DebugData;
}

namespace Hunter::Compiler::Debug {

    class DebugGenerator {
    public:
        DebugGenerator(llvm::Module * module) : m_DebugInfoBuilder(new llvm::DIBuilder(*module)) {}

        void CreateCompileUnit(Hunter::Parser::Debug::DebugData * data);
        void DefineFunction(llvm::Function * func, FunctionExpression * expr);
        void DefineVariable(llvm::IRBuilder<> *builder, llvm::AllocaInst * alloc, Expression * expr);
        void EmitLocation(llvm::IRBuilder<> *builder, Expression * expr);
        void PushLocation(llvm::DIScope * scope);
        void PopLocation();
        void Generate();

    private:
        llvm::DIType * GetDebugDatatype(DataType dataType);
        llvm::DISubroutineType * GetFunctionType(FunctionExpression * expr);

        llvm::DIBuilder * m_DebugInfoBuilder = nullptr;
        llvm::DICompileUnit * m_CompileUnit;
        std::vector<llvm::DIScope *> m_LexicalBlocks;
    };

}
