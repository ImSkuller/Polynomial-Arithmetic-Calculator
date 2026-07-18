#include "polycalc/gui/MainWindow.hpp"

#include <commdlg.h>

#include <cwchar>
#include <stdexcept>

#include "polycalc/Version.hpp"
#include "polycalc/core/Formatting.hpp"
#include "polycalc/gui/HelpWindow.hpp"

namespace polycalc::gui {

namespace {

constexpr wchar_t kWindowClassName[] = L"PolycalcMainWindow";

// Overall window canvas. Fixed-size and non-resizable, sized to fit every
// panel without scrolling - see the README for why a GUI this size doesn't
// need a resizable layout engine.
constexpr int kClientWidth = 970;
constexpr int kClientHeight = 900;

// A handful of rows pack a short label, an edit box, and sometimes a button
// on one line to keep panels compact; this is the narrow label width used
// only on those rows (the full kLabelWidth would leave no room for the
// edit box next to it).
constexpr int kMiniLabelWidth = 46;

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

    kIdEvaluate,
    kIdSort,
    kIdMerge,
    kIdSimplify,

    kIdUndo,
    kIdRedo,

    kIdBrowseFile,
    kIdSave,
    kIdLoad,

    kIdGenerateRandom,
    kIdRefreshStatistics,
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
    createHistoryPanel(kLeftColumnX, leftY, kColumnWidth, leftY);
    leftY += metrics::kGroupGap;
    createSaveLoadPanel(kLeftColumnX, leftY, kColumnWidth, leftY);
    leftY += metrics::kGroupGap;

    int rightY = kPanelTop;
    createArithmeticPanel(kRightColumnX, rightY, kColumnWidth, rightY);
    rightY += metrics::kGroupGap;
    createAnalyzePanel(kRightColumnX, rightY, kColumnWidth, rightY);
    rightY += metrics::kGroupGap;
    createRandomGeneratorPanel(kRightColumnX, rightY, kColumnWidth, rightY);
    rightY += metrics::kGroupGap;
    createStatisticsPanel(kRightColumnX, rightY, kColumnWidth, rightY);
    rightY += metrics::kGroupGap;
}

void MainWindow::createBuildAndEditPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 7;
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

    // Exponent and coefficient share one row: each half gets its own short
    // label so the pair reads as "Exponent: [_] Coefficient: [_]".
    row = col.nextRow();
    RECT half = splitRect(row, 0, 2, 16);
    CreateLabel(hwnd_, instance_, RECT{half.left, half.top, half.left + kMiniLabelWidth, half.bottom},
                L"Exp:");
    exponentBox_ = CreateEditBox(
        hwnd_, instance_, RECT{half.left + kMiniLabelWidth, half.top, half.right, half.bottom}, -1);
    half = splitRect(row, 1, 2, 16);
    CreateLabel(hwnd_, instance_, RECT{half.left, half.top, half.left + kMiniLabelWidth, half.bottom},
                L"Coef:");
    coefficientBox_ = CreateEditBox(
        hwnd_, instance_, RECT{half.left + kMiniLabelWidth, half.top, half.right, half.bottom}, -1);

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
    constexpr int kRows = 5;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Second Polynomial Q(x) && Arithmetic", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    // Expression, edit, and "Set" button share one row to save vertical
    // space - the row is wide enough for a fixed label, a flexible edit
    // box, and a fixed-width button on the right.
    constexpr int kSetButtonWidth = 96;
    RECT row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"Expression Q:");
    secondaryExpressionBox_ = CreateEditBox(
        hwnd_, instance_,
        RECT{row.left + kLabelWidth, row.top, row.right - kSetButtonWidth - 6, row.bottom}, -1);
    CreateButtonCtrl(hwnd_, instance_,
                      RECT{row.right - kSetButtonWidth, row.top, row.right, row.bottom},
                      L"Set Q(x)", kIdSetSecondary);

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

void MainWindow::createAnalyzePanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 4;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Analyze", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    constexpr int kEvaluateButtonWidth = 110;
    RECT row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kMiniLabelWidth, row.bottom},
                L"x =");
    xValueBox_ = CreateEditBox(
        hwnd_, instance_,
        RECT{row.left + kMiniLabelWidth, row.top, row.right - kEvaluateButtonWidth - 6, row.bottom},
        -1);
    CreateButtonCtrl(hwnd_, instance_,
                      RECT{row.right - kEvaluateButtonWidth, row.top, row.right, row.bottom},
                      L"Evaluate P(x)", kIdEvaluate);

    row = col.nextRow();
    evaluateResultLabel_ = CreateLabel(hwnd_, instance_, row, L"Result: (evaluate above)");

    // Sort, merge, and simplify are alternative "put the polynomial in
    // canonical form" operations - grouping them on one row reads as a set
    // of related choices rather than three unrelated steps.
    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 0, 3), L"Sort", kIdSort);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 1, 3), L"Merge Like Terms", kIdMerge);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 2, 3), L"Simplify", kIdSimplify);

    row = col.nextRow();
    timingLabel_ = CreateLabel(hwnd_, instance_, row, L"Last operation took: -");

    bottomY = groupRect.bottom;
}

