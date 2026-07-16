#include "polycalc/core/Formatting.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace polycalc {

namespace {

constexpr std::array<const char*, 10> kSuperscriptDigits = {
    "⁰", "¹", "²", "³", "⁴",
    "⁵", "⁶", "⁷", "⁸", "⁹",
};

} // namespace

std::string toSuperscript(int exponent) {
    const std::string digits = std::to_string(exponent);
    std::string result;
    result.reserve(digits.size() * 2);
    for (const char digit : digits) {
        result += kSuperscriptDigits[static_cast<std::size_t>(digit - '0')];
    }
    return result;
}

std::string formatCoefficient(double value) {
    std::ostringstream stream;
    if (std::fabs(value - std::round(value)) < 1e-9) {
        stream << static_cast<long long>(std::llround(value));
    } else {
        stream << std::setprecision(6) << value;
    }
    return stream.str();
}

} // namespace polycalc
