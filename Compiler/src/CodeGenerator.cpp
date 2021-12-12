#include "CodeGenerator.h"
#include "Parser.h"
#include "Expressions.h"
#include "utils/logger.h"

#include <iostream>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Object/IRObjectFile.h>


namespace Hunter::Compiler {

    llvm::Type *GetTypeFromDataType(llvm::IRBuilder<> *builder, DataType dataType) {
        switch (dataType) {
            case DataType::Void:
                return builder->getVoidTy();
            case DataType::Memory:
                return llvm::PointerType::get(builder->getVoidTy(), 0);
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

    std::string GetFormatPlaceholderFromDataType(DataType dataType) {
        switch (dataType) {
            case DataType::Void: {
                COMPILER_ERROR("Unknown data type has no format string");
                exit(1);
            }
            case DataType::Memory: {
                return "%s";
            }
            case DataType::String:
                return "%s";
            case DataType::i8:
            case DataType::i16:
            case DataType::i32:
                return "%d";
            case DataType::i64:
                return "%lld";
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

        llvm::BasicBlock *entryBlock = llvm::BasicBlock::Create(m_Context, "EntryBlock", hunterFunction);
        auto *builder = new llvm::IRBuilder<>(entryBlock);

        for (const auto &instr : ast->GetInstructions()) {
            if (!m_DebugGenerator) {
                m_DebugGenerator = new Debug::DebugGenerator(m_Module, this);
                auto * debugData = instr->GetDebugData();
                m_DebugGenerator->CreateCompileUnit(debugData);
            }

            if (auto *funcExpr = dynamic_cast<FunctionExpression *>(instr)) {
                InsertFunctionExpression(builder, funcExpr);
            } else {
                InsertExpression(builder, instr);
            }
        }

        builder->CreateRet(llvm::ConstantInt::get(m_Context, llvm::APInt(32, 0)));

        if (m_DebugGenerator) {
            m_DebugGenerator->Generate();
        }

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
        } else if (auto *varDeclExpr = dynamic_cast<VariableDeclarationExpression *>(expr)) {
            InsertVarDeclarationExpression(builder, varDeclExpr);
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
        } else if (auto *retExpr = dynamic_cast<FunctionReturnExpression *>(expr)) {
            InsertFuncReturnExpression(builder, retExpr);
        } else if (auto *structExpr = dynamic_cast<StructExpression *>(expr)) {
            InsertStructDeclareExpression(builder, structExpr);
        } else if (auto *externExpr = dynamic_cast<ExternExpression *>(expr)) {
            if (auto * funcExpr = dynamic_cast<FunctionExpression *>(externExpr->GetData())) {
                InsertFunctionExpression(builder, funcExpr);
            } else {
                COMPILER_ERROR("Unknown data type used for external declaration: {0}", externExpr->GetData()->GetClassName());
                exit(1);
            }
        }  else if (auto *moduleExpr = dynamic_cast<ModuleExpression *>(expr)) {
            // maybe this will not be here anymore
        } else {
            COMPILER_ERROR("Unhandled expression found: {0}", expr->GetClassName());
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

            DataType returnType = funcExpr->GetReturnType();
            auto *simpleFuncType = llvm::FunctionType::get(GetTypeFromDataType(builder, returnType), parameterDescriptions, false);

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

            m_Functions[functionName] = currentFunction;
            m_FunctionsDefinitions[functionName] = funcExpr;

            m_DebugGenerator->DefineFunction(currentFunction, funcExpr);

            if (funcExpr->IsExternal()) {
                return;
            }

            // Create a new basic block to start insertion into.
            llvm::BasicBlock::Create(m_Context, "entry", currentFunction);
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

        for (const auto &parameter : funcExpr->GetParameters()) {
            auto parameterName = parameter->GetName();
            if (outerVariables.contains(parameterName)) {
                m_Variables[parameterName] = outerVariables[parameterName];
            } else {
                m_Variables.erase(parameterName);
            }
        }

//        if (!dynamic_cast<FunctionReturnExpression *>(*funcExpr->GetBody().end())) {
            funcBlockBuilder.CreateRetVoid();
//        }

        if (functionName == "hunt") {
            builder->CreateCall(currentFunction);
        }

        m_DebugGenerator->PopLocation();
    }

    void CodeGenerator::InsertStructDeclareExpression(llvm::IRBuilder<> *builder, StructExpression *structExpr) {
        std::vector<llvm::Type *> structTypes;

        for (const auto &propertyExpr : structExpr->GetBody()) {
            if (auto * property = dynamic_cast<PropertyDeclarationExpression *>(propertyExpr)) {
                structTypes.push_back(GetTypeFromDataType(builder, GetVariableDeclarationType(property)));
            } else {
                COMPILER_ERROR("Struct body element is not a property: {0}", propertyExpr->GetClassName());
                exit(1);
            }
        }

        std::string structName = structExpr->GetStructName();
        auto * structType = llvm::StructType::create(m_Context, structTypes, structName);

        m_Structs[structName] = structType;
        m_StructsDefinitions[structName] = structExpr;
    }

    void CodeGenerator::InsertIfExpression(llvm::IRBuilder<> *builder, IfExpression *ifExpr) {
        auto *condition = dynamic_cast<BooleanExpression *>(ifExpr->GetCondition());
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

            m_DebugGenerator->EmitLocation(builder, printExpr);

            for (const auto &parameter : funcCallExpr->GetParameters()) {

                if (auto *strExpr = dynamic_cast<StringExpression *>(parameter)) {
                    llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
                    ops.push_back(strData);
                    formatString += "%s";

                } else if (auto *identifierExpr = dynamic_cast<IdentifierExpression *>(parameter)) {
                    std::string variableName = identifierExpr->GetVariableName();
                    std::string propertyName;

                    uint64_t lastDotPosition = variableName.find_last_of('.');

                    if (lastDotPosition != std::string::npos) {
                        propertyName = variableName.substr(lastDotPosition+1);
                        std::string objectName = variableName.substr(0, lastDotPosition);
                        variableName = objectName;
                    }

                    if (!m_Variables.contains(variableName)) {
                        COMPILER_ERROR("Could not find variable {0}", variableName);
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
                    } else if (auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(variableExpr)) {
                        auto * funcDef = m_FunctionsDefinitions[funcCallExpr->GetFunctionName()];
                        auto * funcReturnType = GetTypeFromDataType(builder, funcDef->GetReturnType());
                        auto * funcReturnValue = builder->CreateLoad(funcReturnType, m_Variables[variableName]);
                        ops.push_back(funcReturnValue);
                        formatString += GetFormatPlaceholderFromDataType(funcDef->GetReturnType());
                    } else if (auto * structConstrExpr = dynamic_cast<StructConstructionExpression *>(variableExpr)) {
                        std::string structName = structConstrExpr->GetStructName();

                        llvm::StructType * structType = m_Structs[structName];
                        StructExpression * structExpr = m_StructsDefinitions[structName];
                        llvm::Type * structPointerType = llvm::PointerType::get(structType, 0);
                        uint64_t propertyIndex = structExpr->GetPropertyIndex(propertyName);

                        auto * loadedValue = builder->CreateLoad(structPointerType, m_Variables[variableName]);

                        auto * attributePointer = builder->CreateStructGEP(
                                structType,
                                builder->CreateBitCast(loadedValue, structPointerType),
                                propertyIndex
                        );

                        auto * structPropertyValue = builder->CreateLoad(structPointerType, attributePointer);
                        ops.push_back(structPropertyValue);

                        auto * propertyExpr = dynamic_cast<PropertyDeclarationExpression *>(structExpr->GetBody().at(propertyIndex));
                        formatString += GetFormatPlaceholderFromDataType(GetVariableDeclarationType(propertyExpr));

                    } else if (!variableExpr) {
                        COMPILER_ERROR("Invalid expression found for variable {0}", variableName);
                        exit(1);
                    } else {
                        COMPILER_ERROR("Unknown expression type of variable {0}: {1}", variableName, variableExpr->GetClassName());
                        exit(1);
                    }

                } else {
                    std::cerr << "Unsupported expression for call parameter" << std::endl;
                    exit(1);
                }
            }

            llvm::GlobalVariable *var = builder->CreateGlobalString(llvm::StringRef(formatString));
            ops.insert(ops.cbegin(), var);

            builder->CreateCall(GetCLibraryFunction("printf"), llvm::ArrayRef(ops));
        }
    }

    llvm::Value * CodeGenerator::InsertFunctionCallExpression(llvm::IRBuilder<> *builder, FunctionCallExpression *funcCallExpr) {
        m_DebugGenerator->EmitLocation(builder, funcCallExpr);
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
            COMPILER_ERROR("Function not defined: {0}", functionName);
            exit(1);
        }

        return builder->CreateCall(m_Functions[functionName], llvm::ArrayRef(ops));
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

    void CodeGenerator::InsertVarDeclarationExpression(llvm::IRBuilder<> *builder, VariableDeclarationExpression *constExpr) {
        m_DebugGenerator->EmitLocation(builder, constExpr);

        std::string variableName = constExpr->GetVariableName();
        Expression *value = constExpr->GetValue();

        if (m_Variables.contains(variableName)) {
            COMPILER_ERROR("Variable {0} was already defined", variableName);
            exit(1);
        }

        if (auto *strExpr = dynamic_cast<StringExpression *>(value)) {
            llvm::GlobalVariable *strData = builder->CreateGlobalString(llvm::StringRef(strExpr->GetString()));
            auto *var = builder->CreateAlloca(builder->getInt8PtrTy(), nullptr, variableName);
            m_DebugGenerator->DefineVariable(builder, var, constExpr);

            m_Variables[variableName] = var;
            m_VariablesExpression[variableName] = value;

            builder->CreateStore(strData, var);
        } else if (auto *intExpr = dynamic_cast<IntExpression *>(value)) {
           auto * var = InsertIntExpression(builder, variableName, intExpr);
           m_DebugGenerator->DefineVariable(builder, var, constExpr);
        } else if (auto *funcCallExpr = dynamic_cast<FunctionCallExpression *>(value)) {
            auto * func = m_FunctionsDefinitions[funcCallExpr->GetFunctionName()];

            if (!func) {
                COMPILER_ERROR("Could not find function definition for {0}", funcCallExpr->GetFunctionName());
                exit(1);
            }

            DataType returnType = func->GetReturnType();
            auto *var = builder->CreateAlloca(GetTypeFromDataType(builder, returnType), nullptr, variableName);
            m_DebugGenerator->DefineVariable(builder, var, constExpr);

            m_Variables[variableName] = var;
            m_VariablesExpression[variableName] = value;

            llvm::Value * funcReturnVal = InsertFunctionCallExpression(builder, funcCallExpr);
            builder->CreateStore(funcReturnVal, var);
        } else if (auto *structConstrExpr = dynamic_cast<StructConstructionExpression *>(value)) {
            std::string structName = structConstrExpr->GetStructName();
            if (!m_Structs.contains(structName)) {
                COMPILER_ERROR("A struct with the name \"{0}\" does not exist", structName);
                exit(1);
            }

            llvm::StructType * structType = m_Structs[structName];
            StructExpression * structExpr = m_StructsDefinitions[structName];
            llvm::Type * structPointerType = llvm::PointerType::get(structType, 0);
            auto *var = builder->CreateAlloca(structPointerType, nullptr, variableName);
            //m_DebugGenerator->DefineVariable(builder, var, structConstrExpr);

            m_Variables[variableName] = var;
            m_VariablesExpression[variableName] = value;

            std::vector<llvm::Value *> ops;
            auto dataLayout = m_Module->getDataLayout();
            ops.push_back(builder->getInt64(dataLayout.getStructLayout(structType)->getSizeInBytes()));

            auto * structData = builder->CreateCall(GetCLibraryFunction("malloc"), llvm::ArrayRef(ops));
            auto * value = builder->CreateBitCast(structData, structPointerType);
            builder->CreateStore(value, var);

            for (const auto &attribute : structConstrExpr->GetAttributes()) {
                auto * attributePointer = builder->CreateStructGEP(
                    structType,
                    value,
                    structExpr->GetPropertyIndex(attribute->GetVariableName())
                );

                builder->CreateStore(GetValueFromExpression(builder, attribute->GetValue()), attributePointer);
            }
        } else {
            COMPILER_ERROR("Const assignment for \"{0}\" with unknown type: {1}", variableName, value->GetClassName());
            exit(1);
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

    void CodeGenerator::InsertFuncReturnExpression(llvm::IRBuilder<> *builder, FunctionReturnExpression *retExpr) {
        builder->CreateRet(GetValueFromExpression(builder, retExpr->GetValue()));
    }

    llvm::AllocaInst * CodeGenerator::InsertIntExpression(llvm::IRBuilder<> *builder, const std::string &variableName,
                                            IntExpression *intExpr) {
        auto *var = builder->CreateAlloca(GetVariableTypeForInt(builder, intExpr->GetType()), nullptr, variableName);
        m_Variables[variableName] = var;
        m_VariablesExpression[variableName] = intExpr;

        builder->CreateStore(GetIntValue(builder, intExpr), var);

        return var;
    }

    DataType CodeGenerator::GetVariableDeclarationType(VariableDeclarationExpression *expr) {
        DataType type = expr->GetVariableType();

        if (type != DataType::Unknown) {
            return type;
        }

        auto * value = expr->GetValue();

        if (dynamic_cast<StringExpression *>(value)) {
            return DataType::String;
        } else if (auto *intExpr = dynamic_cast<IntExpression *>(value)) {
            return static_cast<DataType>(GetTypeFromValue(intExpr->GetValue()));
        } else if (auto *funcCallExpr = dynamic_cast<FunctionCallExpression *>(value)) {
            auto * funcDef = m_FunctionsDefinitions[funcCallExpr->GetFunctionName()];
            return funcDef->GetReturnType();
        } else {
            COMPILER_ERROR("Not supported variable value type: {0}", value->GetClassName());
            exit(1);
        }
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
        } else if (auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(variableExpr)) {
            auto * funcDef = m_FunctionsDefinitions[funcCallExpr->GetFunctionName()];
            return builder->CreateLoad(GetTypeFromDataType(builder, funcDef->GetReturnType()), m_Variables[variableName]);
        } else if (auto * structConstrExpr = dynamic_cast<StructConstructionExpression *>(variableExpr)) {
            auto * structType = m_Structs[structConstrExpr->GetStructName()];
            llvm::Type * structPointerType = llvm::PointerType::get(structType, 0);

            return builder->CreateLoad(structPointerType, m_Variables[variableName]);
        } else {
            COMPILER_ERROR("Unsupported expressions for variable values found: {0}", variableExpr->GetClassName());
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

            llvm::Value * compareResult = builder->CreateCall(GetCLibraryFunction("strcmp"), llvm::ArrayRef(params));

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

    llvm::Function *CodeGenerator::GetCLibraryFunction(const std::string &functionName) {

        if (m_Functions.contains(functionName)) {
            return m_Functions[functionName];
        }

        if (functionName == "printf") {

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
            return printfFunc;
        } else if (functionName == "strcmp") {
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
            return strcmpFunc;
        } else if (functionName == "malloc") {
            /////// void * malloc(size: size_t)
            auto *mallocFuncType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getVoidTy(m_Context), 0),
                std::vector<llvm::Type *>({
                  llvm::Type::getInt64Ty(m_Context),
                }),
                false
            );

            llvm::Function *mallocFunc = llvm::Function::Create(
                    mallocFuncType,
                    llvm::Function::ExternalLinkage,
                    "malloc",
                    m_Module
            );

            m_Functions["malloc"] = mallocFunc;
            return mallocFunc;
        }

        COMPILER_ERROR("No standard function with the name \"{0}\" exists", functionName);
        exit(1);
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
}