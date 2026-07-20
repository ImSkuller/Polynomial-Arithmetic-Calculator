#include "polycalc/web/FileDialog.hpp"

#include "polycalc/web/Win32.hpp"
#include <commdlg.h>

#include "polycalc/web/Utf8.hpp"

namespace polycalc::web {

namespace {

std::optional<std::string> runDialog(const std::string& initialPathUtf8, bool forSave) {
    wchar_t pathBuffer[MAX_PATH] = L"";
    const std::wstring initial = Utf8ToWide(initialPathUtf8);
    if (initial.size() < MAX_PATH) {
        wcsncpy_s(pathBuffer, initial.c_str(), initial.size());
    }

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = nullptr;
    dialog.lpstrFilter = L"Polynomial files (*.poly)\0*.poly\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = pathBuffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"poly";
    dialog.Flags = forSave ? (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY)
                            : (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY);

    const BOOL picked = forSave ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    if (!picked) {
        return std::nullopt;
    }
    return WideToUtf8(pathBuffer);
}

} // namespace

std::optional<std::string> FileDialog::openPolynomialFile(const std::string& initialPathUtf8) {
    return runDialog(initialPathUtf8, /*forSave=*/false);
}

std::optional<std::string> FileDialog::savePolynomialFile(const std::string& initialPathUtf8) {
    return runDialog(initialPathUtf8, /*forSave=*/true);
}

} // namespace polycalc::web
