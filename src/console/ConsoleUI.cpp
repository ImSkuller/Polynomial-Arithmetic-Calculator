#include "polycalc/console/ConsoleUI.hpp"

#include <algorithm>
#include <iostream>

#include "polycalc/console/Terminal.hpp"
#include "polycalc/core/Formatting.hpp"

namespace polycalc {

namespace {

namespace box {
constexpr std::string_view TopLeft = "╭";
constexpr std::string_view TopRight = "╮";
constexpr std::string_view BottomLeft = "╰";
constexpr std::string_view BottomRight = "╯";
constexpr std::string_view Horizontal = "─";
constexpr std::string_view Vertical = "│";
constexpr std::string_view TeeDown = "┬";
constexpr std::string_view TeeUp = "┴";
constexpr std::string_view TeeRight = "├";
constexpr std::string_view TeeLeft = "┤";
constexpr std::string_view Cross = "┼";
} // namespace box

std::string repeat(std::string_view unit, std::size_t count) {
    std::string result;
    result.reserve(unit.size() * count);
    for (std::size_t i = 0; i < count; ++i) {
        result += unit;
    }
    return result;
}

std::string centerText(const std::string& text, std::size_t width) {
    const std::size_t textWidth = displayWidth(text);
    if (textWidth >= width) {
        return text;
    }
    const std::size_t totalPad = width - textWidth;
    const std::size_t left = totalPad / 2;
    const std::size_t right = totalPad - left;
    return std::string(left, ' ') + text + std::string(right, ' ');
}

} // namespace

void ConsoleUI::printBanner(const std::string& title, const std::string& subtitle) {
    std::size_t width = displayWidth(title);
    if (!subtitle.empty()) {
        width = std::max(width, displayWidth(subtitle));
    }
    width += 4;

    const std::string border = repeat(box::Horizontal, width + 2);

    std::cout << '\n' << colorize(std::string(box::TopLeft) + border + std::string(box::TopRight),
                                   color::Cyan)
               << '\n';
    std::cout << colorize(box::Vertical, color::Cyan) << ' '
               << colorize(centerText(title, width), color::BoldCyan) << ' '
               << colorize(box::Vertical, color::Cyan) << '\n';
    if (!subtitle.empty()) {
        std::cout << colorize(box::Vertical, color::Cyan) << ' '
                   << colorize(centerText(subtitle, width), color::Gray) << ' '
                   << colorize(box::Vertical, color::Cyan) << '\n';
    }
    std::cout << colorize(std::string(box::BottomLeft) + border + std::string(box::BottomRight),
                           color::Cyan)
               << "\n\n";
}

void ConsoleUI::printBox(const std::vector<std::string>& lines, const std::string& title) {
    std::size_t contentWidth = title.empty() ? 0 : displayWidth(title);
    for (const auto& line : lines) {
        contentWidth = std::max(contentWidth, displayWidth(line));
    }
    contentWidth = std::max<std::size_t>(contentWidth, 1);

    const std::string border = repeat(box::Horizontal, contentWidth + 2);

    std::cout << colorize(std::string(box::TopLeft) + border + std::string(box::TopRight),
                           color::Cyan)
               << '\n';

    if (!title.empty()) {
        const std::size_t pad = contentWidth - displayWidth(title);
        std::cout << colorize(box::Vertical, color::Cyan) << ' '
                   << colorize(title, color::BoldCyan) << std::string(pad, ' ') << ' '
                   << colorize(box::Vertical, color::Cyan) << '\n';
        std::cout << colorize(std::string(box::TeeRight) + border + std::string(box::TeeLeft),
                               color::Cyan)
                   << '\n';
    }

    for (const auto& line : lines) {
        const std::size_t pad = contentWidth - displayWidth(line);
        std::cout << colorize(box::Vertical, color::Cyan) << ' ' << line << std::string(pad, ' ')
                   << ' ' << colorize(box::Vertical, color::Cyan) << '\n';
    }

    std::cout << colorize(std::string(box::BottomLeft) + border + std::string(box::BottomRight),
                           color::Cyan)
               << '\n';
}

void ConsoleUI::printTable(const std::vector<std::string>& headers,
                            const std::vector<std::vector<std::string>>& rows,
                            const std::string& title) {
    const std::size_t columnCount = headers.size();
    std::vector<std::size_t> widths(columnCount);
    for (std::size_t c = 0; c < columnCount; ++c) {
        widths[c] = displayWidth(headers[c]);
    }
    for (const auto& row : rows) {
        for (std::size_t c = 0; c < columnCount && c < row.size(); ++c) {
            widths[c] = std::max(widths[c], displayWidth(row[c]));
        }
    }

    const auto buildBorder = [&](std::string_view left, std::string_view mid,
                                  std::string_view right) {
        std::string line(left);
        for (std::size_t c = 0; c < columnCount; ++c) {
            line += repeat(box::Horizontal, widths[c] + 2);
            line += (c + 1 < columnCount) ? std::string(mid) : std::string(right);
        }
        return line;
    };

    const auto printRow = [&](const std::vector<std::string>& cells, bool emphasize) {
        std::cout << colorize(box::Vertical, color::Cyan);
        for (std::size_t c = 0; c < columnCount; ++c) {
            const std::string cell = c < cells.size() ? cells[c] : std::string();
            const std::size_t pad = widths[c] - displayWidth(cell);
            std::cout << ' ' << (emphasize ? colorize(cell, color::BoldCyan) : cell)
                       << std::string(pad, ' ') << ' ' << colorize(box::Vertical, color::Cyan);
        }
        std::cout << '\n';
    };

    if (!title.empty()) {
        std::cout << colorize(title, color::BoldMagenta) << '\n';
    }
    std::cout << colorize(buildBorder(box::TopLeft, box::TeeDown, box::TopRight), color::Cyan)
               << '\n';
    printRow(headers, true);
    std::cout << colorize(buildBorder(box::TeeRight, box::Cross, box::TeeLeft), color::Cyan)
               << '\n';
    for (const auto& row : rows) {
        printRow(row, false);
    }
    std::cout << colorize(buildBorder(box::BottomLeft, box::TeeUp, box::BottomRight), color::Cyan)
               << '\n';
}

void ConsoleUI::printMenu(const std::string& title, const std::vector<std::string>& options) {
    std::vector<std::string> lines;
    lines.reserve(options.size());
    for (std::size_t i = 0; i < options.size(); ++i) {
        lines.push_back(std::to_string(i + 1) + ". " + options[i]);
    }
    printBox(lines, title);
}

void ConsoleUI::printSuccess(const std::string& message) {
    std::cout << colorize("✔ ", color::BoldGreen) << message << '\n';
}

void ConsoleUI::printError(const std::string& message) {
    std::cout << colorize("✘ ", color::BoldRed) << message << '\n';
}

void ConsoleUI::printWarning(const std::string& message) {
    std::cout << colorize("⚠ ", color::BoldYellow) << message << '\n';
}

void ConsoleUI::printInfo(const std::string& message) {
    std::cout << colorize("ℹ ", color::BoldCyan) << message << '\n';
}

std::string ConsoleUI::readLine(const std::string& prompt) {
    std::cout << colorize("❯ ", color::BoldMagenta) << prompt << ": ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw InputClosedException{};
    }
    return line;
}

