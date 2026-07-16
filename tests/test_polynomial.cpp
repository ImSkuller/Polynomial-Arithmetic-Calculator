#include <cmath>
#include <utility>

#include "TestFramework.hpp"
#include "polycalc/core/Polynomial.hpp"

using polycalc::Polynomial;

namespace {

Polynomial makePolynomial(std::initializer_list<std::pair<double, int>> terms) {
    Polynomial polynomial;
    for (const auto& [coefficient, exponent] : terms) {
        polynomial.insertTerm(coefficient, exponent);
    }
    return polynomial;
}

} // namespace

TEST_CASE("insertTerm keeps exponents unique but does not sort") {
    // insertTerm only guarantees no duplicate exponents; sortByExponent()
    // is what imposes descending order.
    Polynomial p = makePolynomial({{3, 0}, {5, 4}, {2, 2}});
    REQUIRE_EQ(p.termCount(), 3u);

    p.sortByExponent();
    const auto terms = p.terms();
    REQUIRE_EQ(terms[0].second, 4);
    REQUIRE_EQ(terms[1].second, 2);
    REQUIRE_EQ(terms[2].second, 0);
}

TEST_CASE("appendTermRaw preserves duplicate exponents and insertion order") {
    Polynomial p;
    p.appendTermRaw(3, 2);
    p.appendTermRaw(4, 2);
    REQUIRE_EQ(p.termCount(), 2u);

    p.mergeLikeTerms();
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE(std::fabs(p.terms()[0].first - 7.0) < 1e-9);
}

TEST_CASE("insertTerm merges duplicate exponents") {
    Polynomial p = makePolynomial({{3, 2}, {4, 2}});
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE(std::fabs(p.terms()[0].first - 7.0) < 1e-9);
}

TEST_CASE("insertTerm merging to zero removes the term") {
    Polynomial p = makePolynomial({{5, 3}, {-5, 3}});
    REQUIRE(p.isZero());
    REQUIRE_EQ(p.termCount(), 0u);
}

