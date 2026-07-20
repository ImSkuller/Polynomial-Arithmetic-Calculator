#include "polycalc/web/HttpServer.hpp"

#include "polycalc/web/Win32.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

namespace polycalc::web {

namespace {

// Guards against a misbehaving or malicious client streaming an unbounded
// header block at a loopback-only server that's never meant to see one.
constexpr std::size_t kMaxHeaderBytes = 64 * 1024;

std::string statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

std::string urlDecode(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() &&
            std::isxdigit(static_cast<unsigned char>(text[i + 1])) != 0 &&
            std::isxdigit(static_cast<unsigned char>(text[i + 2])) != 0) {
            const std::string hex = text.substr(i + 1, 2);
            result += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else {
            result += text[i];
        }
    }
    return result;
}

} // namespace

HttpResponse HttpResponse::json(std::string body, int status) {
    HttpResponse response;
    response.status = status;
    response.contentType = "application/json; charset=utf-8";
    response.body = std::move(body);
    return response;
}

HttpResponse HttpResponse::text(std::string body, int status) {
    HttpResponse response;
    response.status = status;
    response.contentType = "text/plain; charset=utf-8";
    response.body = std::move(body);
    return response;
}

HttpResponse HttpResponse::notFound() { return HttpResponse::text("Not found", 404); }

HttpServer::HttpServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("Failed to initialize Winsock");
    }
    winsockStarted_ = true;
    listenSocket_ = static_cast<std::uintptr_t>(INVALID_SOCKET);
}

HttpServer::~HttpServer() {
    if (static_cast<SOCKET>(listenSocket_) != INVALID_SOCKET) {
        closesocket(static_cast<SOCKET>(listenSocket_));
    }
    if (winsockStarted_) {
        WSACleanup();
    }
}

void HttpServer::get(std::string path, HttpHandler handler) {
    routes_.push_back(Route{"GET", std::move(path), std::move(handler)});
}

void HttpServer::post(std::string path, HttpHandler handler) {
    routes_.push_back(Route{"POST", std::move(path), std::move(handler)});
}

void HttpServer::setStaticAssets(const EmbeddedAsset* assets, std::size_t count) {
    assets_ = assets;
    assetCount_ = count;
}

std::uint16_t HttpServer::bindLoopback() {
    const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create the listening socket");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0); // let the OS assign a free port

    if (::bind(listenSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
        SOCKET_ERROR) {
        closesocket(listenSocket);
        throw std::runtime_error("Failed to bind to 127.0.0.1");
    }
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        throw std::runtime_error("Failed to listen on the bound socket");
    }

    int addressLength = sizeof(address);
    if (getsockname(listenSocket, reinterpret_cast<sockaddr*>(&address), &addressLength) ==
        SOCKET_ERROR) {
        closesocket(listenSocket);
        throw std::runtime_error("Failed to read back the bound port");
    }

    listenSocket_ = static_cast<std::uintptr_t>(listenSocket);
    return ntohs(address.sin_port);
}

void HttpServer::run() {
    running_ = true;
    const SOCKET listenSocket = static_cast<SOCKET>(listenSocket_);
    while (running_) {
        sockaddr_in clientAddress{};
        int clientAddressLength = sizeof(clientAddress);
        const SOCKET clientSocket =
            accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            // Expected once stop() closes the listening socket out from
            // under a blocked accept(); anything else, just keep serving.
            if (!running_) {
                break;
            }
            continue;
        }
        std::thread(&HttpServer::serveConnection, this, static_cast<std::uintptr_t>(clientSocket))
            .detach();
    }
}

void HttpServer::stop() {
    running_ = false;
    const SOCKET listenSocket = static_cast<SOCKET>(listenSocket_);
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
        listenSocket_ = static_cast<std::uintptr_t>(INVALID_SOCKET);
    }
}

void HttpServer::serveConnection(std::uintptr_t clientSocketHandle) {
    const SOCKET clientSocket = static_cast<SOCKET>(clientSocketHandle);

    std::string buffer;
    char chunk[4096];

    std::size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        const int received = recv(clientSocket, chunk, sizeof(chunk), 0);
        if (received <= 0) {
            closesocket(clientSocket);
            return;
        }
        buffer.append(chunk, static_cast<std::size_t>(received));
        headerEnd = buffer.find("\r\n\r\n");
        if (buffer.size() > kMaxHeaderBytes) {
            closesocket(clientSocket);
            return;
        }
    }

    const std::string headerBlock = buffer.substr(0, headerEnd);
    std::string body = buffer.substr(headerEnd + 4);

    std::istringstream headerStream(headerBlock);
    std::string requestLine;
    std::getline(headerStream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    std::string method;
    std::string target;
    std::string httpVersion;
    requestLineStream >> method >> target >> httpVersion;

    std::size_t contentLength = 0;
    std::string headerLine;
    while (std::getline(headerStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        const std::size_t colon = headerLine.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string name = headerLine.substr(0, colon);
        std::string value = headerLine.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }
        for (char& c : name) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (name == "content-length") {
            try {
                contentLength = static_cast<std::size_t>(std::stoul(value));
            } catch (...) {
                contentLength = 0;
            }
        }
    }

    while (body.size() < contentLength) {
        const int received = recv(clientSocket, chunk, sizeof(chunk), 0);
        if (received <= 0) {
            break;
        }
        body.append(chunk, static_cast<std::size_t>(received));
    }
    if (body.size() > contentLength) {
        body.resize(contentLength);
    }

    HttpRequest request;
    request.method = method;
    const std::size_t queryPos = target.find('?');
    request.path = urlDecode(queryPos == std::string::npos ? target : target.substr(0, queryPos));
    request.body = std::move(body);

    const HttpResponse response = dispatch(request);

    std::ostringstream responseStream;
    responseStream << "HTTP/1.1 " << response.status << ' ' << statusText(response.status)
                    << "\r\n"
                    << "Content-Type: " << response.contentType << "\r\n"
                    << "Content-Length: " << response.body.size() << "\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << response.body;
    const std::string responseText = responseStream.str();
    send(clientSocket, responseText.data(), static_cast<int>(responseText.size()), 0);

    shutdown(clientSocket, SD_SEND);
    closesocket(clientSocket);
}

HttpResponse HttpServer::dispatch(const HttpRequest& request) {
    try {
        for (const Route& route : routes_) {
            if (route.method == request.method && route.path == request.path) {
                return route.handler(request);
            }
        }
        if (request.method == "GET") {
            for (std::size_t i = 0; i < assetCount_; ++i) {
                if (assets_[i].path == request.path) {
                    HttpResponse response;
                    response.contentType = std::string(assets_[i].mimeType);
                    response.body = std::string(assets_[i].content);
                    return response;
                }
            }
        }
        return HttpResponse::notFound();
    } catch (const std::exception& ex) {
        // A safety net only - every route this app registers (see
        // ApiServer::wrap) already catches and reports its own exceptions
        // as a structured JSON error, so this path should never trigger in
        // practice.
        return HttpResponse::text(std::string("Internal error: ") + ex.what(), 500);
    }
}

} // namespace polycalc::web
