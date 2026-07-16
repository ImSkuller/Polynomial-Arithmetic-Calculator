#pragma once

#include <string>

#include "polycalc/core/Polynomial.hpp"

namespace polycalc {

// Parses algebraic expressions such as "3x^2 + 4x - 8" or "5x⁴ - 7" into a
// Polynomial. Understands an optional leading sign per term, an optional
// coefficient, an optional '*' between coefficient and variable, and an
// exponent written either as "^N" or as Unicode superscript digits (the same
// notation Polynomial::toString() produces, so parsing a polynomial's own
// display string round-trips back to an equal polynomial).
class PolynomialParser {
public:
    // Throws std::invalid_argument with a descriptive message if the
    // expression cannot be parsed.
    static Polynomial parse(const std::string& expression);
};

} // namespace polycalc
