#include "CodeGenerator.h"
#include "Parser.h"
#include "Expressions.h"

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

        m_Module = new llvm::Module("Hunt", m_Context);

        llvm::Function *hunterFunction = llvm::Function::Create(
                llvm::FunctionType::get(llvm::Type::getInt32Ty(m_Context), false),
                llvm::Function::ExternalLinkage,
                "main",
                m_Module
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
                m_Module
        );

        m_Functions["printf"] = printfFunc;

        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(m_Context, "EntryBlock", hunterFunction);
        llvm::IRBuilder<> *builder = new llvm::IRBuilder<>(entryBlock);

        for (const auto &instr : ast->GetInstructions()) {
            if (auto *funcExpr = dynamic_cast<FunctionExpression *>(instr)) {
                InsertFunctionExpression(builder, funcExpr);
            } else {
                InsertExpression(builder, instr);
            }
        }

        builder->CreateRet(llvm::ConstantInt::get(m_Context, llvm::APInt(32, 0)));

        std::error_code err;
        llvm::raw_ostream *ostream = new llvm::raw_fd_ostream("output.bc", err);
        llvm::WriteBitcodeToFile(*m_Module, *ostream);

        m_Module->print(llvm::errs(), nullptr);

        delete ostream;

        return m_Module;
    }

    void CodeGenerator::InsertExpression(llvm::IRBuilder<> *builder, Expression *expr) {
        if (auto *printExpr = dynamic_cast<PrintExpression *>(expr)) {
            InsertPrintExpression(builder, printExpr);
        } else if (auto *constExpr = dynamic_cast<ConstExpression *>(expr)) {
            InsertConstExpression(builder, constExpr);
        } else if (auto *ifExpr = dynamic_cast<IfExpression *>(expr)) {
            InsertIfExpression(builder, ifExpr);
        } else if (auto *forExpr = dynamic_cast<ForLoopExpression *>(expr)) {
            InsertForLoopExpression(builder, forExpr);
        }
    }

    llvm::IntegerType *GetVariableTypeForInt(llvm::IRBuilder<> *builder, IntType type) {
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

    void CodeGenerator::InsertFunctionExpression(llvm::IRBuilder<> *builder, FunctionExpression *funcExpr) {
        std::string functionName = funcExpr->GetName();
        llvm::Function *currentFunction;

        if (m_Functions.contains(functionName)) {
            currentFunction = m_Functions[functionName];
        } else {
            auto *simpleFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(m_Context), false);

            currentFunction = llvm::Function::Create(
                    simpleFuncType,
                    llvm::Function::InternalLinkage,
                    functionName,
                    m_Module
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
    }

    void CodeGenerator::InsertIfExpression(llvm::IRBuilder<> *builder, IfExpression *ifExpr) {
        BooleanExpression *condition = dynamic_cast<BooleanExpression *>(ifExpr->GetCondition());

        if (condition->GetOperator() == OperatorType::LogicalEquals) {
            llvm::Value *compareResult = builder->CreateICmpEQ(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
            );

            llvm::Function *func = builder->GetInsertBlock()->getParent();

            llvm::BasicBlock *thenBlock = llvm::BasicBlock::Create(m_Context, "then");
            llvm::BasicBlock *elseConditionBlock = llvm::BasicBlock::Create(m_Context, "else");
            llvm::BasicBlock *afterElseConditionBlock = llvm::BasicBlock::Create(m_Context, "endIf");

            builder->CreateCondBr(compareResult, thenBlock, elseConditionBlock);

            func->getBasicBlockList().push_back(thenBlock);
            builder->SetInsertPoint(thenBlock);
            for (const auto &expr : ifExpr->GetBody()) {
                InsertExpression(builder, expr);
            }

            builder->CreateBr(afterElseConditionBlock);

            func->getBasicBlockList().push_back(elseConditionBlock);
            builder->SetInsertPoint(elseConditionBlock);

            if (ifExpr->GetElse()) {
                for (const auto &expr : ifExpr->GetElse()->GetBody()) {
                    InsertExpression(builder, expr);
                }
            }

            builder->CreateBr(afterElseConditionBlock);

            func->getBasicBlockList().push_back(afterElseConditionBlock);
            builder->SetInsertPoint(afterElseConditionBlock);

        }
    }

    void CodeGenerator::InsertForLoopExpression(llvm::IRBuilder<> *builder, ForLoopExpression *forExpr) {
        // create loop counter
        if (auto *range = dynamic_cast<RangeExpression *>(forExpr->GetRange())) {
            llvm::IntegerType * counterType = GetVariableTypeForInt(builder, IntType::i64);

            std::string counterName = forExpr->GetCounter();
            // Start the PHI node with an entry for Start.
            InsertIntExpression(builder, counterName, new IntExpression(IntType::i64, range->GetStart()));
            llvm::ConstantInt * endValue = llvm::ConstantInt::get(counterType, range->GetEnd());

            llvm::Function *currentFunction = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(m_Context, "loop", currentFunction);

            builder->CreateBr(loopBlock);
            builder->SetInsertPoint(loopBlock);

            // body
            for (const auto &expr : forExpr->GetBody()) {
                InsertExpression(builder, expr);
            }

            // step
            llvm::Constant * step = llvm::ConstantInt::get(GetVariableTypeForInt(builder, IntType::i64), 1);

            llvm::Value *loadedCounter = builder->CreateLoad(counterType, m_Variables[counterName]);
            llvm::Value *nextCounter = builder->CreateAdd(loadedCounter, step, "next-counter");
            builder->CreateStore(nextCounter, m_Variables[counterName]);

            llvm::Value * conditionResult = builder->CreateICmpSLE(
                builder->CreateLoad(GetVariableTypeForInt(builder, IntType::i64), m_Variables[counterName]),
                endValue,
                "loop-condition"
            );
            // Create the "after loop" block and insert it.
            llvm::BasicBlock *afterLoopBlock = llvm::BasicBlock::Create(m_Context, "afterloop", currentFunction);

            // Insert the conditional branch into the end of LoopEndBB.
            builder->CreateCondBr(conditionResult, loopBlock, afterLoopBlock);

            // Any new code will be inserted in AfterBB.
            builder->SetInsertPoint(afterLoopBlock);
            m_Variables.erase(counterName);
            m_VariablesExpression.erase(counterName);
        } else {
            std::cerr << "Unknown expression type for range" << std::endl;
            exit(1);
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

                    Expression *variableExpr = m_VariablesExpression[variableName];

                    if (dynamic_cast<StringExpression *>(variableExpr)) {
                        ops.push_back(builder->CreateLoad(builder->getInt8PtrTy(), m_Variables[variableName]));
                        formatString += "%s";
                    } else if (auto *intValExpr = dynamic_cast<IntExpression *>(variableExpr)) {
                        IntType type = intValExpr->GetType();
                        auto *loadExpr = builder->CreateLoad(GetVariableTypeForInt(builder, type),
                                                             m_Variables[variableName]);
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

    llvm::Constant *GetIntValue(llvm::IRBuilder<> *builder, IntExpression *expr) {
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
        Expression *value = constExpr->GetValue();

        if (m_Variables.contains(variableName)) {
            std::cerr << "Variable " << variableName << " was already defined" << std::endl;
            exit(1);
        }

        if (auto *strExpr = dynamic_cast<StringExpression *>(value)) {
            llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
            auto *var = builder->CreateAlloca(builder->getInt8PtrTy(), nullptr, variableName);

            m_Variables[variableName] = var;
            m_VariablesExpression[variableName] = value;

            builder->CreateStore(strData, var);
        } else if (auto *intExpr = dynamic_cast<IntExpression *>(value)) {
            InsertIntExpression(builder, variableName, intExpr);
        }
    }

    void CodeGenerator::InsertIntExpression(llvm::IRBuilder<> *builder, const std::string &variableName,
                                            IntExpression *intExpr) {
        auto *var = builder->CreateAlloca(GetVariableTypeForInt(builder, intExpr->GetType()), nullptr, variableName);
        m_Variables[variableName] = var;
        m_VariablesExpression[variableName] = intExpr;

        builder->CreateStore(GetIntValue(builder, intExpr), var);
    }

    llvm::Value *CodeGenerator::GetValueFromExpression(llvm::IRBuilder<> *builder, Expression *expr) {

        if (auto *intValExpr = dynamic_cast<IntExpression *>(expr)) {
            return GetIntValue(builder, intValExpr);
        } else if (auto *identifierExpr = dynamic_cast<IdentifierExpression *>(expr)) {
            std::string variableName = identifierExpr->GetVariableName();
            if (!m_Variables.contains(variableName)) {
                std::cerr << "Could not find variable " << variableName << std::endl;
                exit(1);
            }

            Expression *variableExpr = m_VariablesExpression[variableName];

            if (auto *intValExpr = dynamic_cast<IntExpression *>(variableExpr)) {
                IntType type = intValExpr->GetType();
                return builder->CreateLoad(GetVariableTypeForInt(builder, type), m_Variables[variableName]);
            }
        }

        std::cerr << "Could not map expression to value" << std::endl;
        exit(1);
    }
}