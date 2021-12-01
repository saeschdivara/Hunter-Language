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

    llvm::Type *GetTypeFromDataType(llvm::IRBuilder<> *builder, DataType dataType) {
        switch (dataType) {
            case DataType::Unknown: {
                std::cerr << "Cannot convert from unknown data type" << std::endl;
                exit(1);
            }
            case DataType::String:
                return builder->getInt8PtrTy();
            case DataType::i8:
                return builder->getInt8Ty();
            case DataType::i16:
                return builder->getInt16Ty();
            case DataType::i32:
                return builder->getInt32Ty();
            case DataType::i64:
                return builder->getInt64Ty();
        }
    }

    llvm::Module *CodeGenerator::GenerateCode(AbstractSyntaxTree *ast) {

        std::cout << std::endl << std::endl << "Generate code" << std::endl << "------------" << std::endl;

        m_Module = new llvm::Module("Hunt", m_Context);

        llvm::Function *hunterFunction = llvm::Function::Create(
                llvm::FunctionType::get(llvm::Type::getInt32Ty(m_Context), false),
                llvm::Function::ExternalLinkage,
                "main",
                m_Module
        );

        ///////////////////////////  STANDARD FUNCTIONS ///////////////////////////
        /////// int strcmp(const char *__s1, const char *__s2) {}
        auto *strcmpFuncType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(m_Context),
                std::vector<llvm::Type *>({
                    llvm::Type::getInt8PtrTy(m_Context),
                    llvm::Type::getInt8PtrTy(m_Context)
                }),
                false
        );

        llvm::Function *strcmpFunc = llvm::Function::Create(
                strcmpFuncType,
                llvm::Function::ExternalLinkage,
                "strcmp",
                m_Module
        );

        m_Functions["strcmp"] = strcmpFunc;

        /////// printf
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

        ///////////////////////////  STANDARD FUNCTIONS ///////////////////////////

        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(m_Context, "EntryBlock", hunterFunction);
        auto *builder = new llvm::IRBuilder<>(entryBlock);

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
        delete ostream;

        llvm::raw_fd_ostream *moduleOutputStream = &(llvm::outs());

        if (!m_DebugOutputFileName.empty()) {
            moduleOutputStream = new llvm::raw_fd_ostream(m_DebugOutputFileName, err);
        }

        m_Module->print(*moduleOutputStream, nullptr);

        if (!m_DebugOutputFileName.empty()) {
            delete moduleOutputStream;
        }

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
        } else if (auto *funcCallExpr = dynamic_cast<FunctionCallExpression *>(expr)) {
            InsertFunctionCallExpression(builder, funcCallExpr);
        } else if (auto *moduleExpr = dynamic_cast<ModuleExpression *>(expr)) {
            // maybe this will not be here anymore
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
            std::vector<llvm::Type *> parameterDescriptions;

            for (const auto &parameter : funcExpr->GetParameters()) {
                parameterDescriptions.push_back(GetTypeFromDataType(builder, parameter->GetDataType()));
            }

            auto *simpleFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(m_Context), parameterDescriptions, false);

            currentFunction = llvm::Function::Create(
                    simpleFuncType,
                    llvm::Function::InternalLinkage,
                    functionName,
                    m_Module
            );

            int argumentCounter = 0;
            for (auto &arg : currentFunction->args()) {
                arg.setName(funcExpr->GetParameters().at(argumentCounter)->GetName());
                argumentCounter++;
            }

            // Create a new basic block to start insertion into.
            llvm::BasicBlock::Create(m_Context, "entry", currentFunction);

            m_Functions[funcExpr->GetName()] = currentFunction;
        }

        llvm::IRBuilder<> funcBlockBuilder(&currentFunction->getEntryBlock(), currentFunction->getEntryBlock().begin());
        std::unordered_map<std::string, llvm::Value *> outerVariables;

        int argumentCounter = 0;
        for (const auto &arg : currentFunction->args()) {
            auto *parameter = funcExpr->GetParameters().at(argumentCounter);
            auto parameterName = parameter->GetName();

            if (m_Variables.contains(parameterName)) {
                outerVariables[parameterName] = m_Variables[parameterName];
            }

            llvm::Value *variable = funcBlockBuilder.CreateAlloca(
                GetTypeFromDataType(&funcBlockBuilder, parameter->GetDataType()),
                nullptr,
                parameterName
            );

            funcBlockBuilder.CreateStore((llvm::Value *)&arg, variable);

            m_Variables[parameterName] = variable;
            m_VariablesExpression[parameterName] = parameter;

            argumentCounter++;
        }

        for (const auto &expr : funcExpr->GetBody()) {
            InsertExpression(&funcBlockBuilder, expr);
        }

        // todo: reassign variables

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
            llvm::IntegerType *counterType = GetVariableTypeForInt(builder, intType);

            std::string counterName = forExpr->GetCounter();
            InsertIntExpression(builder, counterName, new IntExpression(intType, range->GetStart()));
            llvm::ConstantInt *endValue = llvm::ConstantInt::get(counterType, range->GetEnd());

            llvm::Function *currentFunction = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(m_Context, "for-loop", currentFunction);

            builder->CreateBr(loopBlock);
            builder->SetInsertPoint(loopBlock);

            // body
            for (const auto &expr : forExpr->GetBody()) {
                InsertExpression(builder, expr);
            }

            // step
            llvm::Constant *step = llvm::ConstantInt::get(counterType, 1);

            llvm::Value *loadedCounter = builder->CreateLoad(counterType, m_Variables[counterName]);
            llvm::Value *nextCounter = builder->CreateAdd(loadedCounter, step, "next-counter");
            builder->CreateStore(nextCounter, m_Variables[counterName]);

            llvm::Value *conditionResult = builder->CreateICmpSLE(
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
        if (auto *conditionExpr = dynamic_cast<BooleanExpression *>(whileExpr->GetCondition())) {
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
                    } else if (auto *parameterExpr = dynamic_cast<ParameterExpression *>(variableExpr)) {
                        auto parameterType = parameterExpr->GetDataType();
                        if (parameterType == DataType::String) {
                            ops.push_back(builder->CreateLoad(builder->getInt8PtrTy(), m_Variables[variableName]));
                            formatString += "%s";
                        } else if (
                                parameterType == DataType::i8 ||
                                parameterType == DataType::i16 ||
                                parameterType == DataType::i32 ||
                                parameterType == DataType::i64
                        ) {
                            auto type = static_cast<IntType>(parameterType);
                            auto *loadExpr = builder->CreateLoad(GetTypeFromDataType(builder, parameterType),
                                                                 m_Variables[variableName]);
                            ops.push_back(loadExpr);

                            if (type == IntType::i64) {
                                formatString += "%lld";
                            } else {
                                formatString += "%d";
                            }
                        }
                    } else if (!variableExpr) {
                        std::cerr << "Invalid expression found for variable " << variableName << std::endl;
                        exit(1);
                    } else {
                        std::cerr << "Unknown expression type of variable " << variableName << std::endl;
                        exit(1);
                    }

                } else {
                    std::cerr << "Unsupported expression for call parameter" << std::endl;
                    exit(1);
                }
            }

            llvm::GlobalVariable *var = builder->CreateGlobalString(llvm::StringRef(formatString));
            ops.insert(ops.cbegin(), var);

            builder->CreateCall(m_Functions["printf"], llvm::ArrayRef(ops));
        }
    }

    void CodeGenerator::InsertFunctionCallExpression(llvm::IRBuilder<> *builder, FunctionCallExpression *funcCallExpr) {
        std::vector<llvm::Value *> ops;

        for (const auto &parameter : funcCallExpr->GetParameters()) {

            if (auto *strExpr = dynamic_cast<StringExpression *>(parameter)) {
                llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
                ops.push_back(strData);

            } else if (auto *identifierExpr = dynamic_cast<IdentifierExpression *>(parameter)) {
                ops.push_back(GetVariableValue(builder, identifierExpr->GetVariableName()));
            } else if (auto *intExpr = dynamic_cast<IntExpression *>(parameter)) {
                auto * value = GetValueFromExpression(builder, intExpr);
                ops.push_back(value);
            } else {
                std::cerr << "Unsupported expression as parameter used" << std::endl;
                exit(1);
            }
        }

        auto functionName = funcCallExpr->GetFunctionName();

        if (!m_Functions.contains(functionName)) {
            std::cerr << "Function not defined: " << functionName << std::endl;
            exit(1);
        }

        builder->CreateCall(m_Functions[functionName], llvm::ArrayRef(ops));
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
        if (auto *operatorExpr = dynamic_cast<OperationExpression *>(varMutExpr->GetValue())) {
            if (operatorExpr->GetOperator() == OperatorType::MathPlus) {
                llvm::Value *variable = m_Variables[varMutExpr->GetVariableName()];

                llvm::Value *incrementedValue = builder->CreateAdd(
                        GetValueFromExpression(builder, operatorExpr->Left()),
                        GetValueFromExpression(builder, operatorExpr->Right()),
                        "next-" + varMutExpr->GetVariableName()
                );
                builder->CreateStore(incrementedValue, variable);
            } else {
                std::cerr << "Variable mutation with operator " << GetOperatorString(operatorExpr->GetOperator())
                          << "not yet implemented" << std::endl;
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
        } else if (auto *strExpr = dynamic_cast<StringExpression *>(expr)) {
            llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
            return strData;
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

        if (dynamic_cast<StringExpression *>(variableExpr)) {
            return builder->CreateLoad(builder->getInt8PtrTy(), m_Variables[variableName]);
        } else if (auto *intValExpr = dynamic_cast<IntExpression *>(variableExpr)) {
            IntType type = intValExpr->GetType();
            return builder->CreateLoad(GetVariableTypeForInt(builder, type), m_Variables[variableName]);
        } else {
            std::cerr << "Unsupported expressions for variable values found" << std::endl;
            exit(1);
        }
    }

    llvm::Value *CodeGenerator::GetConditionFromExpression(llvm::IRBuilder<> *builder, BooleanExpression *condition) {

        switch (condition->GetOperator()) {
            case OperatorType::LogicalEquals:
                return GetEqualsCondition(builder, condition);
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
                std::cerr << "Not supported operator used: " << GetOperatorString(condition->GetOperator())
                          << std::endl;
                exit(1);
        }
    }

    bool CodeGenerator::IsString(Expression * expr) {
        if (dynamic_cast<StringExpression *>(expr)) {
            return true;
        }
        if (auto * identifierExpr = dynamic_cast<IdentifierExpression *>(expr)) {
            return dynamic_cast<StringExpression *>(m_VariablesExpression[identifierExpr->GetVariableName()]);
        }
        else {
            return false;
        }
    }

    bool CodeGenerator::IsInt(Expression *expr) {
        if (dynamic_cast<IntExpression *>(expr)) {
            return true;
        }
        if (auto * identifierExpr = dynamic_cast<IdentifierExpression *>(expr)) {
            return dynamic_cast<IntExpression *>(m_VariablesExpression[identifierExpr->GetVariableName()]);
        }
        else {
            return false;
        }
    }

    llvm::Value *CodeGenerator::GetEqualsCondition(llvm::IRBuilder<> *builder, BooleanExpression *condition) {

        if (IsInt(condition->Left()) && IsInt(condition->Right())) {
            return builder->CreateICmpEQ(
                    GetValueFromExpression(builder, condition->Left()),
                    GetValueFromExpression(builder, condition->Right())
            );
        }
        else if (IsString(condition->Left()) && IsString(condition->Right())) {
            std::vector<llvm::Value *> params;
            params.push_back(GetValueFromExpression(builder, condition->Left()));
            params.push_back(GetValueFromExpression(builder, condition->Right()));

            llvm::Value * compareResult = builder->CreateCall(m_Functions["strcmp"], llvm::ArrayRef(params));

            return builder->CreateICmpEQ(
                    compareResult,
                    builder->getInt32(0)
            );
        }
        else {
            std::cerr << "Unsupported equals operation between these two types" << std::endl;
            exit(1);
        }
    }
}