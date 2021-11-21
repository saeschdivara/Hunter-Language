#include "Expressions.h"

namespace Hunter::Compiler {

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


    OperatorType GetOperatorFromString(const std::string &str) {

        if (str == "eq") {
            return OperatorType::LogicalEquals;
        }
        else if (str == "not") {
            return OperatorType::LogicalNot;
        }
        else if (str == "<") {
            return OperatorType::LogicalLower;
        }
        else if (str == "<=") {
            return OperatorType::LogicalLowerEquals;
        }
        else if (str == ">") {
            return OperatorType::LogicalGreater;
        }
        else if (str == ">=") {
            return OperatorType::LogicalGreaterEquals;
        }
        else if (str == "+") {
            return OperatorType::MathPlus;
        }
        else if (str == "-") {
            return OperatorType::MathMinus;
        }
        else if (str == "*") {
            return OperatorType::MathMultiply;
        }
        else if (str == "/") {
            return OperatorType::MathDivide;
        }
        else if (str == "|") {
            return OperatorType::BitOr;
        }
        else if (str == "&") {
            return OperatorType::BitAnd;
        }
        else if (str == "~") {
            return OperatorType::BitNot;
        }

        return OperatorType::NoOperator;
    }

    int8_t GetOperandsNumber(OperatorType operatorType) {
        switch (operatorType) {
            case OperatorType::NoOperator:
                return 0;
            case OperatorType::LogicalEquals:
                return 2;
            case OperatorType::LogicalNot:
                return 1;
            case OperatorType::BitNot:
                return 1;
            case OperatorType::BitOr:
                return 2;
            case OperatorType::BitAnd:
                return 2;
            case OperatorType::BitXor:
                return 2;
            case OperatorType::MathPlus:
                return 2;
            case OperatorType::MathMinus:
                return 2;
            case OperatorType::MathMultiply:
                return 2;
            case OperatorType::MathDivide:
                return 2;
            default:
                return 0;
        }
    }

    std::string GetOperatorString(OperatorType operatorType) {
        switch (operatorType) {
            case OperatorType::NoOperator:
                return "";
            case OperatorType::LogicalEquals:
                return "eq";
            case OperatorType::LogicalNot:
                return "not";
            case OperatorType::MathPlus:
                return "+";
            case OperatorType::MathMinus:
                return "-";
            case OperatorType::MathMultiply:
                return "*";
            case OperatorType::MathDivide:
                return "/";
            case OperatorType::BitNot:
                return "~";
            case OperatorType::BitOr:
                return "|";
            case OperatorType::BitAnd:
                return "&";
            case OperatorType::BitXor:
                return "<not implemented yet>";
        }
    }
}