add_executable(polycalc_tests
    main_test.cpp
    test_version.cpp
    test_polynomial.cpp
    test_parser.cpp
    test_services.cpp
)

target_link_libraries(polycalc_tests PRIVATE polycalc_core)

add_test(NAME polycalc_unit_tests COMMAND polycalc_tests)
