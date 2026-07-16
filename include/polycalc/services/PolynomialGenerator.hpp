#pragma once

#include "polycalc/core/Polynomial.hpp"

namespace polycalc {

// Produces randomized polynomials, useful for demos, stress-testing
// arithmetic, and timing operations on non-trivial term counts.
class PolynomialGenerator {
public:
    struct Options {
        int termCount = 4;
        int maxExponent = 6;
        double minCoefficient = -10.0;
        double maxCoefficient = 10.0;
    };

    // Throws std::invalid_argument if the options describe an empty or
    // inverted range.
    static Polynomial generate(const Options& options = Options{});
};

} // namespace polycalc
