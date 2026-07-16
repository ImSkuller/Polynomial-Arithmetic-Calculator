#include <exception>
#include <string>

#include "polycalc/Application.hpp"
#include "polycalc/console/ConsoleUI.hpp"

int main() {
    try {
        polycalc::Application application;
        return application.run();
    } catch (const polycalc::InputClosedException&) {
        polycalc::ConsoleUI::printInfo("Input closed. Goodbye!");
        return 0;
    } catch (const std::exception& ex) {
        polycalc::ConsoleUI::printError(std::string("Fatal error: ") + ex.what());
        return 1;
    }
}
