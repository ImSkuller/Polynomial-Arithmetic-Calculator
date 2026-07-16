#pragma once

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace polycalc::test {

// Minimal self-registering test framework. Kept dependency-free so the
// project builds and tests offline with nothing beyond the compiler.
class TestRegistry {
public:
    using TestCase = std::function<void()>;

    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void add(std::string name, TestCase test) {
        cases_.emplace_back(std::move(name), std::move(test));
    }

    int runAll() {
        int failures = 0;
        for (const auto& [name, test] : cases_) {
            try {
                test();
                std::cout << "  [ PASS ] " << name << '\n';
            } catch (const std::exception& ex) {
                std::cout << "  [ FAIL ] " << name << " -> " << ex.what() << '\n';
                ++failures;
            }
        }
        const auto total = cases_.size();
        std::cout << '\n' << (total - static_cast<std::size_t>(failures)) << '/' << total
                  << " tests passed\n";
        return failures;
    }

private:
    std::vector<std::pair<std::string, TestCase>> cases_;
};

struct Registrar {
    Registrar(const std::string& name, TestRegistry::TestCase test) {
        TestRegistry::instance().add(name, std::move(test));
    }
};

} // namespace polycalc::test

#define POLYCALC_CONCAT_INNER(a, b) a##b
#define POLYCALC_CONCAT(a, b) POLYCALC_CONCAT_INNER(a, b)

#define TEST_CASE(name)                                                                \
    static void POLYCALC_CONCAT(polycalc_test_fn_, __LINE__)();                        \
    static const polycalc::test::Registrar POLYCALC_CONCAT(polycalc_registrar_,        \
                                                             __LINE__)(                 \
        name, POLYCALC_CONCAT(polycalc_test_fn_, __LINE__));                           \
    static void POLYCALC_CONCAT(polycalc_test_fn_, __LINE__)()

#define REQUIRE(condition)                                                             \
    do {                                                                               \
        if (!(condition)) {                                                           \
            throw std::runtime_error(std::string("REQUIRE failed: ") + #condition +   \
                                      " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        }                                                                              \
    } while (false)

#define REQUIRE_EQ(actual, expected)                                                   \
    do {                                                                               \
        if (!((actual) == (expected))) {                                              \
            throw std::runtime_error(std::string("REQUIRE_EQ failed: ") + #actual +    \
                                      " == " + #expected + " at " + __FILE__ + ":" +    \
                                      std::to_string(__LINE__));                        \
        }                                                                              \
    } while (false)
