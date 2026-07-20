#include "polycalc/web/Json.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace polycalc::web {

namespace {

// A small recursive-descent JSON parser. Only the subset of JSON this app's
// request bodies ever use is exercised in practice (flat objects of
// strings/numbers), but the parser itself handles the full grammar so it
// never surprises a caller with an unsupported-but-valid document.
class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    Json parseDocument() {
        Json value = parseValue();
        skipWhitespace();
        if (pos_ != text_.size()) {
            fail("Unexpected trailing content");
        }
        return value;
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& message) {
        throw std::invalid_argument("JSON parse error at offset " + std::to_string(pos_) + ": " +
                                     message);
    }

    bool atEnd() const noexcept { return pos_ >= text_.size(); }
    char peek() const { return text_[pos_]; }

    void skipWhitespace() {
        while (!atEnd()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    void expect(char c) {
        if (atEnd() || peek() != c) {
            fail(std::string("Expected '") + c + "'");
        }
        ++pos_;
    }

    bool consumeLiteral(const char* literal) {
        const std::size_t length = std::char_traits<char>::length(literal);
        if (text_.compare(pos_, length, literal) == 0) {
            pos_ += length;
            return true;
        }
        return false;
    }

    Json parseValue() {
        skipWhitespace();
        if (atEnd()) {
            fail("Unexpected end of input");
        }
        switch (peek()) {
            case '{':
                return parseObject();
            case '[':
                return parseArray();
            case '"':
                return Json::makeString(parseString());
            case 't':
                if (consumeLiteral("true")) return Json::makeBool(true);
                fail("Invalid literal");
            case 'f':
                if (consumeLiteral("false")) return Json::makeBool(false);
                fail("Invalid literal");
            case 'n':
                if (consumeLiteral("null")) return Json::makeNull();
                fail("Invalid literal");
            default:
                return parseNumber();
        }
    }

    Json parseObject() {
        expect('{');
        Json result = Json::makeObject();
        skipWhitespace();
        if (!atEnd() && peek() == '}') {
            ++pos_;
            return result;
        }
        while (true) {
            skipWhitespace();
            if (atEnd() || peek() != '"') {
                fail("Expected a string key");
            }
            std::string key = parseString();
            skipWhitespace();
            expect(':');
            Json value = parseValue();
            result.set(key, std::move(value));
            skipWhitespace();
            if (!atEnd() && peek() == ',') {
                ++pos_;
                continue;
            }
            break;
        }
        skipWhitespace();
        expect('}');
        return result;
    }

    Json parseArray() {
        expect('[');
        Json result = Json::makeArray();
        skipWhitespace();
        if (!atEnd() && peek() == ']') {
            ++pos_;
            return result;
        }
        while (true) {
            result.push(parseValue());
            skipWhitespace();
            if (!atEnd() && peek() == ',') {
                ++pos_;
                continue;
            }
            break;
        }
        skipWhitespace();
        expect(']');
        return result;
    }

    std::string parseString() {
        expect('"');
        std::string result;
        while (true) {
            if (atEnd()) {
                fail("Unterminated string");
            }
            const char c = text_[pos_++];
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (atEnd()) {
                    fail("Unterminated escape sequence");
                }
                const char escaped = text_[pos_++];
                switch (escaped) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': result += parseUnicodeEscape(); break;
                    default:
                        fail("Invalid escape sequence");
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    // Decodes a \uXXXX escape (and its low surrogate, if present) into UTF-8.
    // Request bodies from this app's own frontend never actually need this
    // (JS's JSON.stringify only escapes control characters), but a correct
    // JSON parser has to handle it.
    std::string parseUnicodeEscape() {
        auto readHex4 = [this]() -> unsigned int {
            if (pos_ + 4 > text_.size()) {
                fail("Truncated \\u escape");
            }
            unsigned int value = 0;
            for (int i = 0; i < 4; ++i) {
                const char c = text_[pos_++];
                value <<= 4;
                if (c >= '0' && c <= '9') value |= static_cast<unsigned int>(c - '0');
                else if (c >= 'a' && c <= 'f') value |= static_cast<unsigned int>(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned int>(c - 'A' + 10);
                else fail("Invalid hex digit in \\u escape");
            }
            return value;
        };

        unsigned int codepoint = readHex4();
        if (codepoint >= 0xD800 && codepoint <= 0xDBFF && text_.compare(pos_, 2, "\\u") == 0) {
            const std::size_t savedPos = pos_;
            pos_ += 2;
            const unsigned int low = readHex4();
            if (low >= 0xDC00 && low <= 0xDFFF) {
                codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
            } else {
                pos_ = savedPos;
            }
        }

        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8 += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            utf8 += static_cast<char>(0xC0 | (codepoint >> 6));
            utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint <= 0xFFFF) {
            utf8 += static_cast<char>(0xE0 | (codepoint >> 12));
            utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            utf8 += static_cast<char>(0xF0 | (codepoint >> 18));
            utf8 += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        return utf8;
    }

    Json parseNumber() {
        const std::size_t start = pos_;
        if (!atEnd() && peek() == '-') {
            ++pos_;
        }
        if (atEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) {
            fail("Invalid number");
        }
        while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            ++pos_;
        }
        if (!atEnd() && peek() == '.') {
            ++pos_;
            if (atEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                fail("Invalid number");
            }
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        if (!atEnd() && (peek() == 'e' || peek() == 'E')) {
            ++pos_;
            if (!atEnd() && (peek() == '+' || peek() == '-')) {
                ++pos_;
            }
            if (atEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                fail("Invalid number");
            }
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
            }
        }
        const std::string token = text_.substr(start, pos_ - start);
        try {
            return Json::makeNumber(std::stod(token));
        } catch (...) {
            fail("Number out of range");
        }
    }
};

void appendEscapedString(std::string& out, const std::string& value) {
    out += '"';
    for (const unsigned char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                    out += buffer;
                } else {
                    // UTF-8 multi-byte sequences (e.g. superscript digits in
                    // a polynomial's display string) pass through as-is;
                    // JSON strings are UTF-8 by definition.
                    out += static_cast<char>(c);
                }
        }
    }
    out += '"';
}

} // namespace

Json Json::makeBool(bool value) {
    Json json;
    json.type_ = Type::Bool;
    json.boolValue_ = value;
    return json;
}

Json Json::makeNumber(double value) {
    Json json;
    json.type_ = Type::Number;
    json.numberValue_ = value;
    return json;
}

Json Json::makeString(std::string value) {
    Json json;
    json.type_ = Type::String;
    json.stringValue_ = std::move(value);
    return json;
}

Json Json::makeArray() {
    Json json;
    json.type_ = Type::Array;
    return json;
}

Json Json::makeObject() {
    Json json;
    json.type_ = Type::Object;
    return json;
}

Json& Json::set(const std::string& key, Json value) {
    type_ = Type::Object;
    for (auto& [existingKey, existingValue] : objectValue_) {
        if (existingKey == key) {
            existingValue = std::move(value);
            return *this;
        }
    }
    objectValue_.emplace_back(key, std::move(value));
    return *this;
}

bool Json::has(const std::string& key) const noexcept {
    if (type_ != Type::Object) {
        return false;
    }
    for (const auto& [existingKey, existingValue] : objectValue_) {
        if (existingKey == key) {
            return true;
        }
    }
    return false;
}

const Json& Json::at(const std::string& key) const {
    if (type_ != Type::Object) {
        throw std::invalid_argument("Expected a JSON object while looking up \"" + key + "\"");
    }
    for (const auto& [existingKey, existingValue] : objectValue_) {
        if (existingKey == key) {
            return existingValue;
        }
    }
    throw std::invalid_argument("Missing required field \"" + key + "\"");
}

void Json::push(Json value) {
    type_ = Type::Array;
    arrayValue_.push_back(std::move(value));
}

bool Json::asBool() const {
    if (type_ != Type::Bool) {
        throw std::invalid_argument("Expected a JSON boolean");
    }
    return boolValue_;
}

double Json::asNumber() const {
    if (type_ != Type::Number) {
        throw std::invalid_argument("Expected a JSON number");
    }
    return numberValue_;
}

int Json::asInt() const { return static_cast<int>(std::lround(asNumber())); }

const std::string& Json::asString() const {
    if (type_ != Type::String) {
        throw std::invalid_argument("Expected a JSON string");
    }
    return stringValue_;
}

double Json::numberOr(const std::string& key, double fallback) const {
    return has(key) ? at(key).asNumber() : fallback;
}

int Json::intOr(const std::string& key, int fallback) const {
    return has(key) ? at(key).asInt() : fallback;
}

std::string Json::stringOr(const std::string& key, const std::string& fallback) const {
    return has(key) ? at(key).asString() : fallback;
}

void Json::dumpTo(std::string& out) const {
    switch (type_) {
        case Type::Null:
            out += "null";
            break;
        case Type::Bool:
            out += boolValue_ ? "true" : "false";
            break;
        case Type::Number: {
            if (std::isfinite(numberValue_) && numberValue_ == std::floor(numberValue_) &&
                std::fabs(numberValue_) < 1e15) {
                out += std::to_string(static_cast<long long>(numberValue_));
            } else {
                std::ostringstream stream;
                stream.precision(17);
                stream << numberValue_;
                out += stream.str();
            }
            break;
        }
        case Type::String:
            appendEscapedString(out, stringValue_);
            break;
        case Type::Array: {
            out += '[';
            for (std::size_t i = 0; i < arrayValue_.size(); ++i) {
                if (i != 0) out += ',';
                arrayValue_[i].dumpTo(out);
            }
            out += ']';
            break;
        }
        case Type::Object: {
            out += '{';
            for (std::size_t i = 0; i < objectValue_.size(); ++i) {
                if (i != 0) out += ',';
                appendEscapedString(out, objectValue_[i].first);
                out += ':';
                objectValue_[i].second.dumpTo(out);
            }
            out += '}';
            break;
        }
    }
}

std::string Json::dump() const {
    std::string out;
    dumpTo(out);
    return out;
}

Json Json::parse(const std::string& text) {
    Parser parser(text);
    return parser.parseDocument();
}

} // namespace polycalc::web
