#pragma once

// windows.h defines min/max macros that shadow std::min/std::max unless this
// is defined first; every GUI translation unit includes this header instead
// of <windows.h> directly so that guard never has to be repeated.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>

namespace polycalc::gui {

// Polynomial::toString() renders Unicode superscript digits (x², x⁴, ...)
// encoded as UTF-8. Win32 controls only render Unicode correctly through
// their wide (W) API, so every string that might contain a superscript has
// to cross this boundary before it reaches SetWindowTextW/DrawTextW.
std::wstring Utf8ToWide(const std::string& utf8);
std::string WideToUtf8(const std::wstring& wide);

// Shared pixel metrics so every panel lines up without each call site
// re-deriving the same constants.
namespace metrics {
inline constexpr int kMargin = 12;
inline constexpr int kRowHeight = 24;
inline constexpr int kRowSpacing = 6;
inline constexpr int kLabelWidth = 118;
inline constexpr int kGroupTitleGap = 22;
inline constexpr int kGroupGap = 12;
} // namespace metrics

// Lays out controls top-to-bottom inside a fixed-width column so panel-
// building code only ever has to ask for "the next row" instead of tracking
// x/y coordinates by hand.
class ColumnLayout {
public:
    ColumnLayout(int x, int y, int width);

    int x() const noexcept { return x_; }
    int width() const noexcept { return width_; }
    int cursorY() const noexcept { return y_; }

    // Reserves a row of the given height at the current cursor and advances
    // past it (plus the standard row spacing).
    RECT nextRow(int height = metrics::kRowHeight);

    // Advances the cursor without reserving a row rect, e.g. after a nested
    // layout (like a group box) has already advanced its own y position.
    void setCursorY(int y) noexcept { y_ = y; }

private:
    int x_;
    int y_;
    int width_;
};

// Splits a row rect into `count` equal, gap-separated segments, for rows
// that hold several buttons side by side.
RECT splitRect(const RECT& row, int index, int count, int gap = 8);

HWND CreateGroupBox(HWND parent, HINSTANCE instance, const RECT& rect, const std::wstring& title,
                    int id);
HWND CreateLabel(HWND parent, HINSTANCE instance, const RECT& rect, const std::wstring& text,
                  int id = -1);
HWND CreateEditBox(HWND parent, HINSTANCE instance, const RECT& rect, int id,
                   bool readOnly = false, bool multiline = false);
HWND CreateButtonCtrl(HWND parent, HINSTANCE instance, const RECT& rect, const std::wstring& text,
                      int id);

// Reads an edit control's text and converts it from UTF-16 to UTF-8.
std::string GetControlTextUtf8(HWND control);
// Sets an edit/static control's text from a UTF-8 string.
void SetControlTextUtf8(HWND control, const std::string& utf8Text);

} // namespace polycalc::gui
