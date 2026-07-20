#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace polycalc::web {

// A minimal JSON value type: just enough to build API responses and parse
// the small, flat request bodies this app's endpoints receive (a handful of
// string/number/bool fields, never deeply nested). No external JSON library
// is used, mirroring PolynomialParser's own hand-rolled parser elsewhere in
// this project - the grammar needed here is tiny and self-contained.
//
// Objects and arrays preserve insertion order (backed by a vector of pairs,
// not a map), so a serialized response's key order matches the order it was
// built in.
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Json() noexcept : type_(Type::Null) {}

    static Json makeNull() { return Json(); }
    static Json makeBool(bool value);
    static Json makeNumber(double value);
    static Json makeString(std::string value);
    static Json makeArray();
    static Json makeObject();

    Type type() const noexcept { return type_; }
    bool isNull() const noexcept { return type_ == Type::Null; }

    // Object helpers. set() returns *this so calls can be chained while
    // building a response.
    Json& set(const std::string& key, Json value);
    bool has(const std::string& key) const noexcept;
    const Json& at(const std::string& key) const;

    // Array helper.
    void push(Json value);

    // Value accessors. Each throws std::invalid_argument if the value is
    // not of the requested type - acceptable here because every call site
    // is reading a request body it just parsed and wants a clear parse
    // error over a silent wrong-type default.
    bool asBool() const;
    double asNumber() const;
    int asInt() const;
    const std::string& asString() const;

    // Like at()/asX(), but returns fallback instead of throwing when the
    // key is absent (still throws if the key exists with the wrong type).
    double numberOr(const std::string& key, double fallback) const;
    int intOr(const std::string& key, int fallback) const;
    std::string stringOr(const std::string& key, const std::string& fallback) const;

    // Serializes to compact JSON text (no pretty-printing needed for a
    // machine-to-fetch()-API response).
    std::string dump() const;

    // Throws std::invalid_argument with a description of where parsing
    // failed.
    static Json parse(const std::string& text);

private:
    Type type_;
    bool boolValue_ = false;
    double numberValue_ = 0.0;
    std::string stringValue_;
    std::vector<Json> arrayValue_;
    std::vector<std::pair<std::string, Json>> objectValue_;

    void dumpTo(std::string& out) const;
};

} // namespace polycalc::web
