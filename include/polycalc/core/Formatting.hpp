#pragma once

#include <string>

namespace polycalc {

// Converts a non-negative integer exponent to Unicode superscript digits (e.g. 12 -> "¹²").
std::string toSuperscript(int exponent);

// Renders a coefficient as a compact string: integral values print without a
// decimal point, fractional values print trimmed to a reasonable precision.
std::string formatCoefficient(double value);

// Decodes a run of Unicode superscript digits back to an integer (e.g. "¹²" -> 12).
// Returns false if any character in text is not a superscript digit.
bool fromSuperscript(const std::string& text, int& exponent);

} // namespace polycalc
