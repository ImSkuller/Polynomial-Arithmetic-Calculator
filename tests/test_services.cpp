#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <thread>

#include "TestFramework.hpp"
#include "polycalc/core/Polynomial.hpp"
#include "polycalc/services/PolynomialGenerator.hpp"
#include "polycalc/services/PolynomialStorage.hpp"
#include "polycalc/services/Timer.hpp"

using polycalc::Polynomial;
using polycalc::PolynomialGenerator;
using polycalc::PolynomialStorage;
using polycalc::Timer;

TEST_CASE("Timer measures a non-negative, plausible duration") {
    Timer timer;
    timer.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    timer.stop();
    REQUIRE(timer.elapsedMilliseconds() >= 4.0);
}

TEST_CASE("Timer::measureMilliseconds times a callable") {
    const double elapsed = Timer::measureMilliseconds([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    });
    REQUIRE(elapsed >= 1.0);
}

TEST_CASE("PolynomialGenerator respects the requested exponent bounds") {
    // Use more exponent buckets than terms so same-exponent draws (which
    // insertTerm legitimately merges, summing coefficients) are rare enough
    // that this stays a meaningful check on the exponent range.
    PolynomialGenerator::Options options;
    options.termCount = 4;
    options.maxExponent = 20;
    options.minCoefficient = -3.0;
    options.maxCoefficient = 3.0;

    Polynomial p = PolynomialGenerator::generate(options);
    REQUIRE(p.termCount() <= 4u);
    REQUIRE(p.degree() <= 20);
    for (const auto& [coefficient, exponent] : p.terms()) {
        (void)coefficient;
        REQUIRE(exponent >= 0 && exponent <= 20);
    }
}

TEST_CASE("PolynomialGenerator rejects invalid options") {
    PolynomialGenerator::Options options;
    options.termCount = 0;
    bool threw = false;
    try {
        PolynomialGenerator::generate(options);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("PolynomialStorage round-trips a polynomial through a file") {
    Polynomial original;
    original.insertTerm(5, 4);
    original.insertTerm(3, 2);
    original.insertTerm(-7, 0);

    const auto path = std::filesystem::temp_directory_path() / "polycalc_storage_test.poly";
    PolynomialStorage::save(original, path.string());
    Polynomial loaded = PolynomialStorage::load(path.string());
    std::filesystem::remove(path);

    REQUIRE(original == loaded);
}

TEST_CASE("PolynomialStorage load rejects a missing file") {
    bool threw = false;
    try {
        PolynomialStorage::load((std::filesystem::temp_directory_path() /
                                  "polycalc_does_not_exist.poly").string());
    } catch (const std::runtime_error&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("PolynomialStorage load rejects a malformed file") {
    const auto path = std::filesystem::temp_directory_path() / "polycalc_malformed.poly";
    {
        std::ofstream file(path);
        file << "NOT_A_POLYCALC_FILE\n";
    }
    bool threw = false;
    try {
        PolynomialStorage::load(path.string());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    std::filesystem::remove(path);
    REQUIRE(threw);
}
