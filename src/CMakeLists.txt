# The frontend (assets/web/*.html/css/js) is baked into the executable at
# build time via cmake/EmbedWebAssets.cmake, so polycalc ships as the single
# .exe it always has - no separate folder of web files has to travel with
# it. Adding a new file under assets/web/ requires a fresh `cmake -S . -B
# build` (a plain rebuild won't pick it up - see the file(GLOB) caveat noted
# in EmbedWebAssets.cmake); editing an existing file's contents does not.
set(POLYCALC_WEB_ASSETS_DIR ${CMAKE_SOURCE_DIR}/assets/web)
set(POLYCALC_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
set(POLYCALC_WEB_ASSETS_HEADER ${POLYCALC_GENERATED_DIR}/polycalc/web/WebAssets.hpp)

file(GLOB POLYCALC_WEB_ASSET_SOURCES ${POLYCALC_WEB_ASSETS_DIR}/*)

add_custom_command(
    OUTPUT ${POLYCALC_WEB_ASSETS_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${POLYCALC_GENERATED_DIR}/polycalc/web
    COMMAND ${CMAKE_COMMAND}
            -DIN_DIR=${POLYCALC_WEB_ASSETS_DIR}
            -DOUT_FILE=${POLYCALC_WEB_ASSETS_HEADER}
            -P ${CMAKE_SOURCE_DIR}/cmake/EmbedWebAssets.cmake
    DEPENDS ${POLYCALC_WEB_ASSET_SOURCES} ${CMAKE_SOURCE_DIR}/cmake/EmbedWebAssets.cmake
    COMMENT "Embedding assets/web/* into WebAssets.hpp"
    VERBATIM
)

add_library(polycalc_core
    core/Formatting.cpp
    core/Polynomial.cpp
    core/PolynomialParser.cpp
    services/Timer.cpp
    services/PolynomialGenerator.cpp
    services/PolynomialStorage.cpp
    services/HistoryManager.cpp
    web/Utf8.cpp
    web/Json.cpp
    web/HttpServer.cpp
    web/FileDialog.cpp
    web/ApiServer.cpp
    Application.cpp
    ${POLYCALC_WEB_ASSETS_HEADER}
)

target_include_directories(polycalc_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${POLYCALC_GENERATED_DIR}
)

target_compile_features(polycalc_core PUBLIC cxx_std_17)
target_compile_definitions(polycalc_core PUBLIC UNICODE _UNICODE)
target_link_libraries(polycalc_core PUBLIC Ws2_32.lib Comdlg32.lib Shell32.lib)

# A normal console-subsystem executable: the console is now this app's
# status window (it prints the local URL it's serving on and opens your
# browser to it), replacing the always-on-top Win32 window the old native
# GUI used. Closing the console window - or Ctrl+C, or the in-page "Quit"
# button - all stop the local server the same way closing the old window
# used to end the process.
add_executable(polycalc
    main.cpp
)

target_link_libraries(polycalc PRIVATE polycalc_core)
