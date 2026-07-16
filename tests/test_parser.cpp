#include <cmath>

#include "TestFramework.hpp"
#include "polycalc/core/Polynomial.hpp"
#include "polycalc/core/PolynomialParser.hpp"

using polycalc::Polynomial;
using polycalc::PolynomialParser;

TEST_CASE("parses a standard multi-term expression") {
    Polynomial p = PolynomialParser::parse("3x^2 + 4x - 8");
    REQUIRE_EQ(p.termCount(), 3u);
    REQUIRE(std::fabs(p.evaluate(1.0) - (3.0 + 4.0 - 8.0)) < 1e-9);
}

TEST_CASE("parser preserves duplicate exponents until explicitly merged") {
    Polynomial p = PolynomialParser::parse("3x^2 + 4x^2 - x^2");
    REQUIRE_EQ(p.termCount(), 3u);
    REQUIRE(std::fabs(p.evaluate(1.0) - 6.0) < 1e-9);

    p.mergeLikeTerms();
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE(std::fabs(p.terms()[0].first - 6.0) < 1e-9);
}

TEST_CASE("parses a single negative term") {
    Polynomial p = PolynomialParser::parse("-5x^3");
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE_EQ(p.degree(), 3);
    REQUIRE(std::fabs(p.terms()[0].first + 5.0) < 1e-9);
}

TEST_CASE("parses implicit unit coefficients") {
    Polynomial x = PolynomialParser::parse("x");
    REQUIRE(std::fabs(x.evaluate(7.0) - 7.0) < 1e-9);

    Polynomial negX = PolynomialParser::parse("-x^2");
    REQUIRE(std::fabs(negX.evaluate(3.0) + 9.0) < 1e-9);
}

TEST_CASE("parses a bare constant") {
    Polynomial p = PolynomialParser::parse("7");
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE_EQ(p.degree(), 0);
}

TEST_CASE("parses expressions with an explicit multiplication sign") {
    Polynomial p = PolynomialParser::parse("3*x^2+4*x-8");
    Polynomial expected = PolynomialParser::parse("3x^2+4x-8");
    REQUIRE(p == expected);
}

TEST_CASE("toString output round-trips through the parser") {
    Polynomial p;
    p.insertTerm(5, 4);
    p.insertTerm(3, 2);
    p.insertTerm(-7, 0);
    Polynomial reparsed = PolynomialParser::parse(p.toString());
    REQUIRE(p == reparsed);
}

TEST_CASE("parser tolerates arbitrary whitespace") {
    Polynomial p = PolynomialParser::parse("   3x^2   +   4x   -   8   ");
    Polynomial expected = PolynomialParser::parse("3x^2+4x-8");
    REQUIRE(p == expected);
}

TEST_CASE("parser rejects an empty expression") {
    bool threw = false;
    try {
        PolynomialParser::parse("   ");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("parser rejects a negative exponent") {
    bool threw = false;
    try {
        PolynomialParser::parse("3x^-2");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("parser rejects a malformed coefficient") {
    bool threw = false;
    try {
        PolynomialParser::parse("abcx^2");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("parser rejects a dangling operator") {
    bool threw = false;
    try {
        PolynomialParser::parse("3x^2++4x");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}
