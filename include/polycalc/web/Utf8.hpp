#pragma once

#include "polycalc/web/Win32.hpp"

#include <string>

namespace polycalc::web {

// Everything in this app's HTTP/JSON layer is UTF-8 (JSON is UTF-8 by
// definition, and Polynomial::toString() already emits UTF-8 superscript
// digits as a std::string). The only place that still needs UTF-16 is the
// native Win32 file-open/save dialog, which is a wide-only API - these two
// functions are the sole crossing point.
std::wstring Utf8ToWide(const std::string& utf8);
std::string WideToUtf8(const std::wstring& wide);

} // namespace polycalc::web
