#include "Parser.h"


namespace Hunter::Compiler {
    void AbstractSyntaxTree::Dump() {
        std::cout << std::endl << "Dump ast:" << std::endl << "-----------" << std::endl;

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

                if (dynamic_cast<FunctionExpression *>(m_CurrentExpression)) {
                    m_CurrentBlockExpression = m_CurrentExpression;
                    tree->AddExpression(m_CurrentExpression);
                } else if (m_IsParsingBlock) {
                    if (auto * func = dynamic_cast<FunctionExpression *>(m_CurrentBlockExpression)) {
                        func->AddExpression(m_CurrentExpression);
                    }
                } else {
                    m_CurrentBlockExpression = nullptr;
                    tree->AddExpression(m_CurrentExpression);
                }

                m_DataStr = "";
                continue;
            }

            m_DataStr.push_back(c);
        }

        return tree;
    }

    Expression *Parser::ParseLine(const std::string &input) {
        int level = 0;
        bool isLevelParsing = true;

        std::string str;
        Expression * expr = nullptr;

        for (int i = 0; i < input.length(); ++i) {
            char c = input.at(i);

            if (isspace(c) && isLevelParsing) {
                level += 1;
                continue;
            } else if (!isspace(c) && isLevelParsing) {
                isLevelParsing = false;

                if (level <= m_CurrentLevel) {
                    m_IsParsingBlock = false;
                }

                m_CurrentLevel = level;
            }

            if (isspace(c) || c == '(') {
                std::cout << "Word: " << str << std::endl;

                if (str == "fun") {
                    ParseResult result = ParseFunctionHeader(i, input);
                    i = result.Pos+1;
                    expr = result.Expr;

                    m_IsParsingBlock = true;
                } else if (str == "print") {
                    ParseResult result = ParseExpression(i-1, input);
                    i = result.Pos+1;
                    expr = new PrintExpression(result.Expr);
                } else {
                    std::cerr << "Unknown keyword: " << str << std::endl;
                    exit(1);
                }

                str = "";
                continue;
            }

            str.push_back(c);
        }

        std::cout << "Word: " << str << std::endl;

        std::cout << "Level: " << level << std::endl;

        return expr;
    }

    ParseResult Parser::ParseExpression(int currentPos, const std::string &input) {

        std::string str;
        Expression * expr = nullptr;

        for (int i = currentPos+1; i < input.length(); ++i, ++currentPos) {
            char c = input.at(i);

            if (isspace(c)) {
                std::cout << "Word: " << str << std::endl;

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
                expr = result.Expr;

                str = "";
                continue;
            }

            str.push_back(c);
        }

        return {
            .Pos = currentPos+1,
            .Expr = expr
        };
    }

    void replaceAll(std::string& str, const std::string& from, const std::string& to) {
        if(from.empty())
            return;
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }

    ParseResult Parser::ParseString(int currentPos, const std::string &input) {
        std::string str;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);
            if (c == '"') {
                break;
            }

            str.push_back(c);
        }

        replaceAll(str, "\\n", "\n");

        return {
            .Pos = currentPos,
            .Expr = new StringExpression(str),
        };
    }

    ParseResult Parser::ParseFunctionHeader(int currentPos, const std::string &input) {

        std::string functionName;
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