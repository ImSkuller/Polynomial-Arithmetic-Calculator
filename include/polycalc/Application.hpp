#pragma once

#include "polycalc/core/Polynomial.hpp"
#include "polycalc/services/HistoryManager.hpp"

namespace polycalc {

// Owns the interactive session: the current (and, for binary operations, a
// secondary) polynomial, undo/redo history, and the top-level menu loop.
// All menu handlers catch and report exceptions from lower layers (parser,
// storage, Polynomial's own validation) so malformed input never crashes
// the session.
class Application {
public:
    int run();

private:
    Polynomial current_;
    Polynomial secondary_;
    HistoryManager history_;
    bool exitRequested_ = false;

    void showMainMenu();
    void handleBuildAndEdit();
    void handleViewAndAnalyze();
    void handleArithmetic();
    void handleHistory();
    void handleSaveAndLoad();
    void handleRandomGenerator();
    void handleStatistics();

    // Pushes `before` onto the undo history only if the operation that just
    // ran actually changed the polynomial, so undo/redo never accumulates
    // no-op steps (e.g. deleting an exponent that was never present).
    void recordIfChanged(const Polynomial& before, bool changed);
    void printCurrentStatus();
};

} // namespace polycalc
