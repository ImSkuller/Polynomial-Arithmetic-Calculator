#pragma once

#include <cstddef>
#include <vector>

#include "polycalc/core/Polynomial.hpp"

namespace polycalc {

// Tracks snapshots of a Polynomial so edits can be undone and redone.
// Call record() with the state immediately *before* a mutation; undo()/redo()
// hand back the polynomial the caller should adopt as its new current state.
class HistoryManager {
public:
    void record(const Polynomial& stateBeforeChange);

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    // Throws std::logic_error if canUndo()/canRedo() is false.
    Polynomial undo(const Polynomial& currentState);
    Polynomial redo(const Polynomial& currentState);

    void clear() noexcept;
    std::size_t undoDepth() const noexcept;
    std::size_t redoDepth() const noexcept;

private:
    std::vector<Polynomial> undoStack_;
    std::vector<Polynomial> redoStack_;
};

} // namespace polycalc
