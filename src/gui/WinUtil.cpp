#include "polycalc/gui/WinUtil.hpp"

#include <vector>

namespace polycalc::gui {

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    const int required =
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(),
                        required);
    return wide;
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                              nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(),
                        required, nullptr, nullptr);
    return utf8;
}

ColumnLayout::ColumnLayout(int x, int y, int width) : x_(x), y_(y), width_(width) {}

RECT ColumnLayout::nextRow(int height) {
    RECT row{x_, y_, x_ + width_, y_ + height};
    y_ += height + metrics::kRowSpacing;
    return row;
}

RECT splitRect(const RECT& row, int index, int count, int gap) {
    const int totalGap = gap * (count - 1);
    const int segmentWidth = (row.right - row.left - totalGap) / count;
    const int left = row.left + index * (segmentWidth + gap);
    const int right = (index == count - 1) ? row.right : left + segmentWidth;
    return RECT{left, row.top, right, row.bottom};
}

HWND CreateGroupBox(HWND parent, HINSTANCE instance, const RECT& rect, const std::wstring& title,
                    int id) {
    return CreateWindowExW(0, L"BUTTON", title.c_str(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                            parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance,
                            nullptr);
}

HWND CreateLabel(HWND parent, HINSTANCE instance, const RECT& rect, const std::wstring& text,
                  int id) {
    return CreateWindowExW(0, L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, rect.left,
                            rect.top, rect.right - rect.left, rect.bottom - rect.top, parent,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
}

HWND CreateEditBox(HWND parent, HINSTANCE instance, const RECT& rect, int id, bool readOnly,
                   bool multiline) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (readOnly) {
        style |= ES_READONLY;
    }
    if (multiline) {
        style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN;
        style &= ~static_cast<DWORD>(ES_AUTOHSCROLL);
    }
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", style, rect.left, rect.top,
                            rect.right - rect.left, rect.bottom - rect.top, parent,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
}

HWND CreateButtonCtrl(HWND parent, HINSTANCE instance, const RECT& rect, const std::wstring& text,
                      int id) {
    return CreateWindowExW(0, L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                            parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance,
                            nullptr);
}

std::string GetControlTextUtf8(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) {
        return std::string();
    }
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    GetWindowTextW(control, wide.data(), length + 1);
    return WideToUtf8(wide);
}

void SetControlTextUtf8(HWND control, const std::string& utf8Text) {
    SetWindowTextW(control, Utf8ToWide(utf8Text).c_str());
}

} // namespace polycalc::gui
