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

TEST_CASE("displayWidth counts codepoints, not bytes") {
    REQUIRE_EQ(displayWidth("x"), 1u);
    REQUIRE_EQ(displayWidth("x²"), 2u);
    REQUIRE_EQ(displayWidth("5x⁴ + 3x² - 7"), 13u);
    REQUIRE_EQ(displayWidth(""), 0u);
}
