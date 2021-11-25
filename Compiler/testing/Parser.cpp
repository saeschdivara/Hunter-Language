#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include "../src/Expressions.h"
#include "../src/Parser.h"

using namespace Hunter::Compiler;

TEST_CASE( "Simple instructions are parsed", "[parser]" ) {

    SECTION("print with static string instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(R"( print("Hello 8\n"))");
        REQUIRE( dynamic_cast<PrintExpression *>(expr) );

        auto * printExpr = dynamic_cast<PrintExpression *>(expr);
        REQUIRE( dynamic_cast<FunctionCallExpression *>(printExpr->GetInput()) );

        auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(printExpr->GetInput());
        REQUIRE(funcCallExpr->GetParameters().size() == 1);

        auto * strExpression = dynamic_cast<StringExpression *>(funcCallExpr->GetParameters().at(0));
        REQUIRE(strExpression);
        REQUIRE(strExpression->GetString() == "Hello 8\n");
    }

    SECTION("print with const string var instruction") {
        Parser parser;

        parser.ParseLine(R"( const helloWorld = "Hello World")");
        auto * expr = parser.ParseLine(R"( print(helloWorld, "\n"))");
        REQUIRE( dynamic_cast<PrintExpression *>(expr) );

        auto * printExpr = dynamic_cast<PrintExpression *>(expr);
        REQUIRE( dynamic_cast<FunctionCallExpression *>(printExpr->GetInput()) );

        auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(printExpr->GetInput());
        REQUIRE(funcCallExpr->GetParameters().size() == 2);

        auto * identifierExpression = dynamic_cast<IdentifierExpression *>(funcCallExpr->GetParameters().at(0));
        REQUIRE(identifierExpression);
        REQUIRE(identifierExpression->GetVariableName() == "helloWorld");

        auto * strExpression = dynamic_cast<StringExpression *>(funcCallExpr->GetParameters().at(1));
        REQUIRE(strExpression);
        REQUIRE(strExpression->GetString() == "\n");
    }

    SECTION("const instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(R"( const my_str = "Foo Bar")");
        REQUIRE( dynamic_cast<ConstExpression *>(expr) );

        auto * constExpr = dynamic_cast<ConstExpression *>(expr);
        REQUIRE( constExpr->GetVariableName() == "my_str" );

        REQUIRE( dynamic_cast<StringExpression *>(constExpr->GetValue()) );

        auto * strExpr = dynamic_cast<StringExpression *>(constExpr->GetValue());
        REQUIRE( strExpr->GetString() == "Foo Bar" );
    }

    SECTION("variable assignment instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(" foo = foo + 1");
        REQUIRE( dynamic_cast<VariableMutationExpression *>(expr) );

        auto * varMutationExpr = dynamic_cast<VariableMutationExpression *>(expr);
        REQUIRE(varMutationExpr->GetVariableName() == "foo");

        REQUIRE( dynamic_cast<OperationExpression *>(varMutationExpr->GetValue()) );

        auto * operationExpr = dynamic_cast<OperationExpression *>(varMutationExpr->GetValue());
        REQUIRE( operationExpr->GetOperator() == OperatorType::MathPlus );
        REQUIRE( dynamic_cast<IdentifierExpression *>(operationExpr->Left()) );
        REQUIRE( dynamic_cast<IntExpression *>(operationExpr->Right()) );

        auto * identifierExpr = dynamic_cast<IdentifierExpression *>(operationExpr->Left());
        REQUIRE( identifierExpr->GetVariableName() == "foo" );

        auto * intExpr = dynamic_cast<IntExpression *>(operationExpr->Right());
        REQUIRE( intExpr->GetType() == IntType::i8 );
        REQUIRE( intExpr->GetValue() == 1 );
    }

    SECTION("function declaration with parameter instruction") {
        Parser parser;

        auto * expr = parser.ParseLine("fun foo(num: i8)");
        REQUIRE( dynamic_cast<FunctionExpression *>(expr) );

        auto * funcExpr = dynamic_cast<FunctionExpression *>(expr);
        REQUIRE( funcExpr->GetName() == "foo" );

        auto parameters = funcExpr->GetParameters();
        REQUIRE( parameters.size() == 1 );

        auto * parameter = parameters.at(0);
        REQUIRE( parameter->GetName() == "num" );
        REQUIRE( parameter->GetDataType() == DataType::i8 );
    }

    SECTION("function declaration without parameter instruction") {
        Parser parser;

        auto * expr = parser.ParseLine("fun foo()");
        REQUIRE( dynamic_cast<FunctionExpression *>(expr) );

        auto * funcExpr = dynamic_cast<FunctionExpression *>(expr);
        REQUIRE( funcExpr->GetName() == "foo" );

        auto parameters = funcExpr->GetParameters();
        REQUIRE( parameters.empty() );
    }

    SECTION("function call instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(" foo()");
        REQUIRE( dynamic_cast<FunctionCallExpression *>(expr) );

        auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(expr);
        REQUIRE( funcCallExpr->GetParameters().empty() );
        REQUIRE( funcCallExpr->GetFunctionName() == "foo" );
    }

    SECTION("if instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(R"( if helloWorld eq \"Hello World\" then)");
        REQUIRE( dynamic_cast<IfExpression *>(expr) );

        auto * ifExpr = dynamic_cast<IfExpression *>(expr);
        REQUIRE( dynamic_cast<BooleanExpression *>(ifExpr->GetCondition()) );

        auto * conditionExpr = dynamic_cast<BooleanExpression *>(ifExpr->GetCondition());
        REQUIRE( conditionExpr->GetOperator() == OperatorType::LogicalEquals );
        REQUIRE( dynamic_cast<IdentifierExpression *>(conditionExpr->Left()) );
        REQUIRE( dynamic_cast<StringExpression *>(conditionExpr->Right()) );
    }
}