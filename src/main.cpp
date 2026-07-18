#include <exception>

#include "polycalc/gui/MainWindow.hpp"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    try {
        polycalc::gui::MainWindow window(instance);
        window.show(showCommand);
        return window.runMessageLoop();
    } catch (const std::exception& ex) {
        MessageBoxA(nullptr, ex.what(), "Polynomial Arithmetic Calculator - Fatal Error",
                    MB_OK | MB_ICONERROR);
        return 1;
    }
}
