#include "Parser.h"
#include "Expressions.h"


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
                } else if (m_CurrentExpression) {
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
                    m_CurrentLevel = level;
                    m_IsParsingBlock = false;
                }

            }

            if (isspace(c) || c == '(') {
                std::cout << "Word: " << str << std::endl;

                if (str == "fun") {
                    ParseResult result = ParseFunctionHeader(i, input);
                    i = result.Pos+1;
                    expr = result.Expr;

                    m_IsParsingBlock = true;
                } else if (str == "print") {
                    ParseResult result = ParseFunctionCall(i-1, input);
                    i = result.Pos+1;
                    expr = new PrintExpression(result.Expr);
                } else if (str == "const") {
                    ParseResult result = ParseConst(i, input);
                    i = result.Pos+1;
                    expr = result.Expr;
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

            else if (c == '(') {
                std::cout << "Word: " << str << std::endl;
                str = "";
                continue;
            }

            else if (c == ')') {
                std::cout << "Word: " << str << std::endl;
                str = "";
                continue;
            }

            else if (c == '"') {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseString(i, input);
                i = result.Pos+1;
                currentPos = result.Pos+1;
                expr = result.Expr;

                str = "";
                continue;
            }

            // parse identifier
            else if (isalpha(c) && str.empty()) {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseIdentifier(i-2, input);
                i = result.Pos+1;
                currentPos = result.Pos+1;
                expr = result.Expr;

                str = "";
                continue;
            }

            // parse number
            else if ((isnumber(c) || c == '-') && str.empty()) {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseInt(i-1, input);
                i = result.Pos+1;
                currentPos = result.Pos+1;
                expr = result.Expr;

                if (!expr) {
                    std::cerr << "Could not parse int number" << std::endl;
                    exit(1);
                }

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

    ParseResult Parser::ParseConst(int currentPos, const std::string &input) {

        std::string str;
        std::string variableName;
        Expression * value;

        bool isParsingVariable = false;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);
            if (isspace(c) && isParsingVariable) {
                variableName = str;
                str = "";
                isParsingVariable = false;
            } else if (isspace(c)) {
                continue;
            } else if (variableName.empty() && !isParsingVariable) {
                isParsingVariable = true;
            } else if (!variableName.empty() && c == '=') {
                ParseResult result = ParseExpression(currentPos+1, input);
                value = result.Expr;
                currentPos = result.Pos+1;
            }

            if (isParsingVariable) {
                str.push_back(c);
            }
        }

        if (!value) {
            std::cerr << "Could not parse constant value" << std::endl;
            exit(1);
        }

        return {
                .Pos = currentPos,
                .Expr = new ConstExpression(variableName, value)
        };
    }

    ParseResult Parser::ParseInt(int currentPos, const std::string &input) {

        std::string str;
        bool isNegative = false;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (c == '-') {
                isNegative = true;
                continue;
            }

            if (!isnumber(c)) {
                break;
            }

            str.push_back(c);
        }

        int64_t value = std::stoll(str);

        if (isNegative) {
            value *= -1;
        }

        IntType type = GetTypeFromValue(value);

        return {
            .Pos = currentPos,
            .Expr = new IntExpression(type, value)
        };
    }

    ParseResult Parser::ParseFunctionCall(int currentPos, const std::string &input) {
        std::vector<Expression *> parameters;

        std::string str;
        bool isParsingParameter = false;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) || c == ',' || c == '(' || c == ')') {
                isParsingParameter = false;
                if (!str.empty()) {
                    std::cout << str << std::endl;
                    ParseResult result;

                    if (str.starts_with("\"")) {
                        result = ParseString(0, str);
                    } else {
                        result = ParseExpression(0, str);
                    }


                    if (!result.Expr) {
                        std::cerr << "Could not parse expression: " << str << std::endl;
                        exit(1);
                    }

                    parameters.push_back(result.Expr);

                    str = "";
                }

                continue;
            } else if (!isParsingParameter) {
                isParsingParameter = true;
            }

            str.push_back(c);
        }

        return {
                .Pos = currentPos,
                .Expr = new FunctionCallExpression(parameters)
        };
    }

    ParseResult Parser::ParseIdentifier(int currentPos, const std::string &input) {
        std::string str;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (!isalnum(c) && c != '_') {
                break;
            }

            str.push_back(c);
        }

        return {
            .Pos = currentPos,
            .Expr = new IdentifierExpression(str)
        };
    }

    IntType GetTypeFromValue(int64_t val) {
        if (val >= INT8_MIN && val <= INT8_MAX) {
            return IntType::i8;
        }
        else if (val >= INT16_MIN && val <= INT16_MAX) {
            return IntType::i16;
        }
        else if (val >= INT32_MIN && val <= INT32_MAX) {
            return IntType::i32;
        }

        return IntType::i64;
    }
}