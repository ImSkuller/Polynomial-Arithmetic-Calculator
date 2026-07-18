#include "polycalc/services/PolynomialStorage.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace polycalc {

namespace {
constexpr const char* kFileHeader = "POLYCALC";
constexpr int kFileVersion = 1;
} // namespace

void PolynomialStorage::save(const Polynomial& polynomial, const std::string& filePath) {
    std::ofstream file(filePath, std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Could not open file for writing: " + filePath);
    }

    const auto terms = polynomial.terms();
    // max_digits10 guarantees the double survives the text round trip
    // exactly; the default stream precision (6 significant digits) silently
    // corrupted coefficients on load.
    file << std::setprecision(std::numeric_limits<double>::max_digits10);
    file << kFileHeader << ' ' << kFileVersion << '\n';
    file << terms.size() << '\n';
    for (const auto& [coefficient, exponent] : terms) {
        file << coefficient << ' ' << exponent << '\n';
    }

    if (!file) {
        throw std::runtime_error("Failed while writing to file: " + filePath);
    }
}

Polynomial PolynomialStorage::load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Could not open file for reading: " + filePath);
    }

    std::string header;
    int version = 0;
    file >> header >> version;
    if (!file || header != kFileHeader) {
        throw std::invalid_argument("Not a valid polynomial file: " + filePath);
    }
    if (version != kFileVersion) {
        throw std::invalid_argument("Unsupported polynomial file version in: " + filePath);
    }

    std::size_t termCount = 0;
    file >> termCount;
    if (!file) {
        throw std::invalid_argument("Malformed polynomial file: " + filePath);
    }

    Polynomial polynomial;
    for (std::size_t i = 0; i < termCount; ++i) {
        double coefficient = 0.0;
        int exponent = 0;
        file >> coefficient >> exponent;
        if (!file) {
            throw std::invalid_argument("Malformed polynomial file: " + filePath);
        }
        polynomial.appendTermRaw(coefficient, exponent);
    }
    return polynomial;
}

} // namespace polycalc
