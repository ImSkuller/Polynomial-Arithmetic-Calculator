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

// Where the two side-by-side panel columns begin, and how wide each is.
// The current-polynomial banner occupies the space above this.
constexpr int kPanelTop = 92;
constexpr int kColumnWidth = 460;
constexpr int kLeftColumnX = metrics::kMargin;
constexpr int kRightColumnX = kLeftColumnX + kColumnWidth + metrics::kGroupGap;

// Control IDs, grouped by the panel they belong to. Extended as each panel
// is added; kept in one enum so no two controls can accidentally share an
// ID.
enum ControlId : int {
    kIdHelpButton = 1000,
    kIdLogBox,

    kIdSetExpression,
    kIdInsertTerm,
    kIdDeleteTerm,
    kIdUpdateCoefficient,
    kIdUpdateExponent,
    kIdClearCurrent,

    kIdSetSecondary,
    kIdAdd,
    kIdSubtract,
    kIdMultiply,
    kIdUseResult,
};

// Height of a group box's title strip plus its bottom padding, given how
// many content rows it holds.
int groupBoxHeight(int rowCount) {
    return metrics::kGroupTitleGap + rowCount * (metrics::kRowHeight + metrics::kRowSpacing) + 8;
}

// Reads a control's text as a double, throwing std::invalid_argument (with
// the field name and offending text in the message) if it isn't one -
// mirrors the validation the old console prompts used to do.
double parseDoubleField(HWND control, const std::string& fieldName) {
    const std::string text = GetControlTextUtf8(control);
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed != text.size()) {
            throw std::invalid_argument("");
        }
        return value;
    } catch (...) {
        throw std::invalid_argument("Invalid " + fieldName + ": '" + text + "'");
    }
}

int parseIntField(HWND control, const std::string& fieldName) {
    const std::string text = GetControlTextUtf8(control);
    try {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        if (consumed != text.size()) {
            throw std::invalid_argument("");
        }
        return value;
    } catch (...) {
        throw std::invalid_argument("Invalid " + fieldName + ": '" + text + "'");
    }
}

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
            refreshSecondaryDisplay();
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

    int leftY = kPanelTop;
    createBuildAndEditPanel(kLeftColumnX, leftY, kColumnWidth, leftY);
    leftY += metrics::kGroupGap;

    int rightY = kPanelTop;
    createArithmeticPanel(kRightColumnX, rightY, kColumnWidth, rightY);
    rightY += metrics::kGroupGap;
}

void MainWindow::createBuildAndEditPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 8;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Build && Edit P(x)", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    RECT row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"Expression:");
    expressionBox_ = CreateEditBox(
        hwnd_, instance_, RECT{row.left + kLabelWidth, row.top, row.right, row.bottom}, -1);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, row, L"Set P(x) from expression (e.g. 3x^2 + 4x - 8)",
                      kIdSetExpression);

    row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"Exponent:");
    exponentBox_ = CreateEditBox(
        hwnd_, instance_, RECT{row.left + kLabelWidth, row.top, row.right, row.bottom}, -1);

    row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"Coefficient:");
    coefficientBox_ = CreateEditBox(
        hwnd_, instance_, RECT{row.left + kLabelWidth, row.top, row.right, row.bottom}, -1);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 0, 2), L"Insert Term", kIdInsertTerm);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 1, 2), L"Delete Term (by exponent)",
                      kIdDeleteTerm);

    row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"New exponent:");
    newExponentBox_ = CreateEditBox(
        hwnd_, instance_, RECT{row.left + kLabelWidth, row.top, row.right, row.bottom}, -1);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 0, 2), L"Update Coefficient",
                      kIdUpdateCoefficient);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 1, 2), L"Update Exponent",
                      kIdUpdateExponent);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, row, L"Clear P(x)", kIdClearCurrent);

    bottomY = groupRect.bottom;
}

void MainWindow::createArithmeticPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 6;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Second Polynomial Q(x) && Arithmetic", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    RECT row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"Expression Q:");
    secondaryExpressionBox_ = CreateEditBox(
        hwnd_, instance_, RECT{row.left + kLabelWidth, row.top, row.right, row.bottom}, -1);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, row, L"Set Q(x) from expression", kIdSetSecondary);

    row = col.nextRow();
    secondaryLabel_ = CreateLabel(hwnd_, instance_, row, L"Q(x) = 0");

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 0, 3), L"P + Q", kIdAdd);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 1, 3), L"P - Q", kIdSubtract);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 2, 3), L"P * Q", kIdMultiply);

    row = col.nextRow();
    arithmeticResultLabel_ = CreateLabel(hwnd_, instance_, row, L"Result: (choose an operation above)");

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, row, L"Use Result as P(x)", kIdUseResult);

    bottomY = groupRect.bottom;
}

