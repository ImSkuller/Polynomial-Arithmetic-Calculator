cmake_minimum_required(VERSION 3.20)
project(PolynomialArithmeticCalculator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(POLYCALC_BUILD_TESTS "Build the unit test suite" ON)

add_subdirectory(src)

if (POLYCALC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
