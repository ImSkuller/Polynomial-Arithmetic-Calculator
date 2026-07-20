#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "polycalc/Application.hpp"
#include "polycalc/core/Polynomial.hpp"
#include "polycalc/web/HttpServer.hpp"
#include "polycalc/web/Json.hpp"

namespace polycalc::web {

// Wires every polycalc::Application operation to an HTTP+JSON route under
// /api/..., and serves the embedded frontend for everything else. This is
// the web-era equivalent of gui::MainWindow: it owns the one piece of
// session state that lived on the window rather than in Application - the
// last computed P op Q arithmetic result - and every route body mirrors a
// MainWindow::onXxx handler one-to-one, just producing a JSON envelope
// instead of writing into Win32 controls.
class ApiServer {
public:
    // Binds a free loopback port and registers every route. Throws
    // std::runtime_error on failure. Returns the URL to open in a browser.
    std::string start();

    // Blocks, serving requests until stop() is called (from the
    // /api/shutdown route, or a Ctrl+C handler in main()).
    void run();
    void stop();

private:
    HttpServer server_;
    Application app_;
    Polynomial lastArithmeticResult_;
    bool hasArithmeticResult_ = false;
    std::uint16_t port_ = 0;

    void registerRoutes();

    // A snapshot of everything the frontend needs to redraw after a
    // mutation - current/secondary polynomial text, degree, term count, and
    // undo/redo availability - attached to almost every response, mirroring
    // how MainWindow::refreshCurrentDisplay()/refreshSecondaryDisplay()
    // were called after nearly every handler.
    Json stateJson() const;
    Json statisticsJson() const;

    // Parses the request body as JSON (an empty body is treated as `{}`,
    // which is what GET routes and no-argument POSTs receive), runs
    // `handler`, and folds the result into `{ok:true, ...}` - or, if
    // `handler` throws, into `{ok:false, error}` with HTTP 400. Centralizes
    // the try/catch every MainWindow::onXxx used to repeat individually.
    static HttpHandler wrap(std::function<Json(const Json& requestBody)> handler);
};

} // namespace polycalc::web
