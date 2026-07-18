#pragma once

#include <cstddef>
#include <string>

#include "polycalc/core/Polynomial.hpp"
#include "polycalc/services/HistoryManager.hpp"
#include "polycalc/services/PolynomialGenerator.hpp"

namespace polycalc {

// A snapshot of the current polynomial's shape and the cost of the core
// operations, all measured live against a scratch copy so the numbers never
// depend on how the polynomial happens to already be ordered.
struct Statistics {
    std::size_t termCount = 0;
    int degree = -1;
    std::size_t memoryBytes = 0;
    double sortMs = 0.0;
    double mergeMs = 0.0;
    double simplifyMs = 0.0;
    double evaluateMs = 0.0;
};

// Owns the session state - the working polynomial ("current"), a secondary
// polynomial used only for binary arithmetic, and undo/redo history -
// independent of any particular user interface. Every mutating method
// throws (propagated from Polynomial, PolynomialParser, or
// PolynomialStorage) instead of reporting errors itself, so the UI layer
// decides how to present them; none of it touches a console or a window.
class Application {
public:
    const Polynomial& current() const noexcept { return current_; }
    const Polynomial& secondary() const noexcept { return secondary_; }

    std::string currentText() const { return current_.toString(); }
    std::string secondaryText() const { return secondary_.toString(); }
    std::string degreeText() const;
    std::size_t termCount() const noexcept { return current_.termCount(); }

    // Replaces the current polynomial with the parsed expression. Throws
    // std::invalid_argument if the expression is malformed.
    void setCurrentFromExpression(const std::string& expression);

    // Each returns true if the polynomial's state actually changed, and
    // records the pre-change state for undo whenever it does.
    bool insertTerm(double coefficient, int exponent);
    bool deleteTerm(int exponent);
    bool updateCoefficient(int exponent, double newCoefficient);
    bool updateExponent(int oldExponent, int newExponent);
    void clearCurrent();

    double evaluate(double x) const;

    // Each performs the named operation on the current polynomial in place
    // and returns how long it took, in milliseconds.
    double sortByExponent();
    double mergeLikeTerms();
    double simplify();

    // Throws std::invalid_argument if the expression is malformed.
    void setSecondaryFromExpression(const std::string& expression);
    Polynomial add() const;
    Polynomial subtract() const;
    Polynomial multiply() const;
    // Adopts the given result (typically from add()/subtract()/multiply())
    // as the new current polynomial, recording undo history.
    void adoptAsCurrent(Polynomial result);

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    std::size_t undoDepth() const noexcept;
    std::size_t redoDepth() const noexcept;
    void undo();
    void redo();

    // Throws std::runtime_error on I/O failure.
    void saveToFile(const std::string& path) const;
    // Throws std::runtime_error on I/O failure, or std::invalid_argument if
    // the file's contents are malformed.
    void loadFromFile(const std::string& path);

    // Throws std::invalid_argument if the options describe an empty or
    // inverted range.
    void generateRandom(const PolynomialGenerator::Options& options);

    Statistics statistics() const;

private:
    Polynomial current_;
    Polynomial secondary_;
    HistoryManager history_;

    // Pushes `before` onto the undo history only if the operation that just
    // ran actually changed the polynomial, so undo/redo never accumulates
    // no-op steps (e.g. deleting an exponent that was never present).
    void recordIfChanged(const Polynomial& before, bool changed);
};

} // namespace polycalc
