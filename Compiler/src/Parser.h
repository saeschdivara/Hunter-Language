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

    enum class VariableHandlingType {
        Const,
        Let,
        Assign
    };

    class Parser {
    public:
        AbstractSyntaxTree * Parse(const std::string & input);

        void OnLineFinished(AbstractSyntaxTree * tree);

        Expression * ParseLine(const std::string & input);
        ParseResult ParseImport(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseExpression(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseFullExpression(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseString(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseFunctionHeader(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseIf(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseFor(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseRange(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseBoolean(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseVariableDeclaration(int currentPos, int endPosition, const std::string & input, VariableHandlingType handlingType);
        ParseResult ParseInt(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseFunctionCall(int currentPos, int endPosition, const std::string & input);
        ParseResult ParseIdentifier(int currentPos, int endPosition, const std::string & input);

    private:
        std::string m_DataStr;
        Expression * m_CurrentExpression = nullptr;
        std::stack<Expression *> m_BlockExpressions;

        bool m_IsParsingBlock = false;
        bool m_IsFullLineComment = false;
        int m_CurrentLevel = 0;
    };

}