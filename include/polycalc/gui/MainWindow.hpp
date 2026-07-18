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

    // Build & Edit P(x) panel.
    HWND expressionBox_ = nullptr;
    HWND exponentBox_ = nullptr;
    HWND coefficientBox_ = nullptr;
    HWND newExponentBox_ = nullptr;

    // Second polynomial Q(x) & arithmetic panel.
    HWND secondaryExpressionBox_ = nullptr;
    HWND secondaryLabel_ = nullptr;
    HWND arithmeticResultLabel_ = nullptr;
    Polynomial lastArithmeticResult_;
    bool hasArithmeticResult_ = false;

    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void createControls();
    void createBuildAndEditPanel(int x, int y, int width, int& bottomY);
    void createArithmeticPanel(int x, int y, int width, int& bottomY);
    void onCommand(int controlId);

    void refreshCurrentDisplay();
    void refreshSecondaryDisplay();
    void appendLog(const std::string& tag, const std::string& message);
    void logSuccess(const std::string& message);
    void logError(const std::string& message);
    void logInfo(const std::string& message);

    // Each wraps its Application call in try/catch and reports the outcome
    // to the activity log, so a malformed expression or out-of-range input
    // never does anything worse than print an [ERROR] line.
    void onSetCurrentFromExpression();
    void onInsertTerm();
    void onDeleteTerm();
    void onUpdateCoefficient();
    void onUpdateExponent();
    void onClearCurrent();
    void onSetSecondaryFromExpression();
    void onArithmetic(int operation);
    void onUseResultAsCurrent();

    void onShowHelp();
};

} // namespace polycalc::gui
