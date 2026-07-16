#pragma once

#include <string>
#include <vector>

namespace polycalc {

// Thrown when an input prompt hits end-of-stream (e.g. stdin was redirected
// from an exhausted file, or the user sent Ctrl-D/Ctrl-Z). Deliberately does
// not derive from std::exception, so the per-menu `catch (const
// std::exception&)` blocks that report ordinary input/parse errors do not
// intercept it - it propagates straight up to main(), which exits cleanly
// instead of looping forever on reads that can never succeed again.
struct InputClosedException {};

// Renders bordered, colored console output (banners, boxes, tables, menus)
// and provides validated input prompts. All layout math is Unicode-aware
// (see Formatting::displayWidth) so borders stay aligned around text
// containing superscript digits. Falls back to plain, uncolored text when
// the host terminal does not support ANSI escape sequences.
class ConsoleUI {
public:
    static void printBanner(const std::string& title, const std::string& subtitle);
    static void printBox(const std::vector<std::string>& lines, const std::string& title = "");
    static void printTable(const std::vector<std::string>& headers,
                            const std::vector<std::vector<std::string>>& rows,
                            const std::string& title = "");
    static void printMenu(const std::string& title, const std::vector<std::string>& options);

    static void printSuccess(const std::string& message);
    static void printError(const std::string& message);
    static void printWarning(const std::string& message);
    static void printInfo(const std::string& message);

    static int readMenuChoice(int minValue, int maxValue);
    static std::string readLine(const std::string& prompt);
    static double readDouble(const std::string& prompt);
    static int readInt(const std::string& prompt);

    static void pause(const std::string& message = "Press Enter to continue...");
};

} // namespace polycalc
