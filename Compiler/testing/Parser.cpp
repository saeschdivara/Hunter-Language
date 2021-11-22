#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include "../src/Expressions.h"
#include "../src/Parser.h"

using namespace Hunter::Compiler;

TEST_CASE( "Simple instructions are parsed", "[parser]" ) {

    SECTION("print instruction") {
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

    SECTION("function call instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(" foo()");
        REQUIRE( dynamic_cast<FunctionCallExpression *>(expr) );

        auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(expr);
        REQUIRE( funcCallExpr->GetParameters().empty() );
        REQUIRE( funcCallExpr->GetFunctionName() == "foo" );
    }
}