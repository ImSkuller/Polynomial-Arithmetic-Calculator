#include "polycalc/gui/MainWindow.hpp"

#include <stdexcept>

#include "polycalc/Version.hpp"

namespace polycalc::gui {

namespace {

constexpr wchar_t kWindowClassName[] = L"PolycalcMainWindow";

// Overall window canvas. Fixed-size and non-resizable, sized to fit every
// panel without scrolling - see the README for why a GUI this size doesn't
// need a resizable layout engine.
constexpr int kClientWidth = 970;
constexpr int kClientHeight = 960;

// Control IDs, grouped by the panel they belong to. Extended as each panel
// is added; kept in one enum so no two controls can accidentally share an
// ID.
enum ControlId : int {
    kIdHelpButton = 1000,
    kIdLogBox,
};

} // namespace

MainWindow::MainWindow(HINSTANCE instance) : instance_(instance) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &MainWindow::WndProcThunk;
    windowClass.hInstance = instance_;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    if (RegisterClassExW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error("Failed to register the main window class");
    }

    RECT windowRect{0, 0, kClientWidth, kClientHeight};
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&windowRect, style, FALSE);

    const std::wstring title =
        Utf8ToWide(std::string(kApplicationName) + " v" + std::string(kVersion));
    hwnd_ = CreateWindowExW(0, kWindowClassName, title.c_str(), style, CW_USEDEFAULT,
                             CW_USEDEFAULT, windowRect.right - windowRect.left,
                             windowRect.bottom - windowRect.top, nullptr, nullptr, instance_,
                             this);
    if (hwnd_ == nullptr) {
        throw std::runtime_error("Failed to create the main window");
    }
}

void MainWindow::show(int showCommand) {
    ShowWindow(hwnd_, showCommand);
    UpdateWindow(hwnd_);
}

int MainWindow::runMessageLoop() {
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK MainWindow::WndProcThunk(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* self = nullptr;
    if (message == WM_NCCREATE) {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<MainWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self != nullptr) {
        return self->handleMessage(message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            createControls();
            refreshCurrentDisplay();
            logInfo("Welcome! Build a polynomial below, or click Help for a full walkthrough.");
            return 0;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                onCommand(LOWORD(wParam));
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

void MainWindow::createControls() {
    using namespace metrics;

    RECT bannerRect{kMargin, kMargin, kClientWidth - kMargin - 110, kMargin + 24};
    currentPolyLabel_ =
        CreateLabel(hwnd_, instance_, bannerRect,
                    Utf8ToWide(std::string(kApplicationName) + " - build, edit, and compute "
                                                                "with polynomials"));

    RECT helpButtonRect{kClientWidth - kMargin - 100, kMargin, kClientWidth - kMargin, kMargin + 26};
    CreateButtonCtrl(hwnd_, instance_, helpButtonRect, L"Help / How it works", kIdHelpButton);

    RECT currentRect{kMargin, kMargin + 34, kClientWidth - kMargin, kMargin + 34 + kRowHeight};
    currentDegreeLabel_ = CreateLabel(hwnd_, instance_, currentRect, L"P(x) = 0");

    RECT logRect{kMargin, kClientHeight - kMargin - 160, kClientWidth - kMargin,
                 kClientHeight - kMargin};
    logBox_ = CreateEditBox(hwnd_, instance_, logRect, kIdLogBox, /*readOnly=*/true,
                            /*multiline=*/true);
}

void MainWindow::onCommand(int controlId) {
    switch (controlId) {
        case kIdHelpButton:
            onShowHelp();
            break;
        default:
            break;
    }
}

void MainWindow::refreshCurrentDisplay() {
    const std::string degreeText = app_.degreeText();
    SetControlTextUtf8(currentDegreeLabel_, "P(x) = " + app_.currentText() + "      (degree " +
                                                degreeText + ", " +
                                                std::to_string(app_.termCount()) + " term(s))");
}

void MainWindow::appendLog(const std::string& tag, const std::string& message) {
    const std::string line = "[" + tag + "] " + message + "\r\n";
    const int existingLength = GetWindowTextLengthW(logBox_);
    SendMessageW(logBox_, EM_SETSEL, existingLength, existingLength);
    SendMessageW(logBox_, EM_REPLACESEL, FALSE,
                reinterpret_cast<LPARAM>(Utf8ToWide(line).c_str()));
}

void MainWindow::logSuccess(const std::string& message) { appendLog("OK", message); }
void MainWindow::logError(const std::string& message) { appendLog("ERROR", message); }
void MainWindow::logInfo(const std::string& message) { appendLog("INFO", message); }

void MainWindow::onShowHelp() {
    MessageBoxW(hwnd_, L"Help window coming soon.", L"Help / How it works", MB_OK | MB_ICONINFORMATION);
}

} // namespace polycalc::gui
