#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace polycalc::web {

struct HttpRequest {
    std::string method;
    std::string path; // decoded, query string already stripped
    std::string body;
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/plain; charset=utf-8";
    std::string body;

    static HttpResponse json(std::string body, int status = 200);
    static HttpResponse text(std::string body, int status = 200);
    static HttpResponse notFound();
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

// A file baked into the executable at build time - see
// cmake/EmbedWebAssets.cmake and the generated WebAssets.hpp.
struct EmbeddedAsset {
    std::string_view path;
    std::string_view mimeType;
    std::string_view content;
};

// A minimal, loopback-only HTTP/1.1 server: exact-path routing for GET/POST,
// embedded static asset serving, one worker thread per connection, no
// keep-alive, no chunked transfer-encoding. This app only ever talks to the
// single browser tab it opens on startup, making a handful of small
// requests at a time - anything beyond this would be complexity this
// project's actual usage never exercises. See the README for why no
// third-party HTTP library (e.g. cpp-httplib) is used instead.
class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void get(std::string path, HttpHandler handler);
    void post(std::string path, HttpHandler handler);

    // Registers the embedded static file table served for any GET that
    // doesn't match a route registered above. The pointer must outlive the
    // server (the generated table is a static constexpr array).
    void setStaticAssets(const EmbeddedAsset* assets, std::size_t count);

    // Binds 127.0.0.1 on an OS-assigned free port and returns it. Throws
    // std::runtime_error on failure.
    std::uint16_t bindLoopback();

    // Accepts and serves connections until stop() is called (typically from
    // a handler running on another connection's worker thread, e.g. a
    // "quit" endpoint). Blocks the calling thread.
    void run();
    void stop();

private:
    struct Route {
        std::string method;
        std::string path;
        HttpHandler handler;
    };

    // Kept as an opaque handle so this header never has to include
    // winsock2.h (and therefore never forces WIN32_LEAN_AND_MEAN/NOMINMAX
    // ordering concerns onto anything that just wants to register routes).
    std::uintptr_t listenSocket_;
    bool winsockStarted_ = false;
    bool running_ = false;

    std::vector<Route> routes_;
    const EmbeddedAsset* assets_ = nullptr;
    std::size_t assetCount_ = 0;

    void serveConnection(std::uintptr_t clientSocket);
    HttpResponse dispatch(const HttpRequest& request);
};

} // namespace polycalc::web
