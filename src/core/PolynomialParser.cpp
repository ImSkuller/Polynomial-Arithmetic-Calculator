#include "polycalc/core/PolynomialParser.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "polycalc/core/Formatting.hpp"

namespace polycalc {

namespace {

std::string stripWhitespace(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c)) == 0) {
            result += c;
        }
    }
    return result;
}

double parseCoefficientMagnitude(const std::string& text) {
    if (text.empty()) {
        return 1.0;
    }
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed != text.size()) {
            throw std::invalid_argument("");
        }
        return value;
    } catch (...) {
        throw std::invalid_argument("Invalid coefficient: '" + text + "'");
    }
}

int parseExponent(const std::string& text) {
    if (text.empty()) {
        return 1;
    }
    if (text.front() == '^') {
        const std::string digits = text.substr(1);
        const bool allDigits = !digits.empty() &&
            std::all_of(digits.begin(), digits.end(),
                        [](unsigned char c) { return std::isdigit(c) != 0; });
        if (!allDigits) {
            throw std::invalid_argument("Invalid exponent: '" + text + "'");
        }
        return std::stoi(digits);
    }

    int exponent = 0;
    if (!fromSuperscript(text, exponent)) {
        throw std::invalid_argument("Invalid exponent: '" + text + "'");
    }
    return exponent;
}

void parseSingleTerm(const std::string& rawTerm, Polynomial& polynomial) {
    std::string term = rawTerm;

    double sign = 1.0;
    if (!term.empty() && (term.front() == '+' || term.front() == '-')) {
        sign = (term.front() == '-') ? -1.0 : 1.0;
        term.erase(term.begin());
    }

    term.erase(std::remove(term.begin(), term.end(), '*'), term.end());

    if (term.empty()) {
        throw std::invalid_argument("Empty term in expression");
    }

    const auto xPos = term.find_first_of("xX");
    if (xPos == std::string::npos) {
        const double magnitude = parseCoefficientMagnitude(term);
        polynomial.appendTermRaw(sign * magnitude, 0);
        return;
    }

    const std::string coefficientText = term.substr(0, xPos);
    const std::string exponentText = term.substr(xPos + 1);

    const double magnitude = parseCoefficientMagnitude(coefficientText);
    const int exponent = parseExponent(exponentText);
    polynomial.appendTermRaw(sign * magnitude, exponent);
}

} // namespace

Polynomial PolynomialParser::parse(const std::string& expression) {
    const std::string compact = stripWhitespace(expression);
    if (compact.empty()) {
        throw std::invalid_argument("Cannot parse an empty expression");
    }

    Polynomial polynomial;
    std::size_t termStart = 0;
    for (std::size_t i = 1; i <= compact.size(); ++i) {
        const bool atDelimiter = i == compact.size() || compact[i] == '+' || compact[i] == '-';
        if (atDelimiter) {
            parseSingleTerm(compact.substr(termStart, i - termStart), polynomial);
            termStart = i;
        }
    }
    return polynomial;
}

} // namespace polycalc
