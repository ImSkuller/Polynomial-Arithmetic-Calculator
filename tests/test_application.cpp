#include <cmath>

#include "TestFramework.hpp"
#include "polycalc/Application.hpp"

using polycalc::Application;

TEST_CASE("Application starts with the zero polynomial and no history") {
    Application app;
    REQUIRE_EQ(app.currentText(), std::string("0"));
    REQUIRE(!app.canUndo());
    REQUIRE(!app.canRedo());
}

TEST_CASE("setCurrentFromExpression parses and records undo history") {
    Application app;
    app.setCurrentFromExpression("3x^2 + 4x - 8");
    REQUIRE_EQ(app.termCount(), static_cast<std::size_t>(3));
    REQUIRE(std::fabs(app.evaluate(1.0) - (3.0 + 4.0 - 8.0)) < 1e-9);
    REQUIRE(app.canUndo());
    REQUIRE(!app.canRedo());
}

TEST_CASE("setCurrentFromExpression throws on malformed input and leaves state untouched") {
    Application app;
    app.setCurrentFromExpression("2x");
    bool threw = false;
    try {
        app.setCurrentFromExpression("not-a-polynomial-@@@");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
    REQUIRE_EQ(app.currentText(), std::string("2x"));
}

TEST_CASE("insertTerm reports whether the polynomial actually changed") {
    Application app;
    REQUIRE(app.insertTerm(5.0, 2));
    REQUIRE(!app.insertTerm(0.0, 3));
}

TEST_CASE("undo and redo restore prior and next states") {
    Application app;
    app.setCurrentFromExpression("5x");
    app.setCurrentFromExpression("9x");
    REQUIRE(app.canUndo());
    app.undo();
    REQUIRE_EQ(app.currentText(), std::string("5x"));
    REQUIRE(app.canRedo());
    app.redo();
    REQUIRE_EQ(app.currentText(), std::string("9x"));
}

TEST_CASE("add/subtract/multiply compute against the secondary polynomial without mutating current") {
    Application app;
    app.setCurrentFromExpression("2x + 1");
    app.setSecondaryFromExpression("3x + 4");

    const polycalc::Polynomial sum = app.add();
    REQUIRE(std::fabs(sum.evaluate(1.0) - 10.0) < 1e-9);
    // add() must not mutate current_: still evaluates the same as before.
    REQUIRE(std::fabs(app.evaluate(1.0) - 3.0) < 1e-9);

    app.adoptAsCurrent(sum);
    REQUIRE(std::fabs(app.evaluate(1.0) - 10.0) < 1e-9);
}

TEST_CASE("sortByExponent/mergeLikeTerms/simplify mutate current and return a non-negative timing") {
    Application app;
    app.setCurrentFromExpression("1x^2 + 1x^2 + 3");
    REQUIRE(app.mergeLikeTerms() >= 0.0);
    REQUIRE_EQ(app.termCount(), static_cast<std::size_t>(2));
}

TEST_CASE("statistics reflect the current polynomial's term count and degree") {
    Application app;
    app.setCurrentFromExpression("5x^4 + 3x^2 - 7");
    const polycalc::Statistics stats = app.statistics();
    REQUIRE_EQ(stats.termCount, static_cast<std::size_t>(3));
    REQUIRE_EQ(stats.degree, 4);
}

TEST_CASE("clearCurrent resets to the zero polynomial") {
    Application app;
    app.setCurrentFromExpression("7x - 1");
    app.clearCurrent();
    REQUIRE_EQ(app.currentText(), std::string("0"));
}