TEST_CASE("insertTerm rejects negative exponents") {
    Polynomial p;
    bool threw = false;
    try {
        p.insertTerm(1.0, -2);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("insertTerm ignores a zero coefficient") {
    Polynomial p;
    const bool changed = p.insertTerm(0.0, 3);
    REQUIRE(!changed);
    REQUIRE(p.isZero());
}

TEST_CASE("deleteTerm removes an existing term and reports missing ones") {
    Polynomial p = makePolynomial({{1, 1}, {2, 2}});
    REQUIRE(p.deleteTerm(1));
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE(!p.deleteTerm(99));
}

TEST_CASE("updateCoefficient changes value and removes on zero") {
    Polynomial p = makePolynomial({{4, 2}});
    REQUIRE(p.updateCoefficient(2, 9.0));
    REQUIRE(std::fabs(p.terms()[0].first - 9.0) < 1e-9);
    REQUIRE(p.updateCoefficient(2, 0.0));
    REQUIRE(p.isZero());
    REQUIRE(!p.updateCoefficient(2, 5.0));
}

TEST_CASE("updateExponent relocates a term to its new exponent") {
    Polynomial p = makePolynomial({{3, 1}, {2, 5}});
    REQUIRE(p.updateExponent(1, 8));
    REQUIRE_EQ(p.termCount(), 2u);
    REQUIRE(!p.deleteTerm(1));
    REQUIRE(p.deleteTerm(8));
    REQUIRE(p.deleteTerm(5));
    REQUIRE(p.isZero());
}

TEST_CASE("updateExponent merges into an existing term at the destination") {
    Polynomial p = makePolynomial({{3, 1}, {2, 5}});
    REQUIRE(p.updateExponent(1, 5));
    REQUIRE_EQ(p.termCount(), 1u);
    REQUIRE(std::fabs(p.terms()[0].first - 5.0) < 1e-9);
}

TEST_CASE("degree and termCount reflect polynomial state") {
    Polynomial p;
    REQUIRE_EQ(p.degree(), -1);
    REQUIRE_EQ(p.termCount(), 0u);

    p = makePolynomial({{1, 0}, {2, 3}, {4, 1}});
    REQUIRE_EQ(p.degree(), 3);
    REQUIRE_EQ(p.termCount(), 3u);
}

TEST_CASE("evaluate computes the correct numeric result") {
    // 2x^2 + 3x - 5 at x = 5 -> 2*25 + 15 - 5 = 60
    Polynomial p = makePolynomial({{2, 2}, {3, 1}, {-5, 0}});
    REQUIRE(std::fabs(p.evaluate(5.0) - 60.0) < 1e-9);
}

TEST_CASE("clear empties the polynomial") {
    Polynomial p = makePolynomial({{1, 1}, {2, 2}});
    p.clear();
    REQUIRE(p.isZero());
    REQUIRE_EQ(p.termCount(), 0u);
}

TEST_CASE("copy constructor performs a deep copy") {
    Polynomial original = makePolynomial({{1, 1}, {2, 2}});
    Polynomial copy(original);
    copy.insertTerm(100, 9);
    REQUIRE_EQ(original.termCount(), 2u);
    REQUIRE_EQ(copy.termCount(), 3u);
}

TEST_CASE("copy assignment performs a deep copy") {
    Polynomial original = makePolynomial({{1, 1}});
    Polynomial other;
    other = original;
    other.insertTerm(5, 5);
    REQUIRE_EQ(original.termCount(), 1u);
    REQUIRE_EQ(other.termCount(), 2u);
}

TEST_CASE("move constructor transfers ownership") {
    Polynomial original = makePolynomial({{1, 1}, {2, 2}});
    Polynomial moved(std::move(original));
    REQUIRE_EQ(moved.termCount(), 2u);
    REQUIRE(original.isZero());
}

TEST_CASE("self-assignment leaves the polynomial unchanged") {
    Polynomial p = makePolynomial({{1, 1}, {2, 2}});
    p = p;
    REQUIRE_EQ(p.termCount(), 2u);
}

TEST_CASE("operator+ merges two sorted polynomials") {
    Polynomial a = makePolynomial({{2, 2}, {3, 0}});
    Polynomial b = makePolynomial({{5, 2}, {1, 1}});
    Polynomial sum = a + b;
    REQUIRE(sum == makePolynomial({{7, 2}, {1, 1}, {3, 0}}));
}

TEST_CASE("operator+ is correct even with raw, unmerged, out-of-order operands") {
    Polynomial a;
    a.appendTermRaw(3, 0);
    a.appendTermRaw(2, 2);
    a.appendTermRaw(2, 2); // duplicate, not yet merged

    Polynomial b = makePolynomial({{5, 2}, {1, 1}});

    Polynomial sum = a + b;
    REQUIRE(sum == makePolynomial({{9, 2}, {1, 1}, {3, 0}}));
}

TEST_CASE("operator- subtracts term by term") {
    Polynomial a = makePolynomial({{5, 2}, {3, 0}});
    Polynomial b = makePolynomial({{2, 2}, {3, 0}});
    Polynomial difference = a - b;
    REQUIRE(difference == makePolynomial({{3, 2}}));
}

TEST_CASE("unary negation flips every coefficient") {
    Polynomial a = makePolynomial({{5, 2}, {-3, 0}});
    Polynomial negated = -a;
    REQUIRE(negated == makePolynomial({{-5, 2}, {3, 0}}));
}

TEST_CASE("operator* multiplies and merges like terms") {
    // (x + 1) * (x - 1) = x^2 - 1
    Polynomial a = makePolynomial({{1, 1}, {1, 0}});
    Polynomial b = makePolynomial({{1, 1}, {-1, 0}});
    Polynomial product = a * b;
    REQUIRE(product == makePolynomial({{1, 2}, {-1, 0}}));
}

TEST_CASE("compound assignment operators behave like their binary forms") {
    Polynomial a = makePolynomial({{1, 1}});
    Polynomial b = makePolynomial({{2, 1}});
    Polynomial expectedSum = a + b;
    a += b;
    REQUIRE(a == expectedSum);

    Polynomial c = makePolynomial({{5, 2}});
    Polynomial d = makePolynomial({{1, 2}});
    Polynomial expectedDiff = c - d;
    c -= d;
    REQUIRE(c == expectedDiff);

    Polynomial e = makePolynomial({{1, 1}});
    Polynomial f = makePolynomial({{1, 1}});
    Polynomial expectedProduct = e * f;
    e *= f;
    REQUIRE(e == expectedProduct);
}

TEST_CASE("sortByExponent orders raw, unsorted terms descending") {
    Polynomial p;
    p.appendTermRaw(1, 3);
    p.appendTermRaw(5, 9);
    p.appendTermRaw(2, 5);
    p.sortByExponent();
    const auto terms = p.terms();
    REQUIRE_EQ(terms[0].second, 9);
    REQUIRE_EQ(terms[1].second, 5);
    REQUIRE_EQ(terms[2].second, 3);
}

TEST_CASE("mergeLikeTerms sorts and combines raw duplicate terms") {
    Polynomial p;
    p.appendTermRaw(1, 3);
    p.appendTermRaw(4, 5);
    p.appendTermRaw(3, 3);
    p.mergeLikeTerms();
    const auto terms = p.terms();
    REQUIRE_EQ(terms.size(), 2u);
    REQUIRE_EQ(terms[0].second, 5);
    REQUIRE(std::fabs(terms[0].first - 4.0) < 1e-9);
    REQUIRE_EQ(terms[1].second, 3);
    REQUIRE(std::fabs(terms[1].first - 4.0) < 1e-9);
}

TEST_CASE("simplify removes zero-coefficient terms") {
    Polynomial p = makePolynomial({{3, 2}});
    p.insertTerm(-3, 2);
    p.simplify();
    REQUIRE(p.isZero());
}

TEST_CASE("toString renders proper mathematical notation") {
    Polynomial p = makePolynomial({{5, 4}, {3, 2}, {-7, 0}});
    p.sortByExponent();
    REQUIRE_EQ(p.toString(), std::string("5x") + "⁴" + " + 3x" + "²" + " - 7");
}

TEST_CASE("toString handles unit coefficients and linear terms") {
    Polynomial p = makePolynomial({{1, 2}, {-1, 1}});
    p.sortByExponent();
    REQUIRE_EQ(p.toString(), "x² - x");
}

TEST_CASE("toString faithfully reflects unmerged, unsorted raw terms") {
    Polynomial p;
    p.appendTermRaw(3, 2);
    p.appendTermRaw(4, 2);
    p.appendTermRaw(-1, 0);
    // appendTermRaw prepends, so terms appear in reverse insertion order and
    // the duplicate x^2 terms are not yet combined.
    REQUIRE_EQ(p.toString(), "-1 + 4x² + 3x²");
    REQUIRE_EQ(p.termCount(), 3u);
}

TEST_CASE("toString of the zero polynomial is 0") {
    Polynomial p;
    REQUIRE_EQ(p.toString(), "0");
}

TEST_CASE("equality compares polynomials by value") {
    Polynomial a = makePolynomial({{1, 2}, {2, 0}});
    Polynomial b = makePolynomial({{2, 0}, {1, 2}});
    REQUIRE(a == b);
    Polynomial c = makePolynomial({{1, 2}});
    REQUIRE(a != c);
}
