#pragma once

#include <string>

#include "polycalc/Application.hpp"
#include "polycalc/gui/WinUtil.hpp"

namespace polycalc::gui {

// Owns the main application window: builds every control, wires their
// WM_COMMAND handlers to the Application session layer, and keeps the
// current-polynomial display and activity log in sync with whatever the
// user just did. There is exactly one of these per process.
class MainWindow {
public:
    // Registers the window class (once per process) and creates the window.
    // Throws std::runtime_error if either step fails.
    explicit MainWindow(HINSTANCE instance);

    void show(int showCommand);

    // Standard Win32 message loop; returns the code main() should exit with.
    int runMessageLoop();

private:
    HINSTANCE instance_;
    HWND hwnd_ = nullptr;
    Application app_;

    // Read-only displays kept in sync with the session state.
    HWND currentPolyLabel_ = nullptr;
    HWND currentDegreeLabel_ = nullptr;
    HWND logBox_ = nullptr;

    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void createControls();
    void onCommand(int controlId);

    void refreshCurrentDisplay();
    void appendLog(const std::string& tag, const std::string& message);
    void logSuccess(const std::string& message);
    void logError(const std::string& message);
    void logInfo(const std::string& message);

    void onShowHelp();
};

} // namespace polycalc::gui
