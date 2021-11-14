#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace Hunter::Compiler {

    enum class Token {
        UNKNOWN = -1,

        END_OF_LINE = 0,

        KEYWORD_FUNCTION,
        KEYWORD_PRINT,

        IDENTIFIER,
        STRING,
    };

    class Expression {
    public:
        virtual ~Expression() {}
        virtual void Dump() = 0;
    };

    class PrintExpression : public Expression {
    public:
        PrintExpression(Expression * expr) : m_Data(expr) {}
        Expression * GetInput() { return m_Data; }

        void Dump() override {
            std::cout << "Print Expression: " << std::endl;
            std::cout << "    ";
            GetInput()->Dump();
        }

    private:
        Expression * m_Data;
    };

    class StringExpression : public Expression {
    public:
        StringExpression(std::string str) : m_Data(std::move(str)) {}
        std::string & GetString() { return m_Data; }

        void Dump() override {
            std::cout << "String Expression: " << GetString() << std::endl;
        }

    private:
        std::string m_Data;
    };

    class FunctionExpression : public Expression {
    public:
        FunctionExpression(std::string name) : m_Name(std::move(name)) {}
        std::string & GetName() { return m_Name; }

        void Dump() override {
            std::cout << "Function Expression: " << GetName() << std::endl;
        }
    private:
        std::string m_Name;
    };

    class AbstractSyntaxTree {
    public:
        void AddExpression(Expression * expr) {
            m_Expressions.push_back(expr);
        }

        std::vector<Expression *> & GetInstructions() {
            return m_Expressions;
        }

        void Dump();

    private:
        std::vector<Expression *> m_Expressions;
    };

    struct ParseResult {
        int Pos;
        Expression * Expr;
    };

    class Parser {
    public:
        AbstractSyntaxTree * Parse(const std::string & input);

    protected:

        Expression * ParseLine(const std::string & input);
        ParseResult ParseExpression(int currentPos, const std::string & input);
        ParseResult ParseString(int currentPos, const std::string & input);
        ParseResult ParseFunctionHeader(int currentPos, const std::string & input);
        Token GetCurrentToken();

    private:
        std::string m_DataStr;
        Token m_PreviousToken;
        Token m_CurrentToken;
        Expression * m_CurrentExpression = nullptr;
    };

}