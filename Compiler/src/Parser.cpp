#include "Parser.h"


namespace Hunter::Compiler {
    void AbstractSyntaxTree::Dump() {
        for (const auto &expression : m_Expressions) {
            if (expression) {
                expression->Dump();
            }
        }
    }

    AbstractSyntaxTree *Parser::Parse(const std::string &input) {
        auto *tree = new AbstractSyntaxTree;

        for (char const &c: input) {

            if (c == '\n') {
                std::cout << m_DataStr << std::endl;
                m_CurrentExpression = ParseLine(m_DataStr);
                tree->AddExpression(m_CurrentExpression);

                m_DataStr = "";
                continue;
            }

//            if (isspace(c)) {
//                if (m_CurrentToken == Token::STRING) {
//                    m_DataStr.push_back(c);
//                }
//                else if (m_PreviousToken == Token::KEYWORD_FUNCTION && !m_DataStr.empty()) {
//                    m_CurrentExpression = new FunctionExpression(m_DataStr);
//                    tree->AddExpression(m_CurrentExpression);
//                    m_DataStr = "";
//                }
//                else if (!m_DataStr.empty()) {
//                    std::cout << m_DataStr << " <- " << static_cast<int>(GetCurrentToken()) << std::endl;
//                    m_DataStr = "";
//                }
//                continue;
//            }
//            else if (c == '(') {
//                if (!m_DataStr.empty()) {
//                    std::cout << m_DataStr << " <- " << static_cast<int>(GetCurrentToken()) << std::endl;
//                    m_DataStr = "";
//                }
//
//                continue;
//            }
//            else if (c == ')') {
//                if (!m_DataStr.empty()) {
//                    std::cout << m_DataStr << " <- " << static_cast<int>(GetCurrentToken()) << std::endl;
//                    m_DataStr = "";
//                }
//
//                continue;
//            }
//            else if (c == '"') {
//
//                m_PreviousToken = m_CurrentToken;
//                m_CurrentToken = Token::STRING;
//
//                if (m_PreviousToken == Token::STRING) {
//                    m_CurrentExpression = new StringExpression(m_DataStr);
//                    tree->AddExpression(m_CurrentExpression);
//                }
//
//                if (!m_DataStr.empty()) {
//                    std::cout << m_DataStr << " <- " << static_cast<int>(GetCurrentToken()) << std::endl;
//                    m_DataStr = "";
//                }
//
//                continue;
//            }

            m_DataStr.push_back(c);
        }

        return tree;
    }

    Expression *Parser::ParseLine(const std::string &input) {
        int level = 0;
        bool isLevelParsing = true;

        std::string str = "";

        for (int i = 0; i < input.length(); ++i) {
            char c = input.at(i);

            if (isspace(c) && isLevelParsing) {
                level += 1;
                continue;
            } else if (!isspace(c) && isLevelParsing) {
                isLevelParsing = false;
            }

            if (isspace(c)) {
                std::cout << "Word: " << str << std::endl;

                if (str == "fun") {
                    ParseResult result = ParseFunctionHeader(i, input);
                    i = result.Pos+1;
                    result.Expr->Dump();
                }

                str = "";
                continue;
            }

            if (c == '(') {
                std::cout << "Word: " << str << std::endl;
                str = "";
                continue;
            }

            if (c == ')') {
                std::cout << "Word: " << str << std::endl;
                str = "";
                continue;
            }

            if (c == '"') {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseString(i, input);
                i = result.Pos+1;
                result.Expr->Dump();

                str = "";
                continue;
            }

            str.push_back(c);
        }

        std::cout << "Word: " << str << std::endl;

        std::cout << "Level: " << level << std::endl;

        return nullptr;
    }

    ParseResult Parser::ParseString(int currentPos, const std::string &input) {
        std::string str = "";

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);
            if (c == '"') {
                break;
            }

            str.push_back(c);
        }


        return {
            .Pos = currentPos,
            .Expr = new StringExpression(str),
        };
    }

    ParseResult Parser::ParseFunctionHeader(int currentPos, const std::string &input) {

        std::string functionName = "";
        bool isNameParsing = true;
        bool isParametersParsing = false;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) && isNameParsing) {
                isNameParsing = false;
                continue;
            }

            if (isspace(c)) {
                continue;
            }

            if (c == '(') {
                isNameParsing = false;
                isParametersParsing = true;
                continue;
            }

            if (isParametersParsing && c == ')') {
                break;
            }

            if (isNameParsing) {
                functionName.push_back(c);
            }

        }

            return {
                .Pos = currentPos,
                .Expr = new FunctionExpression(functionName)
            };
    }

    Token Parser::GetCurrentToken() {
        m_PreviousToken = m_CurrentToken;

        if (m_DataStr == "fun") {
            m_CurrentToken = Token::KEYWORD_FUNCTION;
        } else if (m_DataStr == "print") {
            m_CurrentToken = Token::KEYWORD_PRINT;
        } else if (m_CurrentToken == Token::KEYWORD_FUNCTION) {
            m_CurrentToken = Token::IDENTIFIER;
        }

        return m_CurrentToken;
    }
}