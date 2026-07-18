#pragma once

#include "polycalc/gui/WinUtil.hpp"

namespace polycalc::gui {

// Opens the Help / How it Works window: a scrollable, read-only walkthrough
// of every panel in the main window, what each control does, and the
// algorithm and time complexity behind each operation. Non-modal (both
// windows share the same message loop on this single-threaded app), and
// safe to call more than once - reopening just brings a fresh window to
// the front.
void ShowHelpWindow(HWND owner, HINSTANCE instance);

} // namespace polycalc::gui
