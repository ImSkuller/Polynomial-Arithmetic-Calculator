#include "TestFramework.hpp"

int main() {
    std::cout << "Running Polynomial Arithmetic Calculator test suite\n\n";
    return polycalc::test::TestRegistry::instance().runAll();
}
