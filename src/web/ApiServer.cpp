#include "polycalc/web/ApiServer.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>

#include "polycalc/core/Formatting.hpp"
#include "polycalc/services/PolynomialGenerator.hpp"
#include "polycalc/web/FileDialog.hpp"
#include "polycalc/web/WebAssets.hpp"

namespace polycalc::web {

HttpHandler ApiServer::wrap(std::function<Json(const Json&)> handler) {
    return [handler = std::move(handler)](const HttpRequest& request) -> HttpResponse {
        try {
            const Json requestBody = request.body.empty() ? Json::makeObject() : Json::parse(request.body);
            Json result = handler(requestBody);
            result.set("ok", Json::makeBool(true));
            return HttpResponse::json(result.dump());
        } catch (const std::exception& ex) {
            Json error = Json::makeObject();
            error.set("ok", Json::makeBool(false));
            error.set("error", Json::makeString(ex.what()));
            return HttpResponse::json(error.dump(), 400);
        }
    };
}

Json ApiServer::stateJson() const {
    Json state = Json::makeObject();
    state.set("currentText", Json::makeString(app_.currentText()));
    state.set("currentDegree", Json::makeString(app_.degreeText()));
    state.set("termCount", Json::makeNumber(static_cast<double>(app_.termCount())));
    state.set("secondaryText", Json::makeString(app_.secondaryText()));
    state.set("canUndo", Json::makeBool(app_.canUndo()));
    state.set("canRedo", Json::makeBool(app_.canRedo()));
    state.set("undoDepth", Json::makeNumber(static_cast<double>(app_.undoDepth())));
    state.set("redoDepth", Json::makeNumber(static_cast<double>(app_.redoDepth())));
    state.set("hasArithmeticResult", Json::makeBool(hasArithmeticResult_));
    return state;
}

Json ApiServer::statisticsJson() const {
    const Statistics stats = app_.statistics();
    Json json = Json::makeObject();
    json.set("termCount", Json::makeNumber(static_cast<double>(stats.termCount)));
    json.set("degreeText", Json::makeString(stats.degree < 0 ? "-" : std::to_string(stats.degree)));
    json.set("memoryBytes", Json::makeNumber(static_cast<double>(stats.memoryBytes)));
    json.set("sortMs", Json::makeNumber(stats.sortMs));
    json.set("mergeMs", Json::makeNumber(stats.mergeMs));
    json.set("simplifyMs", Json::makeNumber(stats.simplifyMs));
    json.set("evaluateMs", Json::makeNumber(stats.evaluateMs));
    return json;
}

void ApiServer::registerRoutes() {
    server_.get("/api/state", wrap([this](const Json&) {
        Json result = Json::makeObject();
        result.set("state", stateJson());
        return result;
    }));

    server_.get("/api/statistics", wrap([this](const Json&) {
        Json result = Json::makeObject();
        result.set("stats", statisticsJson());
        return result;
    }));

    server_.post("/api/expression", wrap([this](const Json& body) {
        app_.setCurrentFromExpression(body.at("expression").asString());
        Json result = Json::makeObject();
        result.set("message", Json::makeString("P(x) set to: " + app_.currentText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/term/insert", wrap([this](const Json& body) {
        const double coefficient = body.at("coefficient").asNumber();
        const int exponent = body.at("exponent").asInt();
        const bool changed = app_.insertTerm(coefficient, exponent);
        Json result = Json::makeObject();
        result.set("changed", Json::makeBool(changed));
        result.set("message", Json::makeString(changed ? "Inserted. P(x) = " + app_.currentText()
                                                         : "A zero coefficient was ignored."));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/term/delete", wrap([this](const Json& body) {
        const int exponent = body.at("exponent").asInt();
        const bool changed = app_.deleteTerm(exponent);
        Json result = Json::makeObject();
        result.set("changed", Json::makeBool(changed));
        result.set("message", Json::makeString(changed ? "Removed. P(x) = " + app_.currentText()
                                                         : "No term with that exponent exists."));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/term/update-coefficient", wrap([this](const Json& body) {
        const int exponent = body.at("exponent").asInt();
        const double coefficient = body.at("coefficient").asNumber();
        const bool changed = app_.updateCoefficient(exponent, coefficient);
        Json result = Json::makeObject();
        result.set("changed", Json::makeBool(changed));
        result.set("message", Json::makeString(changed ? "Updated. P(x) = " + app_.currentText()
                                                         : "No term with that exponent exists."));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/term/update-exponent", wrap([this](const Json& body) {
        const int oldExponent = body.at("oldExponent").asInt();
        const int newExponent = body.at("newExponent").asInt();
        const bool changed = app_.updateExponent(oldExponent, newExponent);
        Json result = Json::makeObject();
        result.set("changed", Json::makeBool(changed));
        result.set("message", Json::makeString(changed ? "Updated. P(x) = " + app_.currentText()
                                                         : "No term with that exponent exists."));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/clear", wrap([this](const Json&) {
        app_.clearCurrent();
        Json result = Json::makeObject();
        result.set("message", Json::makeString(std::string("P(x) cleared.")));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/secondary", wrap([this](const Json& body) {
        app_.setSecondaryFromExpression(body.at("expression").asString());
        Json result = Json::makeObject();
        result.set("message", Json::makeString("Q(x) set to: " + app_.secondaryText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/arithmetic", wrap([this](const Json& body) {
        const std::string op = body.at("op").asString();
        Polynomial computed;
        std::string label;
        if (op == "add") {
            computed = app_.add();
            label = "P + Q";
        } else if (op == "subtract") {
            computed = app_.subtract();
            label = "P - Q";
        } else if (op == "multiply") {
            computed = app_.multiply();
            label = "P * Q";
        } else {
            throw std::invalid_argument("Unknown arithmetic operation: " + op);
        }

        const std::string resultText = computed.toString();
        lastArithmeticResult_ = std::move(computed);
        hasArithmeticResult_ = true;

        Json result = Json::makeObject();
        result.set("label", Json::makeString(label));
        result.set("resultText", Json::makeString(resultText));
        result.set("message", Json::makeString(label + " = " + resultText));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/use-result", wrap([this](const Json&) {
        if (!hasArithmeticResult_) {
            throw std::invalid_argument("Compute P + Q, P - Q, or P * Q first.");
        }
        app_.adoptAsCurrent(lastArithmeticResult_);
        Json result = Json::makeObject();
        result.set("message",
                    Json::makeString("P(x) is now the arithmetic result: " + app_.currentText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/evaluate", wrap([this](const Json& body) {
        const double x = body.at("x").asNumber();
        const double value = app_.evaluate(x);
        Json result = Json::makeObject();
        result.set("x", Json::makeNumber(x));
        result.set("result", Json::makeNumber(value));
        result.set("message", Json::makeString("P(" + formatCoefficient(x) + ") = " +
                                                 formatCoefficient(value)));
        result.set("state", stateJson());
        return result;
    }));

    // Sort/merge/simplify all share the same shape: run one Application
    // timing method, report how long it took and the resulting P(x).
    const auto registerCanonicalize = [this](const std::string& path, const std::string& pastTense,
                                              double (Application::*operation)()) {
        server_.post(path, wrap([this, pastTense, operation](const Json&) {
            const double elapsed = (app_.*operation)();
            Json result = Json::makeObject();
            result.set("elapsedMs", Json::makeNumber(elapsed));
            result.set("message", Json::makeString(pastTense + " in " + formatCoefficient(elapsed) +
                                                     " ms. P(x) = " + app_.currentText()));
            result.set("state", stateJson());
            return result;
        }));
    };
    registerCanonicalize("/api/sort", "Sorted", &Application::sortByExponent);
    registerCanonicalize("/api/merge", "Merged", &Application::mergeLikeTerms);
    registerCanonicalize("/api/simplify", "Simplified", &Application::simplify);

    server_.post("/api/undo", wrap([this](const Json&) {
        app_.undo();
        Json result = Json::makeObject();
        result.set("message", Json::makeString("Undone. P(x) = " + app_.currentText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/redo", wrap([this](const Json&) {
        app_.redo();
        Json result = Json::makeObject();
        result.set("message", Json::makeString("Redone. P(x) = " + app_.currentText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/save", wrap([this](const Json& body) {
        const std::string path = body.at("path").asString();
        if (path.empty()) {
            throw std::invalid_argument("Enter or browse to a file path first.");
        }
        app_.saveToFile(path);
        Json result = Json::makeObject();
        result.set("message", Json::makeString("Saved to " + path));
        return result;
    }));

    server_.post("/api/load", wrap([this](const Json& body) {
        const std::string path = body.at("path").asString();
        if (path.empty()) {
            throw std::invalid_argument("Enter or browse to a file path first.");
        }
        app_.loadFromFile(path);
        Json result = Json::makeObject();
        result.set("message", Json::makeString("Loaded. P(x) = " + app_.currentText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/browse/open", wrap([](const Json& body) {
        const auto picked = FileDialog::openPolynomialFile(body.stringOr("path", ""));
        Json result = Json::makeObject();
        result.set("picked", Json::makeBool(picked.has_value()));
        result.set("path", Json::makeString(picked.value_or("")));
        return result;
    }));

    server_.post("/api/browse/save", wrap([](const Json& body) {
        const auto picked = FileDialog::savePolynomialFile(body.stringOr("path", ""));
        Json result = Json::makeObject();
        result.set("picked", Json::makeBool(picked.has_value()));
        result.set("path", Json::makeString(picked.value_or("")));
        return result;
    }));

    server_.post("/api/generate", wrap([this](const Json& body) {
        PolynomialGenerator::Options options;
        options.termCount = body.at("termCount").asInt();
        options.maxExponent = body.at("maxExponent").asInt();
        options.minCoefficient = body.at("minCoefficient").asNumber();
        options.maxCoefficient = body.at("maxCoefficient").asNumber();
        app_.generateRandom(options);
        Json result = Json::makeObject();
        result.set("message", Json::makeString("Generated. P(x) = " + app_.currentText()));
        result.set("state", stateJson());
        return result;
    }));

    server_.post("/api/shutdown", wrap([this](const Json&) {
        Json result = Json::makeObject();
        result.set("message", Json::makeString(std::string("Shutting down.")));
        // Reply before stopping - stopping closes the listening socket from
        // this same request's worker thread, which is safe (see
        // HttpServer::stop()), but give the response a moment to actually
        // reach the browser first.
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            stop();
        }).detach();
        return result;
    }));
}

std::string ApiServer::start() {
    server_.setStaticAssets(kEmbeddedAssets, kEmbeddedAssetCount);
    registerRoutes();
    port_ = server_.bindLoopback();
    return "http://127.0.0.1:" + std::to_string(port_) + "/";
}

void ApiServer::run() { server_.run(); }

void ApiServer::stop() { server_.stop(); }

} // namespace polycalc::web
