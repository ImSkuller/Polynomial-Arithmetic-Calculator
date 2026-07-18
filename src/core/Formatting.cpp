#include "polycalc/core/Formatting.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace polycalc {

namespace {

constexpr std::array<const char*, 10> kSuperscriptDigits = {
    "⁰", "¹", "²", "³", "⁴",
    "⁵", "⁶", "⁷", "⁸", "⁹",
};

// How close a value must be to its rounded form to print without a decimal
// point. Chosen independently of Polynomial::kEpsilon (which governs when a
// coefficient counts as zero) - the two tolerances mean different things and
// are allowed to diverge if one is ever tuned without the other.
constexpr double kIntegralDisplayTolerance = 1e-9;

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
    if (std::fabs(value - std::round(value)) < kIntegralDisplayTolerance) {
        stream << static_cast<long long>(std::llround(value));
    } else {
        stream << std::setprecision(6) << value;
    }
    return stream.str();
}

bool fromSuperscript(const std::string& text, int& exponent) {
    std::string digits;
    std::size_t pos = 0;
    while (pos < text.size()) {
        bool matched = false;
        for (int digit = 0; digit < static_cast<int>(kSuperscriptDigits.size()); ++digit) {
            const std::string_view symbol = kSuperscriptDigits[static_cast<std::size_t>(digit)];
            if (text.compare(pos, symbol.size(), symbol) == 0) {
                digits += static_cast<char>('0' + digit);
                pos += symbol.size();
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    if (digits.empty()) {
        return false;
    }
    exponent = std::stoi(digits);
    return true;
}

} // namespace polycalc
