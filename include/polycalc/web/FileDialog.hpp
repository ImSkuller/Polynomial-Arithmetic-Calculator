#pragma once

#include <optional>
#include <string>

namespace polycalc::web {

// Wraps the native Win32 common file dialogs (GetOpenFileNameW /
// GetSaveFileNameW). Even though the UI now runs in a browser, the process
// showing it is still a normal Windows program, so it can still pop the
// same native "Open"/"Save As" dialog the old Win32 GUI used - a browser
// <input type=file> can't do this (it can only upload file *contents*, and
// can't preselect an arbitrary server-side path), so this is a genuine
// capability the C++ backend keeps that a plain static web frontend
// couldn't offer on its own.
//
// Each blocks the calling HTTP worker thread until the user picks a file or
// cancels; that's fine here since this app serves one browser tab at a
// time and every other request is independent of file I/O.
class FileDialog {
public:
    // Returns std::nullopt if the user cancelled.
    static std::optional<std::string> openPolynomialFile(const std::string& initialPathUtf8);
    static std::optional<std::string> savePolynomialFile(const std::string& initialPathUtf8);
};

} // namespace polycalc::web