void MainWindow::createHistoryPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 2;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"History (Undo / Redo)", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    RECT row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 0, 2), L"Undo", kIdUndo);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 1, 2), L"Redo", kIdRedo);

    row = col.nextRow();
    historyLabel_ = CreateLabel(hwnd_, instance_, row, L"Undo available: 0   Redo available: 0");

    bottomY = groupRect.bottom;
}

void MainWindow::createSaveLoadPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 2;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Save && Load", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    RECT row = col.nextRow();
    CreateLabel(hwnd_, instance_, RECT{row.left, row.top, row.left + kLabelWidth, row.bottom},
                L"File path:");
    filePathBox_ = CreateEditBox(
        hwnd_, instance_, RECT{row.left + kLabelWidth, row.top, row.right, row.bottom}, -1);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 0, 3), L"Browse...", kIdBrowseFile);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 1, 3), L"Save", kIdSave);
    CreateButtonCtrl(hwnd_, instance_, splitRect(row, 2, 3), L"Load", kIdLoad);

    bottomY = groupRect.bottom;
}

void MainWindow::createRandomGeneratorPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kRows = 3;
    const RECT groupRect{x, y, x + width, y + groupBoxHeight(kRows)};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Random Generator", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    RECT row = col.nextRow();
    RECT half = splitRect(row, 0, 2, 16);
    CreateLabel(hwnd_, instance_, RECT{half.left, half.top, half.left + kMiniLabelWidth, half.bottom},
                L"Terms:");
    genTermCountBox_ = CreateEditBox(
        hwnd_, instance_, RECT{half.left + kMiniLabelWidth, half.top, half.right, half.bottom}, -1);
    half = splitRect(row, 1, 2, 16);
    CreateLabel(hwnd_, instance_, RECT{half.left, half.top, half.left + kMiniLabelWidth, half.bottom},
                L"Max exp:");
    genMaxExponentBox_ = CreateEditBox(
        hwnd_, instance_, RECT{half.left + kMiniLabelWidth, half.top, half.right, half.bottom}, -1);

    row = col.nextRow();
    half = splitRect(row, 0, 2, 16);
    CreateLabel(hwnd_, instance_, RECT{half.left, half.top, half.left + kMiniLabelWidth, half.bottom},
                L"Min c:");
    genMinCoefficientBox_ = CreateEditBox(
        hwnd_, instance_, RECT{half.left + kMiniLabelWidth, half.top, half.right, half.bottom}, -1);
    half = splitRect(row, 1, 2, 16);
    CreateLabel(hwnd_, instance_, RECT{half.left, half.top, half.left + kMiniLabelWidth, half.bottom},
                L"Max c:");
    genMaxCoefficientBox_ = CreateEditBox(
        hwnd_, instance_, RECT{half.left + kMiniLabelWidth, half.top, half.right, half.bottom}, -1);

    row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, row, L"Generate Random Polynomial", kIdGenerateRandom);

    bottomY = groupRect.bottom;
}

void MainWindow::createStatisticsPanel(int x, int y, int width, int& bottomY) {
    using namespace metrics;
    constexpr int kStatsBoxHeight = 80;
    const RECT groupRect{x, y,
                          x + width,
                          y + kGroupTitleGap + kRowHeight + kRowSpacing + kStatsBoxHeight + 8};
    CreateGroupBox(hwnd_, instance_, groupRect, L"Statistics && Performance", -1);

    ColumnLayout col(x + 10, y + kGroupTitleGap, width - 20);

    RECT row = col.nextRow();
    CreateButtonCtrl(hwnd_, instance_, row, L"Refresh Statistics", kIdRefreshStatistics);

    row = col.nextRow(kStatsBoxHeight);
    statsBox_ =
        CreateEditBox(hwnd_, instance_, row, -1, /*readOnly=*/true, /*multiline=*/true);

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
        case kIdEvaluate:
            onEvaluate();
            break;
        case kIdSort:
            onSort();
            break;
        case kIdMerge:
            onMerge();
            break;
        case kIdSimplify:
            onSimplify();
            break;
        case kIdUndo:
            onUndo();
            break;
        case kIdRedo:
            onRedo();
            break;
        case kIdBrowseFile:
            onBrowseFile();
            break;
        case kIdSave:
            onSave();
            break;
        case kIdLoad:
            onLoad();
            break;
        case kIdGenerateRandom:
            onGenerateRandom();
            break;
        case kIdRefreshStatistics:
            onRefreshStatistics();
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
    // Every mutation that changes current_ also changes undo/redo depth, so
    // the history readout is kept in lockstep here rather than duplicated
    // across every handler that calls refreshCurrentDisplay().
    refreshHistoryLabel();
}

