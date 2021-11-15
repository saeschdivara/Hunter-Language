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

            if (auto *funcCallExpr = dynamic_cast<FunctionCallExpression *>(printExpr->GetInput())) {
                unsigned long paramNumber = funcCallExpr->GetParameters().size();
                std::string formatString;

                for (int i = 0; i < paramNumber; ++i) {
                    formatString += "%s";
                }

                llvm::GlobalVariable *var = builder->CreateGlobalString(llvm::StringRef(formatString));

                std::vector<llvm::Value *> ops;
                ops.push_back(var);

                for (const auto &parameter : funcCallExpr->GetParameters()) {

                    if (auto *strExpr = dynamic_cast<StringExpression *>(parameter)) {
                        llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
                        ops.push_back(strData);
                    } else if (auto *identifierExpr = dynamic_cast<IdentifierExpression *>(parameter)) {
                        std::string variableName = identifierExpr->GetVariableName();
                        if (!m_Variables.contains(variableName)) {
                            std::cerr << "Could not find variable " << variableName << std::endl;
                            exit(1);
                        }

                        ops.push_back(builder->CreateLoad(builder->getInt8PtrTy(), m_Variables[variableName]));
                    }
                }
                builder->CreateCall(m_Functions["printf"], llvm::ArrayRef(ops));
            }
        }
        else if (auto *constExpr = dynamic_cast<ConstExpression *>(expr)) {

            if (auto *strExpr = dynamic_cast<StringExpression *>(constExpr->GetValue())) {
                llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
                std::string variableName = constExpr->GetVariableName();
                auto * var = builder->CreateAlloca(builder->getInt8PtrTy(), nullptr, variableName);

                m_Variables[variableName] = var;

                builder->CreateStore(strData, var);
            }
        }
    }
}