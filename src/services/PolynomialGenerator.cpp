#include "polycalc/services/PolynomialGenerator.hpp"

#include <random>
#include <stdexcept>

namespace polycalc {

Polynomial PolynomialGenerator::generate(const Options& options) {
    if (options.termCount <= 0) {
        throw std::invalid_argument("termCount must be positive");
    }
    if (options.maxExponent < 0) {
        throw std::invalid_argument("maxExponent must be non-negative");
    }
    if (options.minCoefficient > options.maxCoefficient) {
        throw std::invalid_argument("minCoefficient must not exceed maxCoefficient");
    }

    static std::mt19937 engine{std::random_device{}()};
    std::uniform_int_distribution<int> exponentDistribution(0, options.maxExponent);
    std::uniform_real_distribution<double> coefficientDistribution(options.minCoefficient,
                                                                     options.maxCoefficient);

    Polynomial polynomial;
    for (int i = 0; i < options.termCount; ++i) {
        polynomial.insertTerm(coefficientDistribution(engine), exponentDistribution(engine));
    }
    return polynomial;
}

} // namespace polycalc
