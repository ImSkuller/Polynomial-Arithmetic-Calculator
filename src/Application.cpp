#include "polycalc/Application.hpp"

#include <utility>

#include "polycalc/Version.hpp"
#include "polycalc/console/ConsoleUI.hpp"
#include "polycalc/console/Terminal.hpp"
#include "polycalc/core/Formatting.hpp"
#include "polycalc/core/Node.hpp"
#include "polycalc/core/PolynomialParser.hpp"
#include "polycalc/services/PolynomialGenerator.hpp"
#include "polycalc/services/PolynomialStorage.hpp"
#include "polycalc/services/Timer.hpp"

namespace polycalc {

void Application::recordIfChanged(const Polynomial& before, bool changed) {
    if (changed) {
        history_.record(before);
    }
}

void Application::printCurrentStatus() {
    const std::string degreeText = current_.isZero() ? "-" : std::to_string(current_.degree());
    ConsoleUI::printBox({"P(x) = " + current_.toString(),
                          "Degree: " + degreeText +
                              "    Terms: " + std::to_string(current_.termCount())},
                         "Current Polynomial");
}

int Application::run() {
    while (!exitRequested_) {
        Terminal::clearScreen();
        ConsoleUI::printBanner(std::string(kApplicationName), "v" + std::string(kVersion));
        printCurrentStatus();
        showMainMenu();
    }
    ConsoleUI::printInfo("Goodbye!");
    return 0;
}

void Application::showMainMenu() {
    ConsoleUI::printMenu("Main Menu", {
                                          "Build & Edit Polynomial",
                                          "View & Analyze",
                                          "Arithmetic Operations",
                                          "History (Undo / Redo)",
                                          "Save & Load",
                                          "Random Generator",
                                          "Statistics & Performance",
                                          "Exit",
                                      });
    switch (ConsoleUI::readMenuChoice(1, 8)) {
        case 1:
            handleBuildAndEdit();
            break;
        case 2:
            handleViewAndAnalyze();
            break;
        case 3:
            handleArithmetic();
            break;
        case 4:
            handleHistory();
            break;
        case 5:
            handleSaveAndLoad();
            break;
        case 6:
            handleRandomGenerator();
            break;
        case 7:
            handleStatistics();
            break;
        case 8:
            exitRequested_ = true;
            break;
        default:
            break;
    }
}

void Application::handleBuildAndEdit() {
    bool back = false;
    while (!back) {
        Terminal::clearScreen();
        printCurrentStatus();
        ConsoleUI::printMenu("Build & Edit Polynomial", {
                                                             "Create from expression (e.g. 3x^2 + 4x - 8)",
                                                             "Insert term",
                                                             "Delete term",
                                                             "Update coefficient",
                                                             "Update exponent",
                                                             "Clear polynomial",
                                                             "Back",
                                                         });
        const int choice = ConsoleUI::readMenuChoice(1, 7);
        try {
            switch (choice) {
                case 1: {
                    const std::string expression = ConsoleUI::readLine("Enter expression");
                    Polynomial parsed = PolynomialParser::parse(expression);
                    const Polynomial before = current_;
                    const bool changed = !(parsed == current_);
                    current_ = std::move(parsed);
                    recordIfChanged(before, changed);
                    ConsoleUI::printSuccess("Polynomial set to: " + current_.toString());
                    break;
                }
                case 2: {
                    const double coefficient = ConsoleUI::readDouble("Coefficient");
                    const int exponent = ConsoleUI::readInt("Exponent");
                    const Polynomial before = current_;
                    const bool changed = current_.insertTerm(coefficient, exponent);
                    recordIfChanged(before, changed);
                    if (changed) {
                        ConsoleUI::printSuccess("Inserted. P(x) = " + current_.toString());
                    } else {
                        ConsoleUI::printWarning("A zero coefficient was ignored.");
                    }
                    break;
                }
                case 3: {
                    const int exponent = ConsoleUI::readInt("Exponent to delete");
                    const Polynomial before = current_;
                    const bool changed = current_.deleteTerm(exponent);
                    recordIfChanged(before, changed);
                    if (changed) {
                        ConsoleUI::printSuccess("Removed. P(x) = " + current_.toString());
                    } else {
                        ConsoleUI::printWarning("No term with that exponent exists.");
                    }
                    break;
                }
                case 4: {
                    const int exponent = ConsoleUI::readInt("Exponent to update");
                    const double coefficient = ConsoleUI::readDouble("New coefficient");
                    const Polynomial before = current_;
                    const bool changed = current_.updateCoefficient(exponent, coefficient);
                    recordIfChanged(before, changed);
                    if (changed) {
                        ConsoleUI::printSuccess("Updated. P(x) = " + current_.toString());
                    } else {
                        ConsoleUI::printWarning("No term with that exponent exists.");
                    }
                    break;
                }
                case 5: {
                    const int oldExponent = ConsoleUI::readInt("Current exponent");
                    const int newExponent = ConsoleUI::readInt("New exponent");
                    const Polynomial before = current_;
                    const bool changed = current_.updateExponent(oldExponent, newExponent);
                    recordIfChanged(before, changed);
                    if (changed) {
                        ConsoleUI::printSuccess("Updated. P(x) = " + current_.toString());
                    } else {
                        ConsoleUI::printWarning("No term with that exponent exists.");
                    }
                    break;
                }
                case 6: {
                    const Polynomial before = current_;
                    const bool changed = !current_.isZero();
                    current_.clear();
                    recordIfChanged(before, changed);
                    ConsoleUI::printSuccess("Polynomial cleared.");
                    break;
                }
                case 7:
                    back = true;
                    break;
                default:
                    break;
            }
        } catch (const std::exception& ex) {
            ConsoleUI::printError(ex.what());
        }
        if (!back) {
            ConsoleUI::pause();
        }
    }
}

void Application::handleViewAndAnalyze() {
    bool back = false;
    while (!back) {
        Terminal::clearScreen();
        printCurrentStatus();
        ConsoleUI::printMenu("View & Analyze", {
                                                    "Display polynomial",
                                                    "Evaluate at x",
                                                    "Sort by exponent",
                                                    "Merge like terms",
                                                    "Simplify",
                                                    "Back",
                                                });
        const int choice = ConsoleUI::readMenuChoice(1, 6);
        try {
            switch (choice) {
                case 1:
                    ConsoleUI::printInfo("P(x) = " + current_.toString());
                    break;
                case 2: {
                    const double x = ConsoleUI::readDouble("Value of x");
                    const double result = current_.evaluate(x);
                    ConsoleUI::printSuccess("P(" + formatCoefficient(x) +
                                             ") = " + formatCoefficient(result));
                    break;
                }
                case 3: {
                    const double elapsed =
                        Timer::measureMilliseconds([this] { current_.sortByExponent(); });
                    ConsoleUI::printSuccess("Sorted in " + formatCoefficient(elapsed) +
                                             " ms. P(x) = " + current_.toString());
                    break;
                }
                case 4: {
                    const double elapsed =
                        Timer::measureMilliseconds([this] { current_.mergeLikeTerms(); });
                    ConsoleUI::printSuccess("Merged in " + formatCoefficient(elapsed) +
                                             " ms. P(x) = " + current_.toString());
                    break;
                }
                case 5: {
                    const double elapsed =
                        Timer::measureMilliseconds([this] { current_.simplify(); });
                    ConsoleUI::printSuccess("Simplified in " + formatCoefficient(elapsed) +
                                             " ms. P(x) = " + current_.toString());
                    break;
                }
                case 6:
                    back = true;
                    break;
                default:
                    break;
            }
        } catch (const std::exception& ex) {
            ConsoleUI::printError(ex.what());
        }
        if (!back) {
            ConsoleUI::pause();
        }
    }
}

void Application::handleArithmetic() {
    bool back = false;
    while (!back) {
        Terminal::clearScreen();
        printCurrentStatus();
        ConsoleUI::printBox({"Q(x) = " + secondary_.toString()}, "Second Polynomial");
        ConsoleUI::printMenu("Arithmetic Operations", {
                                                           "Set second polynomial from expression",
                                                           "Add: P + Q",
                                                           "Subtract: P - Q",
                                                           "Multiply: P * Q",
                                                           "Back",
                                                       });
        const int choice = ConsoleUI::readMenuChoice(1, 5);
        try {
            switch (choice) {
                case 1: {
                    const std::string expression = ConsoleUI::readLine("Enter expression for Q");
                    secondary_ = PolynomialParser::parse(expression);
                    ConsoleUI::printSuccess("Q(x) = " + secondary_.toString());
                    break;
                }
                case 2:
                case 3:
                case 4: {
                    Polynomial result;
                    std::string label;
                    if (choice == 2) {
                        result = current_ + secondary_;
                        label = "P + Q";
                    } else if (choice == 3) {
                        result = current_ - secondary_;
                        label = "P - Q";
                    } else {
                        result = current_ * secondary_;
                        label = "P * Q";
                    }
                    ConsoleUI::printSuccess(label + " = " + result.toString());
                    const std::string answer =
                        ConsoleUI::readLine("Set this as the current polynomial? (y/n)");
                    if (!answer.empty() && (answer.front() == 'y' || answer.front() == 'Y')) {
                        const Polynomial before = current_;
                        current_ = std::move(result);
                        recordIfChanged(before, true);
                    }
                    break;
                }
                case 5:
                    back = true;
                    break;
                default:
                    break;
            }
        } catch (const std::exception& ex) {
            ConsoleUI::printError(ex.what());
        }
        if (!back) {
            ConsoleUI::pause();
        }
    }
}

void Application::handleHistory() {
    bool back = false;
    while (!back) {
        Terminal::clearScreen();
        printCurrentStatus();
        ConsoleUI::printBox({"Undo steps available: " + std::to_string(history_.undoDepth()),
                              "Redo steps available: " + std::to_string(history_.redoDepth())},
                             "History");
        ConsoleUI::printMenu("History", {"Undo", "Redo", "Back"});
        const int choice = ConsoleUI::readMenuChoice(1, 3);
        try {
            switch (choice) {
                case 1:
                    current_ = history_.undo(current_);
                    ConsoleUI::printSuccess("Undone. P(x) = " + current_.toString());
                    break;
                case 2:
                    current_ = history_.redo(current_);
                    ConsoleUI::printSuccess("Redone. P(x) = " + current_.toString());
                    break;
                case 3:
                    back = true;
                    break;
                default:
                    break;
            }
        } catch (const std::exception& ex) {
            ConsoleUI::printError(ex.what());
        }
        if (!back) {
            ConsoleUI::pause();
        }
    }
}

void Application::handleSaveAndLoad() {
    bool back = false;
    while (!back) {
        Terminal::clearScreen();
        printCurrentStatus();
        ConsoleUI::printMenu("Save & Load", {
                                                 "Save current polynomial to file",
                                                 "Load polynomial from file",
                                                 "Back",
                                             });
        const int choice = ConsoleUI::readMenuChoice(1, 3);
        try {
            switch (choice) {
                case 1: {
                    const std::string path = ConsoleUI::readLine("File path to save to");
                    PolynomialStorage::save(current_, path);
                    ConsoleUI::printSuccess("Saved to " + path);
                    break;
                }
                case 2: {
                    const std::string path = ConsoleUI::readLine("File path to load from");
                    Polynomial loaded = PolynomialStorage::load(path);
                    const Polynomial before = current_;
                    current_ = std::move(loaded);
                    recordIfChanged(before, true);
                    ConsoleUI::printSuccess("Loaded. P(x) = " + current_.toString());
                    break;
                }
                case 3:
                    back = true;
                    break;
                default:
                    break;
            }
        } catch (const std::exception& ex) {
            ConsoleUI::printError(ex.what());
        }
        if (!back) {
            ConsoleUI::pause();
        }
    }
}

void Application::handleRandomGenerator() {
    Terminal::clearScreen();
    printCurrentStatus();
    ConsoleUI::printInfo("Generate a random polynomial to replace the current one.");
    try {
        PolynomialGenerator::Options options;
        options.termCount = ConsoleUI::readInt("Number of terms");
        options.maxExponent = ConsoleUI::readInt("Maximum exponent");
        options.minCoefficient = ConsoleUI::readDouble("Minimum coefficient");
        options.maxCoefficient = ConsoleUI::readDouble("Maximum coefficient");

        Polynomial generated = PolynomialGenerator::generate(options);
        const Polynomial before = current_;
        current_ = std::move(generated);
        recordIfChanged(before, true);
        ConsoleUI::printSuccess("Generated. P(x) = " + current_.toString());
    } catch (const std::exception& ex) {
        ConsoleUI::printError(ex.what());
    }
    ConsoleUI::pause();
}

void Application::handleStatistics() {
    Terminal::clearScreen();
    printCurrentStatus();

    const std::string degreeText = current_.isZero() ? "-" : std::to_string(current_.degree());
    const std::size_t memoryBytes = current_.termCount() * sizeof(Node);

    double sortTime = 0.0;
    double mergeTime = 0.0;
    double simplifyTime = 0.0;
    {
        Polynomial scratch = current_;
        sortTime = Timer::measureMilliseconds([&scratch] { scratch.sortByExponent(); });
    }
    {
        Polynomial scratch = current_;
        mergeTime = Timer::measureMilliseconds([&scratch] { scratch.mergeLikeTerms(); });
    }
    {
        Polynomial scratch = current_;
        simplifyTime = Timer::measureMilliseconds([&scratch] { scratch.simplify(); });
    }
    const double evaluateTime =
        Timer::measureMilliseconds([this] { current_.evaluate(2.0); });

    ConsoleUI::printTable(
        {"Metric", "Value"},
        {
            {"Term count", std::to_string(current_.termCount())},
            {"Degree", degreeText},
            {"Estimated memory",
             std::to_string(memoryBytes) + " bytes (" + std::to_string(sizeof(Node)) +
                 " bytes/node)"},
            {"sortByExponent() time", formatCoefficient(sortTime) + " ms"},
            {"mergeLikeTerms() time", formatCoefficient(mergeTime) + " ms"},
            {"simplify() time", formatCoefficient(simplifyTime) + " ms"},
            {"evaluate(2) time", formatCoefficient(evaluateTime) + " ms"},
        },
        "Statistics & Performance");

    ConsoleUI::pause();
}

} // namespace polycalc
