#include <iostream>

#include "polycalc/Version.hpp"

int main() {
    std::cout << polycalc::kApplicationName << " v" << polycalc::kVersion << '\n';
    std::cout << "Build scaffold OK.\n";
    return 0;
}
