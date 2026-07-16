#include "TestFramework.hpp"
#include "polycalc/console/Terminal.hpp"

TEST_CASE("Terminal::colorEnabled is stable across repeated calls") {
    const bool first = polycalc::Terminal::colorEnabled();
    const bool second = polycalc::Terminal::colorEnabled();
    REQUIRE_EQ(first, second);
}

TEST_CASE("colorize preserves the original text content") {
    const std::string text = "5x^2 - 3";
    const std::string result = polycalc::colorize(text, polycalc::color::Red);
    REQUIRE(result.find(text) != std::string::npos);
}

TEST_CASE("colorize returns plain text unchanged when color is disabled") {
    if (!polycalc::Terminal::colorEnabled()) {
        const std::string text = "plain";
        REQUIRE_EQ(polycalc::colorize(text, polycalc::color::Green), text);
    }
}
