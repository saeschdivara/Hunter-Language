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

        m_Module->print(llvm::outs(), nullptr);

        delete ostream;

        return m_Module;
    }

    void CodeGenerator::InsertExpression(llvm::IRBuilder<> *builder, Expression *expr) {
        if (auto *printExpr = dynamic_cast<PrintExpression *>(expr)) {
            InsertPrintExpression(builder, printExpr);
        } else if (auto *constExpr = dynamic_cast<ConstExpression *>(expr)) {
            InsertConstExpression(builder, constExpr);
        } else if (auto *letExpr = dynamic_cast<LetExpression *>(expr)) {
            InsertLetExpression(builder, letExpr);
        } else if (auto *varMutExpr = dynamic_cast<VariableMutationExpression *>(expr)) {
            InsertVarMutationExpression(builder, varMutExpr);
        } else if (auto *ifExpr = dynamic_cast<IfExpression *>(expr)) {
            InsertIfExpression(builder, ifExpr);
        } else if (auto *forExpr = dynamic_cast<ForLoopExpression *>(expr)) {
            InsertForLoopExpression(builder, forExpr);
        } else if (auto *whileExpr = dynamic_cast<WhileExpression *>(expr)) {
            InsertWhileLoopExpression(builder, whileExpr);
        } else {
            std::cerr << "Unhandled expression found" << std::endl;
            exit(1);
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
        llvm::Value *compareResult = GetConditionFromExpression(builder, condition);

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

    void CodeGenerator::InsertForLoopExpression(llvm::IRBuilder<> *builder, ForLoopExpression *forExpr) {
        // create loop counter
        if (auto *range = dynamic_cast<RangeExpression *>(forExpr->GetRange())) {
            IntType intType = IntType::i64;
            llvm::IntegerType * counterType = GetVariableTypeForInt(builder, intType);

            std::string counterName = forExpr->GetCounter();
            InsertIntExpression(builder, counterName, new IntExpression(intType, range->GetStart()));
            llvm::ConstantInt * endValue = llvm::ConstantInt::get(counterType, range->GetEnd());

            llvm::Function *currentFunction = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(m_Context, "for-loop", currentFunction);

            builder->CreateBr(loopBlock);
            builder->SetInsertPoint(loopBlock);

            // body
            for (const auto &expr : forExpr->GetBody()) {
                InsertExpression(builder, expr);
            }

            // step
            llvm::Constant * step = llvm::ConstantInt::get(counterType, 1);

            llvm::Value *loadedCounter = builder->CreateLoad(counterType, m_Variables[counterName]);
            llvm::Value *nextCounter = builder->CreateAdd(loadedCounter, step, "next-counter");
            builder->CreateStore(nextCounter, m_Variables[counterName]);

            llvm::Value * conditionResult = builder->CreateICmpSLE(
                builder->CreateLoad(counterType, m_Variables[counterName]),
                endValue,
                "loop-condition"
            );
            // Create the "after loop" block and insert it.
            llvm::BasicBlock *afterLoopBlock = llvm::BasicBlock::Create(m_Context, "after-for-loop", currentFunction);

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

    void CodeGenerator::InsertWhileLoopExpression(llvm::IRBuilder<> *builder, WhileExpression *whileExpr) {
        if (auto * conditionExpr = dynamic_cast<BooleanExpression *>(whileExpr->GetCondition())) {
            llvm::Function *currentFunction = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(m_Context, "while-loop", currentFunction);
            llvm::BasicBlock *loopBodyBlock = llvm::BasicBlock::Create(m_Context, "while-body", currentFunction);
            llvm::BasicBlock *afterLoopBlock = llvm::BasicBlock::Create(m_Context, "after-while-loop", currentFunction);

            builder->CreateBr(loopBlock);
            builder->SetInsertPoint(loopBlock);

            llvm::Value *conditionResult = GetConditionFromExpression(builder, conditionExpr);
            builder->CreateCondBr(conditionResult, loopBodyBlock, afterLoopBlock);

            builder->SetInsertPoint(loopBodyBlock);

            // body
            for (const auto &expr : whileExpr->GetBody()) {
                InsertExpression(builder, expr);
            }

            builder->CreateBr(loopBlock);
            builder->SetInsertPoint(afterLoopBlock);

        } else {
            std::cerr << "Invalid expression type for condition" << std::endl;
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

    void CodeGenerator::InsertLetExpression(llvm::IRBuilder<> *builder, LetExpression *letExpr) {
        std::string variableName = letExpr->GetVariableName();
        Expression *value = letExpr->GetValue();

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

    void
    CodeGenerator::InsertVarMutationExpression(llvm::IRBuilder<> *builder, VariableMutationExpression *varMutExpr) {
        if (auto * operatorExpr = dynamic_cast<OperationExpression *>(varMutExpr->GetValue())) {
            if (operatorExpr->GetOperator() == OperatorType::MathPlus) {
                llvm::Value * variable = m_Variables[varMutExpr->GetVariableName()];

                llvm::Value *incrementedValue = builder->CreateAdd(
                    GetValueFromExpression(builder, operatorExpr->Left()),
                    GetValueFromExpression(builder, operatorExpr->Right()),
                    "next-" + varMutExpr->GetVariableName()
                );
                builder->CreateStore(incrementedValue, variable);
            } else {
                std::cerr << "Variable mutation with operator " << GetOperatorString(operatorExpr->GetOperator()) << "not yet implemented" << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Variable mutation without operation expr not implemented" << std::endl;
            exit(1);
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
            return GetVariableValue(builder, variableName);
        }

        std::cerr << "Could not map expression to value" << std::endl;
        exit(1);
    }

    llvm::Value *CodeGenerator::GetVariableValue(llvm::IRBuilder<> *builder, const std::string &variableName) {
        if (!m_Variables.contains(variableName)) {
            std::cerr << "Could not find variable " << variableName << std::endl;
            exit(1);
        }

        Expression *variableExpr = m_VariablesExpression[variableName];

        if (auto *intValExpr = dynamic_cast<IntExpression *>(variableExpr)) {
            IntType type = intValExpr->GetType();
            return builder->CreateLoad(GetVariableTypeForInt(builder, type), m_Variables[variableName]);
        } else {
            std::cerr << "Currently only integer expressions are supported for variable values" << std::endl;
            exit(1);
        }
    }

    llvm::Value *CodeGenerator::GetConditionFromExpression(llvm::IRBuilder<> *builder, BooleanExpression *condition) {

        switch (condition->GetOperator()) {
            case OperatorType::LogicalEquals:
                return builder->CreateICmpEQ(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
                );
            case OperatorType::LogicalLower:
                return builder->CreateICmpSLT(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
                );
            case OperatorType::LogicalLowerEquals:
                return builder->CreateICmpSLE(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
                );
            case OperatorType::LogicalGreater:
                return builder->CreateICmpSGT(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
                );
            case OperatorType::LogicalGreaterEquals:
                return builder->CreateICmpSGE(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
                );
            default:
                std::cerr << "Not supported operator used: " << GetOperatorString(condition->GetOperator()) << std::endl;
                exit(1);
        }
    }
}