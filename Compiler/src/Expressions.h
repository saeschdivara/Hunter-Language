#pragma once

#include <string>
#include <utility>
#include <vector>
#include <iostream>

namespace Hunter::Compiler {

    enum class IntType {
        i8 = 8,
        i16 = 16,
        i32 = 32,
        i64 = 64
    };

    enum class DataType {
        Void = 0,
        String  = 1,
        i8      = static_cast<int>(IntType::i8),
        i16     = static_cast<int>(IntType::i16),
        i32     = static_cast<int>(IntType::i32),
        i64     = static_cast<int>(IntType::i64),
    };

    DataType GetDataTypeFromString(const std::string & typeStr);
    std::string GetDataTypeString(DataType dataType);

    IntType GetTypeFromValue(int64_t val);

    enum class OperatorType {
        NoOperator = 0,

        MathPlus,
        MathMinus,
        MathMultiply,
        MathDivide,

        LogicalEquals,
        LogicalNot,
        LogicalLower,
        LogicalLowerEquals,
        LogicalGreater,
        LogicalGreaterEquals,

        BitXor,
        BitOr,
        BitAnd,
        BitNot,
    };

    OperatorType GetOperatorFromString(const std::string & str);
    int8_t GetOperandsNumber(OperatorType operatorType);
    std::string GetOperatorString(OperatorType operatorType);

    class Expression {
    public:
        virtual ~Expression() = default;
        virtual void Dump(int level = 0) = 0;
        virtual bool HasBlock() { return false; }

    protected:
        void DumpSpaces(int level = 0) {
            for (int i = 0; i < level; ++i) {
                std::cout << "  ";
            }
        }
    };

    class ImportExpression : public Expression {
    public:
        ImportExpression(std::string module) : m_Module(std::move(module)) {}

        const std::string &GetModule() const {
            return m_Module;
        }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Import Expression: " << GetModule() << std::endl;
        }

