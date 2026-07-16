#pragma once

#include <string>
#include <string_view>

namespace polycalc {

namespace color {
inline constexpr std::string_view Reset = "\x1b[0m";
inline constexpr std::string_view Bold = "\x1b[1m";
inline constexpr std::string_view Dim = "\x1b[2m";
inline constexpr std::string_view Red = "\x1b[31m";
inline constexpr std::string_view Green = "\x1b[32m";
inline constexpr std::string_view Yellow = "\x1b[33m";
inline constexpr std::string_view Blue = "\x1b[34m";
inline constexpr std::string_view Magenta = "\x1b[35m";
inline constexpr std::string_view Cyan = "\x1b[36m";
inline constexpr std::string_view White = "\x1b[37m";
inline constexpr std::string_view Gray = "\x1b[90m";
inline constexpr std::string_view BoldCyan = "\x1b[1m\x1b[36m";
inline constexpr std::string_view BoldGreen = "\x1b[1m\x1b[32m";
inline constexpr std::string_view BoldRed = "\x1b[1m\x1b[31m";
inline constexpr std::string_view BoldYellow = "\x1b[1m\x1b[33m";
inline constexpr std::string_view BoldMagenta = "\x1b[1m\x1b[35m";
} // namespace color

// Detects and enables ANSI escape sequence support in the host terminal
// (required on legacy Windows consoles), so the rest of the UI layer can
// query once and fall back to plain text everywhere else if unsupported.
class Terminal {
public:
    static bool colorEnabled();
    static void clearScreen();

private:
    static bool detectAndEnable();
};

// Wraps text in the given color code when the terminal supports it,
// otherwise returns the text unchanged.
std::string colorize(std::string_view text, std::string_view colorCode);

} // namespace polycalc
