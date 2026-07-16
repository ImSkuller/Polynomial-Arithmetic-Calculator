#pragma once

#include <string>

#include "polycalc/core/Polynomial.hpp"

namespace polycalc {

// Persists polynomials to and from a small, explicit text format:
//
//   POLYCALC 1
//   <termCount>
//   <coefficient> <exponent>
//   ...
//
// An explicit line format avoids any ambiguity that re-parsing a
// pretty-printed display string could introduce.
class PolynomialStorage {
public:
    // Throws std::runtime_error if the file cannot be opened or written.
    static void save(const Polynomial& polynomial, const std::string& filePath);

    // Throws std::runtime_error if the file cannot be opened, or
    // std::invalid_argument if its contents are malformed.
    static Polynomial load(const std::string& filePath);
};

} // namespace polycalc
