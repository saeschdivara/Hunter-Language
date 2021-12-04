#include "Parser.h"
#include "Expressions.h"
#include "./utils/logger.h"
#include "./utils/strings.h"

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

        if (!m_CurrentExpression && !m_IsFullLineComment) {
            std::cerr << "Could not parse valid expression from current line" << std::endl;
            exit(1);
        }
        else if (!m_CurrentExpression && m_IsFullLineComment) {
            m_DataStr = "";
            m_IsFullLineComment = false;
            return;
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
        int endPosition = input.length();
        int hashPosition = input.find("#");

        if (hashPosition != std::string::npos) {
            if (input.find('"', hashPosition) == std::string::npos) {
                endPosition = hashPosition-1;
                m_IsFullLineComment = true;
            }
        }

        for (int i = 0; i < endPosition; ++i) {
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

                if (str == "import") {
                    ParseResult result = ParseImport(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else if (str == "fun") {
                    ParseResult result = ParseFunctionHeader(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else if (str == "print") {
                    ParseResult result = ParseFunctionCall(i-1, endPosition, input);
                    dynamic_cast<FunctionCallExpression *>(result.Expr)->SetFunctionName("print");

                    i = result.Pos+1;
                    expr = new PrintExpression(result.Expr);
                } else if (str == "const") {
                    ParseResult result = ParseVariableDeclaration(i, endPosition, input, VariableHandlingType::Const);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else if (str == "let") {
                    ParseResult result = ParseVariableDeclaration(i, endPosition, input, VariableHandlingType::Let);
                    i = result.Pos+1;
                    expr = result.Expr;
                }  else if (str == "if") {
                    ParseResult result = ParseIf(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                }   else if (str == "while") {
                    ParseResult result = ParseBoolean(i, endPosition, input);
                    i = result.Pos+1;
                    expr = new WhileExpression(result.Expr);
                }  else if (str == "for") {
                    ParseResult result = ParseFor(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                }  else if (str == "mod") {
                    ParseResult result = ParseModule(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                }  else if (str == "return") {
                    ParseResult result = ParseFunctionReturn(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                }  else if (str == "extern") {
                    ParseResult result = ParseExtern(i, endPosition, input);
                    i = result.Pos+1;
                    expr = result.Expr;
                } else {

                    ParseResult result = ParseIdentifier(-1, str.length(), str);
                    if (result.Expr) {
                        if (c == '(') {
                            result = ParseFunctionCall(i, endPosition, input);
                            dynamic_cast<FunctionCallExpression *>(result.Expr)->SetFunctionName(str);
                        } else {
                            result = ParseVariableDeclaration(i-str.length()-1, endPosition, input, VariableHandlingType::Assign);
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

    ParseResult Parser::ParseExtern(int currentPos, int endPosition, const std::string &input) {
        std::string str;

        for (int i = currentPos; i < endPosition; ++i, ++currentPos) {
            char c = input.at(i);

            if (isspace(c)) {
                if (str == "fun") {
                    break;
                }

                str = "";
                continue;
            }

            str.push_back(c);
        }

        for (int i = currentPos+1; i < endPosition; ++i, ++currentPos) {
            char c = input.at(i);

            if (!isspace(c)) {
                break;
            }
        }

        ParseResult result = ParseFunctionHeader(currentPos, endPosition, input);

        if (!result.Expr) {
            COMPILER_ERROR("Could not parse function definition for external");
            exit(1);
        }

        return {
            .Pos = result.Pos,
            .Expr = new ExternExpression(result.Expr)
        };
    }

    ParseResult Parser::ParseImport(int currentPos, int endPosition, const std::string &input) {

        std::string str;
        Expression * expr;

        for (int i = currentPos; i < endPosition; ++i, ++currentPos) {
            char c = input.at(i);

            if (isspace(c) && str.empty()) {
                continue;
            } else if (isspace(c)) {
                std::cout << "Import: " << str << std::endl;
                str = "";
                continue;
            }

            str.push_back(c);
        }

        return {
            .Pos = currentPos,
            .Expr = new ImportExpression(str)
        };
    }

    ParseResult Parser::ParseModule(int currentPos, int endPosition, const std::string &input) {

        std::string str;
        Expression * expr;

        for (int i = currentPos; i < endPosition; ++i, ++currentPos) {
            char c = input.at(i);

            if (isspace(c) && str.empty()) {
                continue;
            } else if (isspace(c)) {
                std::cout << "Module: " << str << std::endl;
                break;
            }

            str.push_back(c);
        }

        return {
                .Pos = currentPos,
                .Expr = new ModuleExpression(str)
        };
    }

    ParseResult Parser::ParseExpression(int currentPos,  int endPosition, const std::string &input) {

        std::string str;
        Expression * expr = nullptr;

        for (int i = currentPos+1; i < endPosition; ++i, ++currentPos) {
            char c = input.at(i);

            if (isspace(c)) {
                std::cout << "Word: " << str << std::endl;

                str = "";
                continue;
            }

            else if (c == '(') {

                if (auto * identifierExpr = dynamic_cast<IdentifierExpression *>(expr)) {
                    // this is actually a function call

                    ParseResult result = ParseFunctionCall(i, endPosition, input);
                    dynamic_cast<FunctionCallExpression *>(result.Expr)->SetFunctionName(identifierExpr->GetVariableName());

                    i = result.Pos+1;
                    currentPos = result.Pos+1;
                    expr = result.Expr;

                } else {
                    std::cout << "Word: " << str << std::endl;
                    str = "";
                    continue;
                }

            }

            else if (c == ')') {
                std::cout << "Word: " << str << std::endl;
                str = "";
                continue;
            }

            else if (c == '"') {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseString(i, endPosition, input);
                i = result.Pos+1;
                currentPos = result.Pos+1;
                expr = result.Expr;

                str = "";
                break;
            }

            // parse identifier
            else if (isalpha(c) && str.empty()) {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseIdentifier(i-1, input.length(), input);
                i = result.Pos;
                currentPos = result.Pos;
                expr = result.Expr;

                str = "";
                continue;
            }

            // parse number
            else if ((isnumber(c) || c == '-') && str.empty()) {
                std::cout << "Word: " << str << std::endl;

                ParseResult result = ParseInt(i-1, endPosition, input);
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

    ParseResult Parser::ParseFullExpression(int currentPos,  int endPosition, const std::string &input) {
        std::vector<Expression *> ops;
        std::string str;

        for (int i = currentPos; i < endPosition; ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) && !str.empty()) {

                if (str.at(0) == '"' && str.at(str.length()-1) != '"') {
                    str.push_back(c);
                    continue;
                }

                std::cout << "Current expr: " << str << std::endl;

                ParseResult result = ParseExpression(-1, str.length(), str);

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
            ParseResult result = ParseExpression(-1, str.length(), str);

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

    ParseResult Parser::ParseString(int currentPos,  int endPosition, const std::string &input) {
        std::string str;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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

    ParseResult Parser::ParseFunctionHeader(int currentPos,  int endPosition, const std::string &input) {

        std::string functionName;
        std::string parameter;
        std::string returnTypeExpressionStr;
        bool isNameParsing = true;
        bool isParametersParsing = false;
        bool isReturnParsing = false;

        std::vector<ParameterExpression *> parametersList;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
            char c = input.at(i);

            if (isspace(c) && isNameParsing) {
                isNameParsing = false;
                continue;
            }

            else if (isspace(c)) {
                continue;
            }

            else if (c == '(') {
                isNameParsing = false;
                isParametersParsing = true;
                continue;
            }

            else if (!isParametersParsing && c == ':') {
                isNameParsing = false;
                isParametersParsing = false;
                isReturnParsing = true;
                continue;
            }

            else if (isParametersParsing && (c == ',' || c == ')')) {
                if (!parameter.empty()) {
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

                if (c == ')') {
                    isParametersParsing = false;
                }
            }
            else if (isParametersParsing) {
                parameter.push_back(c);
            }
            else if (isNameParsing) {
                functionName.push_back(c);
            }
            else if (isReturnParsing) {
                returnTypeExpressionStr.push_back(c);
            }

        }

        auto * funcExpr = new FunctionExpression(functionName, parametersList);

        if (!returnTypeExpressionStr.empty()) {
            funcExpr->SetReturnType(GetDataTypeFromString(returnTypeExpressionStr));
        }

        return {
            .Pos = currentPos,
            .Expr = funcExpr
        };
    }

    ParseResult Parser::ParseFunctionReturn(int currentPos, int endPosition, const std::string &input) {
        ParseResult result = ParseExpression(currentPos, endPosition, input);

        return {
            .Pos = result.Pos,
            .Expr = new FunctionReturnExpression(result.Expr)
        };
    }

    ParseResult Parser::ParseIf(int currentPos,  int endPosition, const std::string &input) {

        std::string str;
        std::string expressionStr;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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

        Expression * expr = ParseBoolean(0, expressionStr.length(), expressionStr).Expr;

        if (!expr) {
            std::cerr << "Could not parse if boolean expression" << std::endl;
            exit(1);
        }

        return {
            .Pos = currentPos,
            .Expr = new IfExpression(expr)
        };
    }

    ParseResult Parser::ParseFor(int currentPos,  int endPosition, const std::string &input) {

        std::string str;
        std::string counterIdentifier;
        Expression * range;

        int currentParsingPart = 1;

        for (int i = currentPos; i < endPosition; ++i, currentPos++) {
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
                    ParseResult result = ParseRange(-1, str.length(), str);

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
            ParseResult result = ParseRange(-1, str.length(), str);

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

    ParseResult Parser::ParseRange(int currentPos,  int endPosition, const std::string &input) {

        std::string str;
        int64_t start = -1;
        int64_t end = -1;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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

    ParseResult Parser::ParseBoolean(int currentPos,  int endPosition, const std::string &input) {

        std::string str;
        Expression * resultExpr;
        OperatorType currentOperator = OperatorType::NoOperator;
        int8_t operandsNumber = 0;
        int8_t currentOperand = 0;

        for (int i = currentPos; i < endPosition; ++i, currentPos++) {
            char c = input.at(i);

            if (c == '"') {
                do {
                    str.push_back(c);
                    c = input.at(++i);
                    currentPos++;
                } while (c != '"');
            }

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
                    ParseResult result = ParseExpression(-1, str.length(), str);

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

        if (!str.empty()) {
            ParseResult result = ParseExpression(-1, str.length(), str);

            if (currentOperator != OperatorType::NoOperator) {
                currentOperand += 1;

                if (currentOperand == operandsNumber) {

                    if (operandsNumber == 1) {
                        resultExpr = new BooleanExpression(currentOperator, result.Expr, nullptr);
                    }
                    else if (operandsNumber == 2) {
                        resultExpr = new BooleanExpression(currentOperator, resultExpr, result.Expr);
                    }
                }
            }
            else {
                resultExpr = result.Expr;
            }
        }

        return {
            .Pos = currentPos,
            .Expr = resultExpr
        };
    }

    ParseResult Parser::ParseVariableDeclaration(int currentPos,  int endPosition, const std::string &input, VariableHandlingType handlingType) {

        std::string str;
        std::string variableName;
        Expression * value;

        bool isParsingVariable = false;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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
                ParseResult result = ParseFullExpression(currentPos+2, endPosition, input);
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

    ParseResult Parser::ParseInt(int currentPos,  int endPosition, const std::string &input) {

        std::string str;
        bool isNegative = false;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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

    ParseResult Parser::ParseFunctionCall(int currentPos,  int endPosition, const std::string &input) {
        std::vector<Expression *> parameters;

        std::string str;
        bool isParsingParameter = false;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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

                        result = ParseString(0, str.length(), str);
                    } else {
                        result = ParseExpression(-1, str.length(), str);
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

    ParseResult Parser::ParseIdentifier(int currentPos,  int endPosition, const std::string &input) {
        std::string str;

        for (int i = currentPos+1; i < endPosition; ++i, currentPos++) {
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