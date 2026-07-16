#pragma once

#include <string>

namespace polycalc {

// Converts a non-negative integer exponent to Unicode superscript digits (e.g. 12 -> "¹²").
std::string toSuperscript(int exponent);

// Renders a coefficient as a compact string: integral values print without a
// decimal point, fractional values print trimmed to a reasonable precision.
std::string formatCoefficient(double value);

} // namespace polycalc