void MainWindow::onCommand(int controlId) {
    switch (controlId) {
        case kIdHelpButton:
            onShowHelp();
            break;
        case kIdSetExpression:
            onSetCurrentFromExpression();
            break;
        case kIdInsertTerm:
            onInsertTerm();
            break;
        case kIdDeleteTerm:
            onDeleteTerm();
            break;
        case kIdUpdateCoefficient:
            onUpdateCoefficient();
            break;
        case kIdUpdateExponent:
            onUpdateExponent();
            break;
        case kIdClearCurrent:
            onClearCurrent();
            break;
        case kIdSetSecondary:
            onSetSecondaryFromExpression();
            break;
        case kIdAdd:
            onArithmetic(0);
            break;
        case kIdSubtract:
            onArithmetic(1);
            break;
        case kIdMultiply:
            onArithmetic(2);
            break;
        case kIdUseResult:
            onUseResultAsCurrent();
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

void MainWindow::refreshSecondaryDisplay() {
    SetControlTextUtf8(secondaryLabel_, "Q(x) = " + app_.secondaryText());
}

void MainWindow::onSetCurrentFromExpression() {
    try {
        app_.setCurrentFromExpression(GetControlTextUtf8(expressionBox_));
        refreshCurrentDisplay();
        logSuccess("P(x) set to: " + app_.currentText());
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onInsertTerm() {
    try {
        const double coefficient = parseDoubleField(coefficientBox_, "coefficient");
        const int exponent = parseIntField(exponentBox_, "exponent");
        const bool changed = app_.insertTerm(coefficient, exponent);
        refreshCurrentDisplay();
        logSuccess(changed ? "Inserted. P(x) = " + app_.currentText()
                            : "A zero coefficient was ignored.");
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onDeleteTerm() {
    try {
        const int exponent = parseIntField(exponentBox_, "exponent");
        const bool changed = app_.deleteTerm(exponent);
        refreshCurrentDisplay();
        logSuccess(changed ? "Removed. P(x) = " + app_.currentText()
                            : "No term with that exponent exists.");
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onUpdateCoefficient() {
    try {
        const int exponent = parseIntField(exponentBox_, "exponent");
        const double coefficient = parseDoubleField(coefficientBox_, "coefficient");
        const bool changed = app_.updateCoefficient(exponent, coefficient);
        refreshCurrentDisplay();
        logSuccess(changed ? "Updated. P(x) = " + app_.currentText()
                            : "No term with that exponent exists.");
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onUpdateExponent() {
    try {
        const int oldExponent = parseIntField(exponentBox_, "exponent");
        const int newExponent = parseIntField(newExponentBox_, "new exponent");
        const bool changed = app_.updateExponent(oldExponent, newExponent);
        refreshCurrentDisplay();
        logSuccess(changed ? "Updated. P(x) = " + app_.currentText()
                            : "No term with that exponent exists.");
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onClearCurrent() {
    app_.clearCurrent();
    refreshCurrentDisplay();
    logSuccess("P(x) cleared.");
}

void MainWindow::onSetSecondaryFromExpression() {
    try {
        app_.setSecondaryFromExpression(GetControlTextUtf8(secondaryExpressionBox_));
        refreshSecondaryDisplay();
        logSuccess("Q(x) set to: " + app_.secondaryText());
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onArithmetic(int operation) {
    try {
        Polynomial result;
        std::string label;
        switch (operation) {
            case 0:
                result = app_.add();
                label = "P + Q";
                break;
            case 1:
                result = app_.subtract();
                label = "P - Q";
                break;
            default:
                result = app_.multiply();
                label = "P * Q";
                break;
        }
        const std::string resultText = result.toString();
        lastArithmeticResult_ = std::move(result);
        hasArithmeticResult_ = true;
        SetControlTextUtf8(arithmeticResultLabel_, "Result: " + label + " = " + resultText);
        logSuccess(label + " = " + resultText);
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onUseResultAsCurrent() {
    if (!hasArithmeticResult_) {
        logError("Compute P + Q, P - Q, or P * Q first.");
        return;
    }
    app_.adoptAsCurrent(lastArithmeticResult_);
    refreshCurrentDisplay();
    logSuccess("P(x) is now the arithmetic result: " + app_.currentText());
}

void MainWindow::onShowHelp() {
    MessageBoxW(hwnd_, L"Help window coming soon.", L"Help / How it works", MB_OK | MB_ICONINFORMATION);
}

} // namespace polycalc::gui
