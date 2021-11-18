#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace Hunter::Compiler {

    enum class IntType {
        i8 = 8,
        i16 = 16,
        i32 = 32,
        i64 = 64
    };

    IntType GetTypeFromValue(int64_t val);

    enum class OperatorType {
        NoOperator = 0,

        LogicalEquals,
        LogicalNot,

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

    class FunctionCallExpression : public Expression {
    public:
        FunctionCallExpression(std::vector<Expression *> parameters) : m_Parameters(std::move(parameters)) {}
        std::vector<Expression *> & GetParameters() { return m_Parameters; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Function call Expression: " << std::endl;

            for (const auto &parameter : m_Parameters) {
                parameter->Dump(level+1);
            }
        }

    private:
        std::vector<Expression *> m_Parameters;
    };

    class FunctionExpression : public Expression {
    public:
        FunctionExpression(std::string name) : m_Name(std::move(name)) {}
        std::string & GetName() { return m_Name; }

        void Dump(int level) override {
            DumpSpaces(level);
            std::cout << "Function Expression: " << GetName() << std::endl;

            for (const auto &subExpr : m_Body) {
                subExpr->Dump(level+1);
            }
        }

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
        std::string m_Name;
        std::vector<Expression *> m_Body;
    };

    class ElseExpression : public Expression {
    public:
        ElseExpression() = default;

        void Dump(int level) override {
            DumpSpaces(level+1);
            std::cout << "Else Expression: " << std::endl;

            DumpSpaces(level+1);
            std::cout << "  Body: " << std::endl;
            for (const auto &subExpr : GetBody()) {
                subExpr->Dump(level+3);
            }
        }

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

    class IfExpression : public Expression {
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

        bool HasBlock() override {
            return true;
        }

        void AddExpression(Expression * expr) {
            m_Body.push_back(expr);
        }

        void SetElse(ElseExpression * expr) {
            m_Else = expr;
        }

        ElseExpression * GetElse() {
            return m_Else;
        }

        std::vector<Expression *> & GetBody() {
            return m_Body;
        }

    private:
        Expression * m_Condition;
        ElseExpression * m_Else;
        std::vector<Expression *> m_Body;
    };
}