void MainWindow::refreshHistoryLabel() {
    SetControlTextUtf8(historyLabel_, "Undo available: " + std::to_string(app_.undoDepth()) +
                                          "   Redo available: " + std::to_string(app_.redoDepth()));
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

void MainWindow::onEvaluate() {
    try {
        const double x = parseDoubleField(xValueBox_, "x");
        const double result = app_.evaluate(x);
        SetControlTextUtf8(evaluateResultLabel_,
                            "Result: P(" + formatCoefficient(x) + ") = " + formatCoefficient(result));
        logSuccess("P(" + formatCoefficient(x) + ") = " + formatCoefficient(result));
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onSort() {
    const double elapsed = app_.sortByExponent();
    refreshCurrentDisplay();
    SetControlTextUtf8(timingLabel_, "Last operation took: " + formatCoefficient(elapsed) + " ms");
    logSuccess("Sorted in " + formatCoefficient(elapsed) + " ms. P(x) = " + app_.currentText());
}

void MainWindow::onMerge() {
    const double elapsed = app_.mergeLikeTerms();
    refreshCurrentDisplay();
    SetControlTextUtf8(timingLabel_, "Last operation took: " + formatCoefficient(elapsed) + " ms");
    logSuccess("Merged in " + formatCoefficient(elapsed) + " ms. P(x) = " + app_.currentText());
}

void MainWindow::onSimplify() {
    const double elapsed = app_.simplify();
    refreshCurrentDisplay();
    SetControlTextUtf8(timingLabel_, "Last operation took: " + formatCoefficient(elapsed) + " ms");
    logSuccess("Simplified in " + formatCoefficient(elapsed) + " ms. P(x) = " + app_.currentText());
}

void MainWindow::onUndo() {
    try {
        app_.undo();
        refreshCurrentDisplay();
        logSuccess("Undone. P(x) = " + app_.currentText());
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onRedo() {
    try {
        app_.redo();
        refreshCurrentDisplay();
        logSuccess("Redone. P(x) = " + app_.currentText());
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onSave() {
    try {
        const std::string path = GetControlTextUtf8(filePathBox_);
        if (path.empty()) {
            throw std::invalid_argument("Enter or browse to a file path first.");
        }
        app_.saveToFile(path);
        logSuccess("Saved to " + path);
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onLoad() {
    try {
        const std::string path = GetControlTextUtf8(filePathBox_);
        if (path.empty()) {
            throw std::invalid_argument("Enter or browse to a file path first.");
        }
        app_.loadFromFile(path);
        refreshCurrentDisplay();
        logSuccess("Loaded. P(x) = " + app_.currentText());
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onBrowseFile() {
    wchar_t pathBuffer[MAX_PATH] = L"";
    const std::wstring existing = Utf8ToWide(GetControlTextUtf8(filePathBox_));
    if (existing.size() < MAX_PATH) {
        wcsncpy_s(pathBuffer, existing.c_str(), existing.size());
    }

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = L"Polynomial files (*.poly)\0*.poly\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = pathBuffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    dialog.lpstrDefExt = L"poly";

    if (GetOpenFileNameW(&dialog) != 0) {
        SetControlTextUtf8(filePathBox_, WideToUtf8(pathBuffer));
    }
}

void MainWindow::onGenerateRandom() {
    try {
        PolynomialGenerator::Options options;
        options.termCount = parseIntField(genTermCountBox_, "term count");
        options.maxExponent = parseIntField(genMaxExponentBox_, "max exponent");
        options.minCoefficient = parseDoubleField(genMinCoefficientBox_, "min coefficient");
        options.maxCoefficient = parseDoubleField(genMaxCoefficientBox_, "max coefficient");

        app_.generateRandom(options);
        refreshCurrentDisplay();
        logSuccess("Generated. P(x) = " + app_.currentText());
    } catch (const std::exception& ex) {
        logError(ex.what());
    }
}

void MainWindow::onRefreshStatistics() {
    const Statistics stats = app_.statistics();
    const std::string degreeText = stats.degree < 0 ? "-" : std::to_string(stats.degree);
    const std::string table =
        "Term count:              " + std::to_string(stats.termCount) + "\r\n" +
        "Degree:                  " + degreeText + "\r\n" +
        "Estimated memory:        " + std::to_string(stats.memoryBytes) + " bytes\r\n" +
        "sortByExponent() time:   " + formatCoefficient(stats.sortMs) + " ms\r\n" +
        "mergeLikeTerms() time:   " + formatCoefficient(stats.mergeMs) + " ms\r\n" +
        "simplify() time:         " + formatCoefficient(stats.simplifyMs) + " ms\r\n" +
        "evaluate(2) time:        " + formatCoefficient(stats.evaluateMs) + " ms";
    SetControlTextUtf8(statsBox_, table);
    logInfo("Statistics refreshed.");
}

void MainWindow::onShowHelp() { ShowHelpWindow(hwnd_, instance_); }

} // namespace polycalc::gui
