#include "DebugGenerator.h"
#include "DebugData.h"
#include "Expressions.h"

#include <llvm/IR/Module.h>

namespace Hunter::Compiler::Debug {
    void DebugGenerator::CreateCompileUnit(Hunter::Parser::Debug::DebugData *data) {
        m_CompileUnit = m_DebugInfoBuilder->createCompileUnit(
            llvm::dwarf::DW_LANG_C,
            m_DebugInfoBuilder->createFile(data->GetFileName(), data->GetDirectory()),
            "Hunter Compiler",
            false,
            "",
            0
        );
    }

    void DebugGenerator::DefineFunction(llvm::Function *func, FunctionExpression * expr) {
        llvm::DIFile *Unit = m_DebugInfoBuilder->createFile(m_CompileUnit->getFilename(),
                                                      m_CompileUnit->getDirectory());

        llvm::DISubprogram *subProgram = m_DebugInfoBuilder->createFunction(
            Unit,
            expr->GetName(),
            llvm::StringRef(),
            Unit,
            expr->GetDebugData()->GetFileLine(),
            GetFunctionType(expr),
            expr->GetDebugData()->GetLineColumn(),
            llvm::DINode::FlagPrototyped,
            llvm::DISubprogram::SPFlagDefinition
        );
        func->setSubprogram(subProgram);

        PushLocation(subProgram);
    }

    void DebugGenerator::EmitLocation(llvm::IRBuilder<> *builder, Expression *expr) {
        llvm::DIScope *scope;
        if (m_LexicalBlocks.empty()) {
            scope = m_CompileUnit;
        }
        else {
            scope = m_LexicalBlocks.back();
        }

        auto * debugData = expr->GetDebugData();
        builder->SetCurrentDebugLocation(
            llvm::DILocation::get(
                scope->getContext(),
                debugData->GetFileLine(),
                debugData->GetLineColumn(),
                scope
            )
        );
    }

    void DebugGenerator::PushLocation(llvm::DIScope *scope) {
        m_LexicalBlocks.push_back(scope);
    }

    void DebugGenerator::PopLocation() {
        if (!m_LexicalBlocks.empty()) {
            m_LexicalBlocks.pop_back();
        }
    }

    void DebugGenerator::Generate() {
        m_DebugInfoBuilder->finalize();
    }

    llvm::DIType *DebugGenerator::GetDebugDatatype(DataType dataType) {
        switch (dataType) {
            case DataType::i8:
                return m_DebugInfoBuilder->createBasicType("int", 8, 0);
            case DataType::i16:
                return m_DebugInfoBuilder->createBasicType("int", 16, 0);
            case DataType::i32:
                return m_DebugInfoBuilder->createBasicType("int", 32, 0);
            case DataType::i64:
                return m_DebugInfoBuilder->createBasicType("int", 64, 0);
            case DataType::String:
                return m_DebugInfoBuilder->createStringType("string", 8);
            case DataType::Memory:
                return m_DebugInfoBuilder->createPointerType(
                    m_DebugInfoBuilder->createBasicType("void", 64, 0),
                    64
                );
            case DataType::Void:
                return m_DebugInfoBuilder->createBasicType("void", 64, 0);
        }
    }

    llvm::DISubroutineType *DebugGenerator::GetFunctionType(FunctionExpression *expr) {
        std::vector<llvm::Metadata *> routineTypes;

        // Add the result type.
        routineTypes.push_back(GetDebugDatatype(expr->GetReturnType()));

        for (const auto &parameter : expr->GetParameters()) {
            routineTypes.push_back(GetDebugDatatype(parameter->GetDataType()));
        }

        return m_DebugInfoBuilder->createSubroutineType(
            m_DebugInfoBuilder->getOrCreateTypeArray(routineTypes)
        );
    }
}