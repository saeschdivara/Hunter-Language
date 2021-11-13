#include "CodeGenerator.h"
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
    llvm::Module *CodeGenerator::GenerateCodeFromString(const std::string &code) {

        llvm::LLVMContext context;

        auto * module = new llvm::Module("Hunt", context);

        llvm::Function *hunterFunction = llvm::Function::Create(
                llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false),
                llvm::Function::ExternalLinkage,
                "main",
                module
        );

        llvm::BasicBlock * entryBlock = llvm::BasicBlock::Create(context, "EntryBlock", hunterFunction);
        llvm::IRBuilder<> *builder = new llvm::IRBuilder<>(entryBlock);

        builder->CreateRet(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));

        std::error_code err;
        llvm::raw_ostream *ostream = new llvm::raw_fd_ostream("output.bc", err );
        llvm::WriteBitcodeToFile(*module, *ostream);

        module->print(llvm::errs(), nullptr);

        delete ostream;

        return module;
    }
}