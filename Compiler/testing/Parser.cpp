#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include "../src/Expressions.h"
#include "../src/Parser.h"

using namespace Hunter::Compiler;

TEST_CASE( "Simple instructions are parsed", "[parser]" ) {

    SECTION("print instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(" print(\"Hello 8\\n\")");
        REQUIRE( dynamic_cast<PrintExpression *>(expr) );

        auto * printExpr = dynamic_cast<PrintExpression *>(expr);
        REQUIRE( dynamic_cast<FunctionCallExpression *>(printExpr->GetInput()) );

        auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(printExpr->GetInput());
        REQUIRE(funcCallExpr->GetParameters().size() == 1);

        auto * strExpression = dynamic_cast<StringExpression *>(funcCallExpr->GetParameters().at(0));
        REQUIRE(strExpression);
        REQUIRE(strExpression->GetString() == "Hello 8\n");
    }

    SECTION("function call instruction") {
        Parser parser;

        auto * expr = parser.ParseLine(" foo()");
        REQUIRE( dynamic_cast<FunctionCallExpression *>(expr) );

        auto * funcCallExpr = dynamic_cast<FunctionCallExpression *>(expr);
        REQUIRE(funcCallExpr->GetParameters().empty());
    }
}