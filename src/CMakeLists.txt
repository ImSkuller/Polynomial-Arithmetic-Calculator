# Header-only for now; gains compiled sources as core/services modules land.
add_library(polycalc_core INTERFACE)

target_include_directories(polycalc_core
    INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(polycalc_core INTERFACE cxx_std_17)

add_executable(polycalc
    main.cpp
)

target_link_libraries(polycalc PRIVATE polycalc_core)
