#include "Expressions.h"
#include "./utils/logger.h"

namespace Hunter::Compiler {

    DataType GetDataTypeFromString(const std::string &typeStr) {
        if (typeStr == "string") {
            return DataType::String;
        }
        else if (typeStr == "memory") {
            return DataType::Memory;
        }
        else if (typeStr == "i8") {
            return DataType::i8;
        }
        else if (typeStr == "i16") {
            return DataType::i16;
        }
        else if (typeStr == "i32") {
            return DataType::i32;
        }
        else if (typeStr == "i64") {
            return DataType::i64;
        }
        else {
            return DataType::Void;
        }
    }

    std::string GetDataTypeString(DataType dataType) {
        switch (dataType) {
            case DataType::i8:
                return "i8";
            case DataType::i16:
                return "i16";
            case DataType::i32:
                return "i32";
            case DataType::i64:
                return "i64";
            case DataType::String:
                return "String";
            case DataType::Memory:
                return "Memory";
            case DataType::Void:
                return "Void";
        }
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
            case OperatorType::LogicalLower:
                return 2;
            case OperatorType::LogicalLowerEquals:
                return 2;
            case OperatorType::LogicalGreater:
                return 2;
            case OperatorType::LogicalGreaterEquals:
                return 2;
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
            case OperatorType::LogicalLower:
                return "<";
            case OperatorType::LogicalLowerEquals:
                return "<=";
            case OperatorType::LogicalGreater:
                return ">";
            case OperatorType::LogicalGreaterEquals:
                return ">=";
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

    DataType VariableDeclarationExpression::GetVariableType() {
        return m_Type;
    }

    uint64_t StructExpression::GetPropertyIndex(const std::string &propertyName) {
        uint64_t counter = 0;
        for (const auto &propertyExpr : GetBody()) {
            auto * p = dynamic_cast<PropertyDeclarationExpression *>(propertyExpr);

            if (p->GetVariableName() == propertyName) {
                return counter;
            }

            counter += 1;
        }

        return -1;
    }
}