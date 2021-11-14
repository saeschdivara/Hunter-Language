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
        virtual bool HasBlock() { return false; }
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
        Expression * m_CurrentExpression = nullptr;
        Expression * m_CurrentBlockExpression = nullptr;

        bool m_IsParsingBlock = false;
        int m_CurrentLevel = 0;

        Token m_PreviousToken;
        Token m_CurrentToken;
    };

}