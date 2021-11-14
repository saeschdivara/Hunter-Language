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
                llvm::FunctionType::get(llvm::Type::getInt32Ty(m_Context), true),
                llvm::Function::ExternalLinkage,
                "main",
                module
        );

        auto *printfFuncType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(m_Context),
                std::vector<llvm::Type *>({llvm::Type::getInt8PtrTy(m_Context)}),
                false
        );

        llvm::Function *printfFunc = llvm::Function::Create(
                printfFuncType,
                llvm::Function::ExternalLinkage,
                "printf",
                module
        );

        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(m_Context, "EntryBlock", hunterFunction);
        llvm::IRBuilder<> *builder = new llvm::IRBuilder<>(entryBlock);

        for (const auto &instr : ast->GetInstructions()) {
            if (auto *funcExpr = dynamic_cast<FunctionExpression *>(instr)) {
                //
            } else if (auto *printExpr = dynamic_cast<PrintExpression *>(instr)) {

                if (auto *strExpr = dynamic_cast<StringExpression *>(printExpr->GetInput())) {
                    llvm::GlobalVariable *var = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
                    llvm::Value *ops[] = {var};
                    builder->CreateCall(printfFunc, llvm::ArrayRef(ops, sizeof(ops) / sizeof(llvm::Value *)));
                }
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
}