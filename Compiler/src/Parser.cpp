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

                if (m_DataStr.empty()) {
                    continue;
                }

                OnLineFinished(tree);

                continue;
            }

            m_DataStr.push_back(c);
        }

        if (!m_DataStr.empty()) {
            OnLineFinished(tree);
        }

        return tree;
    }

    void Parser::OnLineFinished(AbstractSyntaxTree * tree) {

        m_CurrentExpression = ParseLine(m_DataStr);

        if (!m_CurrentExpression) {
            std::cerr << "Could not parse valid expression from current line" << std::endl;
            exit(1);
        }

        if (m_IsParsingBlock) {
            if (auto *ifExpr = dynamic_cast<IfExpression *>(m_BlockExpressions.top())) {

                if (auto * elseExpr = dynamic_cast<ElseExpression *>(m_CurrentExpression)) {
                    m_BlockExpressions.pop();
                    ifExpr->SetElse(elseExpr);
                } else {
                    ifExpr->AddExpression(m_CurrentExpression);
                }

            } else if (auto *blockExpr = dynamic_cast<BlockExpression *>(m_BlockExpressions.top())) {
                blockExpr->AddExpression(m_CurrentExpression);
            }

            if (m_CurrentExpression->HasBlock()) {

                m_BlockExpressions.push(m_CurrentExpression);
                m_IsParsingBlock = true;
            }
        } else {
            tree->AddExpression(m_CurrentExpression);

            if (m_CurrentExpression->HasBlock()) {
                m_BlockExpressions.push(m_CurrentExpression);
                m_IsParsingBlock = true;
            }
        }

        m_DataStr = "";
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

                    if (!m_BlockExpressions.empty()) {
                        m_BlockExpressions.pop();
                    }

                    if (m_BlockExpressions.empty()) {
                        m_IsParsingBlock = false;
                    }
                }

            }

            if (isspace(c) || c == '(') {
                std::cout << "Word 1: " << str << std::endl;

                if (str == "fun") {
                    ParseResult result = ParseFunctionHeader(i, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else if (str == "print") {
                    ParseResult result = ParseFunctionCall(i-1, input);
                    dynamic_cast<FunctionCallExpression *>(result.Expr)->SetFunctionName("print");

                    i = result.Pos+1;
                    expr = new PrintExpression(result.Expr);
                } else if (str == "const") {
                    ParseResult result = ParseVariableDeclaration(i, input, VariableHandlingType::Const);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else if (str == "let") {
                    ParseResult result = ParseVariableDeclaration(i, input, VariableHandlingType::Let);
                    i = result.Pos+1;
                    expr = result.Expr;
                }  else if (str == "if") {
                    ParseResult result = ParseIf(i, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                }   else if (str == "while") {
                    ParseResult result = ParseBoolean(i, input);
                    i = result.Pos+1;
                    expr = new WhileExpression(result.Expr);
                }  else if (str == "for") {
                    ParseResult result = ParseFor(i, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else {

                    ParseResult result = ParseIdentifier(-1, str);
                    if (result.Expr) {
                        if (c == '(') {
                            result = ParseFunctionCall(i, input);
                            dynamic_cast<FunctionCallExpression *>(result.Expr)->SetFunctionName(str);
                        } else {
                            result = ParseVariableDeclaration(i-str.length()-1, input, VariableHandlingType::Assign);
                        }

                        expr = result.Expr;
                        i = result.Pos+1;
                    } else {
                        std::cerr << "Unknown keyword: " << str << std::endl;
                        exit(1);
                    }

                }

                if (!expr) {
                    std::cerr << "Parsing produced invalid expression" << std::endl;
                    exit(1);
                }

                str = "";
                continue;
            }

            str.push_back(c);
        }

        std::cout << "Word 2: " << str << std::endl;

        if (!str.empty()) {
            // this happens if there is a single keyword like 'else' and there is no space left afterwards
            if (str == "else") {
                expr = new ElseExpression();
            } else {
                std::cerr << "Unknown keyword was found" << std::endl;
                exit(1);
            }
        }

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
                break;
            }

            // parse identifier
            else if (isalpha(c) && str.empty()) {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseIdentifier(i-1, input);
                i = result.Pos+1;
                currentPos = result.Pos+1;
                expr = result.Expr;

                str = "";
                break;
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
                break;
            }

            str.push_back(c);
        }

        if (!expr) {
            OperatorType operatorType = GetOperatorFromString(str);

            if (operatorType != OperatorType::NoOperator) {
                expr = new OperationExpression(operatorType);
            } else {
                std::cerr << "Could not parse valid expression from " << input << std::endl;
                exit(1);
            }

        }

        return {
            .Pos = currentPos+1,
            .Expr = expr
        };
    }

    ParseResult Parser::ParseFullExpression(int currentPos, const std::string &input) {
        std::vector<Expression *> ops;
        std::string str;

        for (int i = currentPos; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) && !str.empty()) {

                if (str.at(0) == '"' && str.at(str.length()-1) != '"') {
                    str.push_back(c);
                    continue;
                }

                std::cout << "Current expr: " << str << std::endl;

                ParseResult result = ParseExpression(-1, str);

                if (!result.Expr) {
                    std::cerr << "Could not parse expression" << std::endl;
                    exit(1);
                }

                auto * operationExpr = dynamic_cast<OperationExpression *>(result.Expr);
                if (operationExpr) {
                    if (GetOperandsNumber(operationExpr->GetOperator()) == 2) {
                        if (ops.empty()) {
                            std::cerr << "Operation " << GetOperatorString(operationExpr->GetOperator()) << " expects operand befor its use" << std::endl;
                            exit(1);
                        } else {
                            operationExpr->SetLeft(ops.at(0));
                            ops.pop_back();
                        }
                    }

                    ops.push_back(result.Expr);
                } else if (!ops.empty() && (operationExpr = dynamic_cast<OperationExpression *>(ops.at(0)))) {
                    if (GetOperandsNumber(operationExpr->GetOperator()) == 2) {
                        operationExpr->SetRight(result.Expr);
                    } else {
                        operationExpr->SetLeft(result.Expr);
                    }
                } else {
                    ops.push_back(result.Expr);
                }

                str = "";
                continue;
            } else if (isspace(c)) {
                continue;
            }

            str.push_back(c);
        }

        if (!str.empty()) {
            std::cout << "Current expr: " << str << std::endl;
            ParseResult result = ParseExpression(-1, str);

            if (!result.Expr) {
                std::cerr << "Could not parse expression" << std::endl;
                exit(1);
            }

            auto * operationExpr = dynamic_cast<OperationExpression *>(result.Expr);
            if (operationExpr) {
                if (GetOperandsNumber(operationExpr->GetOperator()) == 2) {
                    if (ops.empty()) {
                        std::cerr << "Operation " << GetOperatorString(operationExpr->GetOperator()) << " expects operand befor its use" << std::endl;
                        exit(1);
                    } else {
                        operationExpr->SetLeft(ops.at(0));
                        ops.pop_back();
                    }
                }

                ops.push_back(result.Expr);
            } else if (!ops.empty() && (operationExpr = dynamic_cast<OperationExpression *>(ops.at(0)))) {
                if (GetOperandsNumber(operationExpr->GetOperator()) == 2) {
                    operationExpr->SetRight(result.Expr);
                } else {
                    operationExpr->SetLeft(result.Expr);
                }
            } else {
                ops.push_back(result.Expr);
            }
        }

        Expression * expr = nullptr;

        if (ops.size() == 1) {
            expr = ops.at(0);
        }

        return {
            .Pos = currentPos,
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
        std::string parameter;
        bool isNameParsing = true;
        bool isParametersParsing = false;

        std::vector<ParameterExpression *> parametersList;

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

            if (isParametersParsing && (c == ',' || c == ')') && !parameter.empty()) {
                std::cout << "Parameter: " << parameter << std::endl;
                auto index = parameter.find(':');

                if (index > parameter.length()) {
                    std::cerr << ": is missing for parameter type" << std::endl;
                    exit(1);
                }

                std::string parameterName = parameter.substr(0, index);
                std::string parameterType = parameter.substr(index+1, parameter.size());

                parametersList.push_back(
                    new ParameterExpression(parameterName, GetDataTypeFromString(parameterType))
                );

                parameter = "";
            }
            else if (isParametersParsing) {
                parameter.push_back(c);
            }
            else if (isNameParsing) {
                functionName.push_back(c);
            }

        }

        return {
            .Pos = currentPos,
            .Expr = new FunctionExpression(functionName, parametersList)
        };
    }

    ParseResult Parser::ParseIf(int currentPos, const std::string &input) {

        std::string str;
        std::string expressionStr;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c)) {
                std::cout << str << std::endl;

                if (str != "then") {
                    expressionStr += str + " ";
                }

                str = "";
                continue;
            }

            str.push_back(c);
        }

        if (str != "then") {
            std::cerr << "If condition has always to be followed by then" << std::endl;
            exit(1);
        }

        Expression * expr = ParseBoolean(0, expressionStr).Expr;

        if (!expr) {
            std::cerr << "Could not parse if boolean expression" << std::endl;
            exit(1);
        }

        return {
            .Pos = currentPos,
            .Expr = new IfExpression(expr)
        };
    }

    ParseResult Parser::ParseFor(int currentPos, const std::string &input) {

        std::string str;
        std::string counterIdentifier;
        Expression * range;

        int currentParsingPart = 1;

        for (int i = currentPos; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) && !str.empty()) {

                std::cout << "part: " << currentParsingPart << " " << str << std::endl;

                if (currentParsingPart == 1) {
                    counterIdentifier = str;
                } else if (currentParsingPart == 2) {
                    if (str != "in") {
                        std::cerr << "Between counter and range there has to be \"in\" keyword" << std::endl;
                        exit(1);
                    }
                } else if (currentParsingPart == 3) {
                    ParseResult result = ParseRange(-1, str);

                    if (!result.Expr) {
                        std::cerr << "Could not parse range expression" << std::endl;
                        exit(1);
                    }

                    range = result.Expr;

                } else {
                    std::cerr << "After range nothing else is expected" << std::endl;
                    exit(1);
                }

                currentParsingPart += 1;

                str = "";
                continue;
            } else if (isspace(c)) {
                continue;
            }

            str.push_back(c);
        }

        if (currentParsingPart == 3) {
            ParseResult result = ParseRange(-1, str);

            if (!result.Expr) {
                std::cerr << "Could not parse range expression" << std::endl;
                exit(1);
            }

            range = result.Expr;
        }

        std::cout << str << std::endl;

        return {
            .Pos = currentPos,
            .Expr = new ForLoopExpression(counterIdentifier, range)
        };
    }

    ParseResult Parser::ParseRange(int currentPos, const std::string &input) {

        std::string str;
        int64_t start = -1;
        int64_t end = -1;

        for (int i = currentPos+1; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);
            if (c == '.') {

                std::cout << "Nr: " << c << " -> " << str << std::endl;

                if (!str.empty()) {
                    if (start == -1) {
                        start = std::stoll(str);
                    } else {
                        end = std::stoll(str);
                    }
                }

                str = "";
                continue;
            } else if (!isnumber(c)) {
                std::cerr << "Found not a number in range: " << c << std::endl;
                exit(1);
            }

            str.push_back(c);
        }

        if (!str.empty()) {
            if (end == -1) {
                end = std::stoll(str);
            }
        }

        return {
            .Pos = currentPos,
            .Expr = new RangeExpression(start, end)
        };
    }

    ParseResult Parser::ParseBoolean(int currentPos, const std::string &input) {

        std::string str;
        Expression * resultExpr;
        OperatorType currentOperator = OperatorType::NoOperator;
        int8_t operandsNumber = 0;
        int8_t currentOperand = 0;

        for (int i = currentPos; i < input.length(); ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) && !str.empty()) {
                OperatorType operatorType = GetOperatorFromString(str);
                if (operatorType != OperatorType::NoOperator) {
                    currentOperator = operatorType;
                    operandsNumber = GetOperandsNumber(currentOperator);

                    currentOperand = operandsNumber > 1 ? 1 : 0;

                    if (resultExpr == nullptr && operandsNumber > 1) {
                        std::cerr << "Operator " << str << " needs " << operandsNumber << " operands" << std::endl;
                        exit(1);
                    }

                } else {
                    ParseResult result = ParseExpression(-1, str);

                    if (currentOperator != OperatorType::NoOperator) {
                        currentOperand += 1;

                        if (currentOperand == operandsNumber) {

                            if (operandsNumber == 1) {
                                resultExpr = new BooleanExpression(currentOperator, result.Expr, nullptr);
                            }
                            else if (operandsNumber == 2) {
                                resultExpr = new BooleanExpression(currentOperator, resultExpr, result.Expr);
                            }

                            currentOperator = OperatorType::NoOperator;
                            operandsNumber = 0;
                            currentOperand = 0;
                        }
                    }
                    else {
                        resultExpr = result.Expr;
                    }
                }

                str = "";
                continue;
            }
            else if (isspace(c)) {
                continue;
            }

            str.push_back(c);
        }

        ParseResult result = ParseExpression(-1, str);

        if (currentOperator != OperatorType::NoOperator) {
            currentOperand += 1;

            if (currentOperand == operandsNumber) {

                if (operandsNumber == 1) {
                    resultExpr = new BooleanExpression(currentOperator, result.Expr, nullptr);
                }
                else if (operandsNumber == 2) {
                    resultExpr = new BooleanExpression(currentOperator, resultExpr, result.Expr);
                }

                currentOperator = OperatorType::NoOperator;
                operandsNumber = 0;
                currentOperand = 0;
            }
        }
        else {
            resultExpr = result.Expr;
        }

        return {
            .Pos = currentPos,
            .Expr = resultExpr
        };
    }

    ParseResult Parser::ParseVariableDeclaration(int currentPos, const std::string &input, VariableHandlingType handlingType) {

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
                ParseResult result = ParseFullExpression(currentPos+2, input);
                value = result.Expr;
                currentPos = result.Pos+1;
            }

            if (isParsingVariable) {
                str.push_back(c);
            }
        }

        if (!value) {
            std::cerr << "Could not parse variable value" << std::endl;
            exit(1);
        }

        Expression * expr;
        if (handlingType == VariableHandlingType::Const) {
            expr = new ConstExpression(variableName, value);
        } else if (handlingType == VariableHandlingType::Let) {
            expr = new LetExpression(variableName, value);
        } else if (handlingType == VariableHandlingType::Assign) {
            expr = new VariableMutationExpression(variableName, value);
        }

        return {
                .Pos = currentPos,
                .Expr = expr
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

                        if (!str.ends_with("\"")) {
                            str.push_back(c);
                            continue;
                        }

                        result = ParseString(0, str);
                    } else {
                        result = ParseExpression(-1, str);
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
}