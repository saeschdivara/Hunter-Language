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

namespace Hunter::Compiler {
    class CodeGenerator;
}

namespace Hunter::Compiler::Debug {

    class DebugGenerator {
    public:
        DebugGenerator(llvm::Module * module, Hunter::Compiler::CodeGenerator * generator)
            : m_DebugInfoBuilder(new llvm::DIBuilder(*module)), m_CodeGenerator(generator) {}

        void CreateCompileUnit(Hunter::Parser::Debug::DebugData * data);
        void DefineFunction(llvm::Function * func, FunctionExpression * expr);
        void DefineVariable(llvm::IRBuilder<> *builder, llvm::AllocaInst * alloc, Expression * expr);
        void EmitLocation(llvm::IRBuilder<> *builder, Expression * expr);
        void PushLocation(llvm::DIScope * scope);
        void PopLocation();
        void Generate();

    private:
        llvm::DIType * GetDebugDatatype(DataTypeId dataType);
        llvm::DISubroutineType * GetFunctionType(FunctionExpression * expr);

        llvm::DIBuilder * m_DebugInfoBuilder = nullptr;
        llvm::DICompileUnit * m_CompileUnit;
        std::vector<llvm::DIScope *> m_LexicalBlocks;

        Hunter::Compiler::CodeGenerator * m_CodeGenerator;
    };

}
