#include "TestFramework.hpp"
#include "polycalc/Version.hpp"

TEST_CASE("version string is non-empty") {
    REQUIRE(!polycalc::kVersion.empty());
}

TEST_CASE("application name is set") {
    REQUIRE(!polycalc::kApplicationName.empty());
}
