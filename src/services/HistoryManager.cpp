#include "polycalc/services/HistoryManager.hpp"

#include <stdexcept>
#include <utility>

namespace polycalc {

void HistoryManager::record(const Polynomial& stateBeforeChange) {
    undoStack_.push_back(stateBeforeChange);
    redoStack_.clear();
}

bool HistoryManager::canUndo() const noexcept {
    return !undoStack_.empty();
}

bool HistoryManager::canRedo() const noexcept {
    return !redoStack_.empty();
}

Polynomial HistoryManager::undo(const Polynomial& currentState) {
    if (!canUndo()) {
        throw std::logic_error("No operation to undo");
    }
    Polynomial previous = std::move(undoStack_.back());
    undoStack_.pop_back();
    redoStack_.push_back(currentState);
    return previous;
}

Polynomial HistoryManager::redo(const Polynomial& currentState) {
    if (!canRedo()) {
        throw std::logic_error("No operation to redo");
    }
    Polynomial next = std::move(redoStack_.back());
    redoStack_.pop_back();
    undoStack_.push_back(currentState);
    return next;
}

void HistoryManager::clear() noexcept {
    undoStack_.clear();
    redoStack_.clear();
}

std::size_t HistoryManager::undoDepth() const noexcept {
    return undoStack_.size();
}

std::size_t HistoryManager::redoDepth() const noexcept {
    return redoStack_.size();
}

} // namespace polycalc
