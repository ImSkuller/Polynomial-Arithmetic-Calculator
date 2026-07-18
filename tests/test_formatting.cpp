#include "TestFramework.hpp"
#include "polycalc/core/Formatting.hpp"

using namespace polycalc;

TEST_CASE("toSuperscript converts multi-digit exponents") {
    REQUIRE_EQ(toSuperscript(0), "⁰");
    REQUIRE_EQ(toSuperscript(2), "²");
    REQUIRE_EQ(toSuperscript(12), "¹²");
    REQUIRE_EQ(toSuperscript(340), "³⁴⁰");
}

TEST_CASE("fromSuperscript decodes what toSuperscript produces") {
    for (int exponent = 0; exponent <= 25; ++exponent) {
        int decoded = -1;
        REQUIRE(fromSuperscript(toSuperscript(exponent), decoded));
        REQUIRE_EQ(decoded, exponent);
    }
}

TEST_CASE("fromSuperscript rejects non-superscript text") {
    int decoded = 0;
    REQUIRE(!fromSuperscript("2", decoded));
    REQUIRE(!fromSuperscript("", decoded));
    REQUIRE(!fromSuperscript("x²", decoded));
}

TEST_CASE("formatCoefficient trims integral values and keeps fractions") {
    REQUIRE_EQ(formatCoefficient(5.0), "5");
    REQUIRE_EQ(formatCoefficient(-7.0), "-7");
    REQUIRE_EQ(formatCoefficient(2.5), "2.5");
}

TEST_CASE("formatCoefficient survives values beyond long long range") {
    // Regression: 1e300 used to overflow llround and print
    // -9223372036854775808; it must fall back to scientific notation.
    REQUIRE_EQ(formatCoefficient(1e300), "1e+300");
    REQUIRE_EQ(formatCoefficient(-1e300), "-1e+300");
}

TEST_CASE("fromSuperscript rejects exponents wider than int without throwing") {
    // Regression: std::stoi used to throw std::out_of_range through the
    // bool-returning API.
    std::string huge;
    for (int i = 0; i < 15; ++i) {
        huge += "⁹";
    }
    int decoded = 0;
    REQUIRE(!fromSuperscript(huge, decoded));
}

TEST_CASE("toSuperscript tolerates a negative value without crashing") {
    // Regression: '-' - '0' used to index outside the digit table (UB).
    REQUIRE_EQ(toSuperscript(-3), "⁻³");
}
