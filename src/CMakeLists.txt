add_library(polycalc_core
    core/Formatting.cpp
    core/Polynomial.cpp
    core/PolynomialParser.cpp
    services/Timer.cpp
    services/PolynomialGenerator.cpp
    services/PolynomialStorage.cpp
)

target_include_directories(polycalc_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(polycalc_core PUBLIC cxx_std_17)

add_executable(polycalc
    main.cpp
)

target_link_libraries(polycalc PRIVATE polycalc_core)
