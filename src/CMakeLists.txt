add_library(polycalc_core
    core/Formatting.cpp
    core/Polynomial.cpp
    core/PolynomialParser.cpp
    services/Timer.cpp
    services/PolynomialGenerator.cpp
    services/PolynomialStorage.cpp
    services/HistoryManager.cpp
    gui/WinUtil.cpp
    gui/MainWindow.cpp
    Application.cpp
)

target_include_directories(polycalc_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(polycalc_core PUBLIC cxx_std_17)
target_compile_definitions(polycalc_core PUBLIC UNICODE _UNICODE)
target_link_libraries(polycalc_core PUBLIC User32.lib Gdi32.lib Comdlg32.lib)

# WIN32 marks this a GUI-subsystem executable, so it launches as a window
# with no console attached (the previous CLI version relied on the console
# subsystem for stdin/stdout; the GUI drives everything through controls).
add_executable(polycalc WIN32
    main.cpp
)

target_link_libraries(polycalc PRIVATE polycalc_core)