    private:
        std::string m_Module;
    };

    class ModuleExpression : public Expression {
    public:
        ModuleExpression(std::string module) : m_Module(std::move(module)) {}

        const std::string &GetModule() const {
            return m_Module;
        }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Module Expression: " << GetModule() << std::endl;
        }

    private:
        std::string m_Module;
    };

    class BlockExpression : public Expression {
    public:
        bool HasBlock() override {
            return true;
        }

        void AddExpression(Expression * expr) {
            m_Body.push_back(expr);
        }

        std::vector<Expression *> & GetBody() {
            return m_Body;
        }

    private:
        std::vector<Expression *> m_Body;
    };

    class PrintExpression : public Expression {
    public:
        PrintExpression(Expression * expr) : m_Data(expr) {}
        Expression * GetInput() { return m_Data; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Print Expression: " << std::endl;
            GetInput()->Dump(level+1);
        }

    private:
        Expression * m_Data;
    };

    class StringExpression : public Expression {
    public:
        StringExpression(std::string str) : m_Data(std::move(str)) {}
        std::string & GetString() { return m_Data; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "String Expression: " << GetString() << std::endl;
        }

    private:
        std::string m_Data;
    };

    class ConstExpression : public Expression {
    public:
        ConstExpression(std::string name, Expression * value) : m_VariableName(std::move(name)), m_Value(value) {}
        std::string & GetVariableName() { return m_VariableName; }
        Expression * GetValue() { return m_Value; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Const Expression: " << GetVariableName() << " := " << std::endl;
            GetValue()->Dump(level+1);
        }

    private:
        std::string m_VariableName;
        Expression * m_Value;
    };

    class LetExpression : public Expression {
    public:
        LetExpression(std::string name, Expression * value) : m_VariableName(std::move(name)), m_Value(value) {}
        std::string & GetVariableName() { return m_VariableName; }
        Expression * GetValue() { return m_Value; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Let Expression: " << GetVariableName() << " := " << std::endl;
            GetValue()->Dump(level+1);
        }

    private:
        std::string m_VariableName;
        Expression * m_Value;
    };

    class VariableMutationExpression : public Expression {
    public:
        VariableMutationExpression(std::string name, Expression * value) : m_VariableName(std::move(name)), m_Value(value) {}
        std::string & GetVariableName() { return m_VariableName; }
        Expression * GetValue() { return m_Value; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Var Mutation Expression: " << GetVariableName() << " := " << std::endl;
            GetValue()->Dump(level+1);
        }

    private:
        std::string m_VariableName;
        Expression * m_Value;
    };

    class BooleanExpression : public Expression {
    public:
        BooleanExpression(OperatorType operatorType, Expression * leftExpression, Expression * rightExpression)
            : m_Operator(operatorType), m_Left(leftExpression), m_Right(rightExpression) {}

        OperatorType GetOperator() { return m_Operator; }
        Expression * Left() { return m_Left; }
        Expression * Right() { return m_Right; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Boolean Expression: " << std::endl;

            if (GetOperandsNumber(GetOperator()) == 1) {
                DumpSpaces(level+1);
                std::cout << GetOperatorString(GetOperator()) << std::endl;
                Left()->Dump(level+1);
            }

            else if (GetOperandsNumber(GetOperator()) == 2) {
                Left()->Dump(level+1);
                DumpSpaces(level+1);
                std::cout << GetOperatorString(GetOperator()) << std::endl;
                Right()->Dump(level+1);
            }
        }

    private:
        OperatorType m_Operator;
        Expression * m_Left;
        Expression * m_Right;
    };

    class OperationExpression : public Expression {
    public:
        OperationExpression(OperatorType operatorType)
            : m_Operator(operatorType), m_Left(nullptr), m_Right(nullptr) {}

        OperatorType GetOperator() { return m_Operator; }
        Expression * Left() { return m_Left; }
        Expression * Right() { return m_Right; }

        void SetLeft(Expression *left) {
            m_Left = left;
        }

        void SetRight(Expression *right) {
            m_Right = right;
        }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Operation Expression: " << std::endl;

            if (GetOperandsNumber(GetOperator()) == 1) {
                DumpSpaces(level+1);
                std::cout << GetOperatorString(GetOperator()) << std::endl;
                Left()->Dump(level+1);
            }

            else if (GetOperandsNumber(GetOperator()) == 2) {
                Left()->Dump(level+1);
                DumpSpaces(level+1);
                std::cout << GetOperatorString(GetOperator()) << std::endl;
                Right()->Dump(level+1);
            }
        }

    private:
        OperatorType m_Operator;
        Expression * m_Left;
        Expression * m_Right;
    };

    class IntExpression : public Expression {
    public:
        IntExpression(IntType type, int64_t value) : m_Type(type), m_Value(value) {}

        IntType GetType() const { return m_Type; }
        int64_t GetValue() const { return m_Value; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Int Expression: " << GetValue() << " (" << static_cast<int>(GetType()) << ")" << std::endl;
        }

    private:
        IntType m_Type;
        int64_t m_Value;
    };

    class IdentifierExpression : public Expression {
    public:
        IdentifierExpression(std::string name) : m_VariableName(std::move(name)) {}
        std::string & GetVariableName() { return m_VariableName; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Identifier Expression: " << GetVariableName() << std::endl;
        }

    private:
        std::string m_VariableName;
    };

    class FunctionReturnExpression : public Expression {
    public:
        FunctionReturnExpression(Expression * expr) : m_ReturnExpr(expr) {}
        Expression * GetValue() { return m_ReturnExpr; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Function Return Expression: " << std::endl;
            GetValue()->Dump(level+1);
        }

    private:
        Expression * m_ReturnExpr;
    };

    class FunctionCallExpression : public Expression {
    public:
        FunctionCallExpression(std::vector<Expression *> parameters) : m_Parameters(std::move(parameters)) {}
        std::vector<Expression *> & GetParameters() { return m_Parameters; }

        void SetFunctionName(const std::string &functionName) {
            m_FunctionName = functionName;
        }

        const std::string &GetFunctionName() const {
            return m_FunctionName;
        }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Function call Expression: " << GetFunctionName() << std::endl;

            for (const auto &parameter : m_Parameters) {
                parameter->Dump(level+1);
            }
        }

    private:
        std::string m_FunctionName;
        std::vector<Expression *> m_Parameters;
    };

    class ParameterExpression : public Expression {
    public:
        ParameterExpression(std::string name, DataType dataType) : m_Name(std::move(name)), m_DataType(dataType) {}

        std::string & GetName() { return m_Name; }

        DataType GetDataType() const {
            return m_DataType;
        }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Parameter Expression: " << GetName() << " (" << static_cast<int>(GetDataType()) << ")" << std::endl;
        }

    private:
        std::string m_Name;
        DataType m_DataType;
    };

    class FunctionExpression : public BlockExpression {
    public:
        FunctionExpression(std::string name, std::vector<ParameterExpression *>  params)
            : m_Name(std::move(name)), m_Parameters(std::move(params)) {}

        void SetName(const std::string & name) { m_Name = name; }
        std::string & GetName() { return m_Name; }

        const std::vector<ParameterExpression *> &GetParameters() const {
            return m_Parameters;
        }

        DataType GetReturnType() const {
            return m_ReturnType;
        }

        void SetReturnType(DataType returnType) {
            m_ReturnType = returnType;
        }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Function Expression: " << GetName() << " : " << GetDataTypeString(GetReturnType()) << std::endl;

            DumpSpaces(level+1);
            std::cout << "Parameters:" << std::endl;
            for (const auto &subExpr : GetParameters()) {
                subExpr->Dump(level+2);
            }

            DumpSpaces(level+1);
            std::cout << "Body:" << std::endl;
            for (const auto &subExpr : GetBody()) {
                subExpr->Dump(level+2);
            }
        }

    private:
        std::string m_Name;
        std::vector<ParameterExpression *> m_Parameters;
        DataType m_ReturnType = DataType::Void;
    };

    class RangeExpression : public Expression {
    public:
        RangeExpression(int64_t start, int64_t end) : m_Start(start), m_End(end) {}
        int64_t GetStart() const { return m_Start; }
        int64_t GetEnd() const { return m_End; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Range Expression: " << GetStart() << " - " << GetEnd() << std::endl;
        }

    private:
        int64_t m_Start;
        int64_t m_End;
    };

    class ElseExpression : public BlockExpression {
    public:
        void Dump(int level) override {
            DumpSpaces(level+1);
            std::cout << "Else Expression: " << std::endl;

            DumpSpaces(level+1);
            std::cout << "  Body: " << std::endl;
            for (const auto &subExpr : GetBody()) {
                subExpr->Dump(level+3);
            }
        }
    };

    class IfExpression : public BlockExpression {
    public:
        IfExpression(Expression * condition) : m_Condition(condition) {}
        Expression * GetCondition() { return m_Condition; }

        void Dump(int level) override {
            DumpSpaces(level+1);
            std::cout << "If Expression: " << std::endl;

            DumpSpaces(level+1);
            std::cout << "  Condition: " << std::endl;
            GetCondition()->Dump(level+3);

            DumpSpaces(level+1);
            std::cout << "  Body: " << std::endl;
            for (const auto &subExpr : GetBody()) {
                subExpr->Dump(level+3);
            }

            if (GetElse()) {
                GetElse()->Dump(level+1);
            }
        }

        void SetElse(ElseExpression * expr) {
            m_Else = expr;
        }

        ElseExpression * GetElse() {
            return m_Else;
        }

    private:
        Expression * m_Condition;
        ElseExpression * m_Else;
    };

    class WhileExpression : public BlockExpression {
    public:
        WhileExpression(Expression * condition) : m_Condition(condition) {}
        Expression * GetCondition() { return m_Condition; }

        void Dump(int level) override {
            DumpSpaces(level+1);
            std::cout << "While Expression: " << std::endl;

            DumpSpaces(level+1);
            std::cout << "  Condition: " << std::endl;
            GetCondition()->Dump(level+3);

            DumpSpaces(level+1);
            std::cout << "  Body: " << std::endl;
            for (const auto &subExpr : GetBody()) {
                subExpr->Dump(level+3);
            }
        }

    private:
        Expression * m_Condition;
    };

    class ForLoopExpression : public BlockExpression {
    public:
        ForLoopExpression(std::string counterIdentifier, Expression * range)
            : m_CounterIdentifier(std::move(counterIdentifier)), m_Range(range) {};

        void Dump(int level) override {
            DumpSpaces(level+1);
            std::cout << "For loop Expression: " << std::endl;

            DumpSpaces(level+1);
            std::cout << "  Counter: " << GetCounter() << std::endl;

            DumpSpaces(level+1);
            std::cout << "  Range: " << std::endl;
            GetRange()->Dump(level + 3);

            DumpSpaces(level+1);
            std::cout << "  Body: " << std::endl;
            for (const auto &subExpr : GetBody()) {
                subExpr->Dump(level+3);
            }
        }

        Expression * GetRange() {
            return m_Range;
        }

        std::string & GetCounter() {
            return m_CounterIdentifier;
        }

    private:
        std::string m_CounterIdentifier;
        Expression * m_Range;
    };
}