#include "polycalc/console/Terminal.hpp"

#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace polycalc {

bool Terminal::colorEnabled() {
    static const bool enabled = detectAndEnable();
    return enabled;
}

bool Terminal::detectAndEnable() {
#if defined(_WIN32)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(handle, &mode)) {
        return false;
    }
    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        if (!SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            return false;
        }
    }
    return true;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

void Terminal::clearScreen() {
    if (colorEnabled()) {
        std::fputs("\x1b[2J\x1b[H", stdout);
    } else {
#if defined(_WIN32)
        std::system("cls");
#else
        std::system("clear");
#endif
    }
}

std::string colorize(std::string_view text, std::string_view colorCode) {
    if (!Terminal::colorEnabled()) {
        return std::string(text);
    }
    std::string result;
    result.reserve(text.size() + colorCode.size() + color::Reset.size());
    result += colorCode;
    result += text;
    result += color::Reset;
    return result;
}

} // namespace polycalc
