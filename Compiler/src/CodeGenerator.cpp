#include "CodeGenerator.h"
#include "Parser.h"
#include <fstream>
#include <iostream>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/IRObjectFile.h>


namespace Hunter::Compiler {
    llvm::Module *CodeGenerator::GenerateCode(AbstractSyntaxTree *ast) {

        std::cout << std::endl << std::endl << "Generate code" << std::endl << "------------" << std::endl;

        auto * module = new llvm::Module("Hunt", m_Context);

        llvm::Function *hunterFunction = llvm::Function::Create(
                llvm::FunctionType::get(llvm::Type::getInt32Ty(m_Context), false),
                llvm::Function::ExternalLinkage,
                "main",
                module
        );

        auto *printfFuncType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(m_Context),
                std::vector<llvm::Type *>({llvm::Type::getInt8PtrTy(m_Context)}),
                true
        );

        llvm::Function *printfFunc = llvm::Function::Create(
                printfFuncType,
                llvm::Function::ExternalLinkage,
                "printf",
                module
        );

        m_Functions["printf"] = printfFunc;

        auto * simpleFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(m_Context), false);

        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(m_Context, "EntryBlock", hunterFunction);
        llvm::IRBuilder<> *builder = new llvm::IRBuilder<>(entryBlock);

        for (const auto &instr : ast->GetInstructions()) {
            if (auto *funcExpr = dynamic_cast<FunctionExpression *>(instr)) {
                std::string functionName = funcExpr->GetName();
                llvm::Function * currentFunction;

                if (m_Functions.contains(functionName)) {
                    currentFunction = m_Functions[functionName];
                } else {
                    currentFunction = llvm::Function::Create(
                            simpleFuncType,
                            llvm::Function::InternalLinkage,
                            functionName,
                            module
                    );

                    // Create a new basic block to start insertion into.
                    llvm::BasicBlock::Create(m_Context, "entry", currentFunction);

                    m_Functions[funcExpr->GetName()] = currentFunction;
                }
                llvm::IRBuilder<> funcBlockBuilder(&currentFunction->getEntryBlock(), currentFunction->getEntryBlock().begin());

                for (const auto &expr : funcExpr->GetBody()) {
                    InsertExpression(&funcBlockBuilder, expr);
                }

                funcBlockBuilder.CreateRetVoid();

                if (functionName == "hunt") {
                    builder->CreateCall(currentFunction);
                }

            } else {
                InsertExpression(builder, instr);
            }
        }

        builder->CreateRet(llvm::ConstantInt::get(m_Context, llvm::APInt(32, 0)));

        std::error_code err;
        llvm::raw_ostream *ostream = new llvm::raw_fd_ostream("output.bc", err);
        llvm::WriteBitcodeToFile(*module, *ostream);

        module->print(llvm::errs(), nullptr);

        delete ostream;

        return module;
    }

    void CodeGenerator::InsertExpression(llvm::IRBuilder<> * builder, Expression *expr) {
        if (auto *printExpr = dynamic_cast<PrintExpression *>(expr)) {
            InsertPrintExpression(builder, printExpr);
        }
        else if (auto *constExpr = dynamic_cast<ConstExpression *>(expr)) {
            InsertConstExpression(builder, constExpr);
        }
    }

    llvm::Type * GetVariableTypeForInt(llvm::IRBuilder<> *builder, IntType type) {
        switch (type) {
            case IntType::i8:
                return builder->getInt8Ty();
            case IntType::i16:
                return builder->getInt16Ty();
            case IntType::i32:
                return builder->getInt32Ty();
            case IntType::i64:
                return builder->getInt64Ty();
        }
    }

    void CodeGenerator::InsertPrintExpression(llvm::IRBuilder<> *builder, PrintExpression *printExpr) {

        if (auto *funcCallExpr = dynamic_cast<FunctionCallExpression *>(printExpr->GetInput())) {
            std::string formatString;
            std::vector<llvm::Value *> ops;

            for (const auto &parameter : funcCallExpr->GetParameters()) {

                if (auto *strExpr = dynamic_cast<StringExpression *>(parameter)) {
                    llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
                    ops.push_back(strData);
                    formatString += "%s";

                } else if (auto *identifierExpr = dynamic_cast<IdentifierExpression *>(parameter)) {
                    std::string variableName = identifierExpr->GetVariableName();
                    if (!m_Variables.contains(variableName)) {
                        std::cerr << "Could not find variable " << variableName << std::endl;
                        exit(1);
                    }

                    Expression * variableExpr = m_VariablesExpression[variableName];

                    if (dynamic_cast<StringExpression *>(variableExpr)) {
                        ops.push_back(builder->CreateLoad(builder->getInt8PtrTy(), m_Variables[variableName]));
                        formatString += "%s";
                    }
                    else if (auto * intValExpr = dynamic_cast<IntExpression *>(variableExpr)) {
                        IntType type = intValExpr->GetType();
                        auto * loadExpr = builder->CreateLoad(GetVariableTypeForInt(builder, type), m_Variables[variableName]);
                        ops.push_back(loadExpr);

                        if (type == IntType::i64) {
                            formatString += "%lld";
                        } else {
                            formatString += "%d";
                        }
                    }

                }
            }

            llvm::GlobalVariable *var = builder->CreateGlobalString(llvm::StringRef(formatString));
            ops.insert(ops.cbegin(), var);

            builder->CreateCall(m_Functions["printf"], llvm::ArrayRef(ops));
        }
    }

    llvm::Constant * GetIntValue(llvm::IRBuilder<> *builder, IntExpression * expr) {
        switch (expr->GetType()) {
            case IntType::i8:
                return builder->getInt8(expr->GetValue());
            case IntType::i16:
                return builder->getInt16(expr->GetValue());
            case IntType::i32:
                return builder->getInt32(expr->GetValue());
            case IntType::i64:
                return builder->getInt64(expr->GetValue());
        }
    }

    void CodeGenerator::InsertConstExpression(llvm::IRBuilder<> *builder, ConstExpression *constExpr) {
        std::string variableName = constExpr->GetVariableName();
        Expression * value = constExpr->GetValue();
        
        if (auto *strExpr = dynamic_cast<StringExpression *>(value)) {
            llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
            auto * var = builder->CreateAlloca(builder->getInt8PtrTy(), nullptr, variableName);

            m_Variables[variableName] = var;
            m_VariablesExpression[variableName] = value;

            builder->CreateStore(strData, var);
        }
        else if (auto *intExpr = dynamic_cast<IntExpression *>(value)) {
            auto * var = builder->CreateAlloca(GetVariableTypeForInt(builder, intExpr->GetType()), nullptr, variableName);
            m_Variables[variableName] = var;
            m_VariablesExpression[variableName] = value;

            builder->CreateStore(GetIntValue(builder, intExpr), var);
        }
    }
}