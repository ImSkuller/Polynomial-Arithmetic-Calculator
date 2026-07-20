#include "polycalc/web/Win32.hpp"
#include <shellapi.h>

#include <exception>
#include <iostream>

#include "polycalc/Version.hpp"
#include "polycalc/web/ApiServer.hpp"
#include "polycalc/web/Utf8.hpp"

namespace {

// SetConsoleCtrlHandler's callback is a plain C function with no user-data
// parameter, so the one ApiServer this process ever creates is reachable
// through this file-local pointer instead.
polycalc::web::ApiServer* gServerForCtrlHandler = nullptr;

BOOL WINAPI handleConsoleEvent(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            if (gServerForCtrlHandler != nullptr) {
                std::cout << "\nStopping the server...\n";
                gServerForCtrlHandler->stop();
            }
            return TRUE;
        default:
            return FALSE;
    }
}

} // namespace

// A normal console entry point - see src/CMakeLists.txt for why polycalc is
// no longer built as a WIN32-subsystem (windowless) executable. The console
// this prints to is this app's status window now: it reports the local URL
// it's serving the UI on, and stays open for as long as the server runs.
int main() {
    try {
        polycalc::web::ApiServer server;
        const std::string url = server.start();

        gServerForCtrlHandler = &server;
        SetConsoleCtrlHandler(&handleConsoleEvent, TRUE);

        std::cout << std::string(polycalc::kApplicationName) << " v" << polycalc::kVersion << "\n";
        std::cout << "Serving the web UI at " << url << "\n";
        std::cout << "Press Ctrl+C, close this window, or use the in-page Quit button to stop.\n\n";
        std::cout.flush();

        ShellExecuteW(nullptr, L"open", polycalc::web::Utf8ToWide(url).c_str(), nullptr, nullptr,
                      SW_SHOWNORMAL);

        server.run();

        std::cout << "Server stopped.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }
}
