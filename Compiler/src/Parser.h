#pragma once

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <stack>

namespace Hunter::Compiler {

    class Expression;

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
        ParseResult ParseIf(int currentPos, const std::string & input);
        ParseResult ParseBoolean(int currentPos, const std::string & input);
        ParseResult ParseConst(int currentPos, const std::string & input);
        ParseResult ParseInt(int currentPos, const std::string & input);
        ParseResult ParseFunctionCall(int currentPos, const std::string & input);
        ParseResult ParseIdentifier(int currentPos, const std::string & input);

    private:
        std::string m_DataStr;
        Expression * m_CurrentExpression = nullptr;
        std::stack<Expression *> m_BlockExpressions;

        bool m_IsParsingBlock = false;
        int m_CurrentLevel = 0;
    };

}