int ConsoleUI::readInt(const std::string& prompt) {
    while (true) {
        const std::string line = readLine(prompt);
        try {
            std::size_t consumed = 0;
            const int value = std::stoi(line, &consumed);
            if (consumed == line.size()) {
                return value;
            }
        } catch (...) {
            // fall through to the error message below
        }
        printError("Please enter a valid whole number.");
    }
}

double ConsoleUI::readDouble(const std::string& prompt) {
    while (true) {
        const std::string line = readLine(prompt);
        try {
            std::size_t consumed = 0;
            const double value = std::stod(line, &consumed);
            if (consumed == line.size()) {
                return value;
            }
        } catch (...) {
            // fall through to the error message below
        }
        printError("Please enter a valid number.");
    }
}

int ConsoleUI::readMenuChoice(int minValue, int maxValue) {
    while (true) {
        const int value =
            readInt("Select an option (" + std::to_string(minValue) + "-" +
                     std::to_string(maxValue) + ")");
        if (value >= minValue && value <= maxValue) {
            return value;
        }
        printError("Choice must be between " + std::to_string(minValue) + " and " +
                    std::to_string(maxValue) + ".");
    }
}

void ConsoleUI::pause(const std::string& message) {
    std::cout << colorize(message, color::Gray);
    std::string discard;
    std::getline(std::cin, discard);
}

} // namespace polycalc
