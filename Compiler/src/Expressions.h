#pragma once


namespace Hunter::Compiler {
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

    enum class IntType {
        i8 = 8,
        i16 = 16,
        i32 = 32,
        i64 = 64
    };

    IntType GetTypeFromValue(int64_t val);

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
}