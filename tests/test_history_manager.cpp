#include "TestFramework.hpp"
#include "polycalc/services/HistoryManager.hpp"

using polycalc::HistoryManager;
using polycalc::Polynomial;

namespace {

Polynomial make(double coefficient, int exponent) {
    Polynomial p;
    p.insertTerm(coefficient, exponent);
    return p;
}

} // namespace

TEST_CASE("fresh history cannot undo or redo") {
    HistoryManager history;
    REQUIRE(!history.canUndo());
    REQUIRE(!history.canRedo());
}

TEST_CASE("undo restores the previously recorded state") {
    HistoryManager history;
    Polynomial before = make(3, 1);
    Polynomial after = make(3, 1);
    after.insertTerm(5, 2);

    history.record(before);
    REQUIRE(history.canUndo());

    Polynomial restored = history.undo(after);
    REQUIRE(restored == before);
    REQUIRE(history.canRedo());
}

TEST_CASE("redo re-applies the state that was undone") {
    HistoryManager history;
    Polynomial before = make(3, 1);
    Polynomial after = make(7, 2);

    history.record(before);
    Polynomial restored = history.undo(after);
    Polynomial redone = history.redo(restored);

    REQUIRE(redone == after);
    REQUIRE(!history.canRedo());
}

TEST_CASE("a new record after undo clears the redo stack") {
    HistoryManager history;
    Polynomial state1 = make(1, 0);
    Polynomial state2 = make(2, 0);
    Polynomial state3 = make(3, 0);

    history.record(state1);
    history.undo(state2);
    REQUIRE(history.canRedo());

    history.record(state2);
    REQUIRE(!history.canRedo());
}

TEST_CASE("undo on an empty history throws") {
    HistoryManager history;
    bool threw = false;
    try {
        history.undo(make(1, 0));
    } catch (const std::logic_error&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("redo on an empty history throws") {
    HistoryManager history;
    bool threw = false;
    try {
        history.redo(make(1, 0));
    } catch (const std::logic_error&) {
        threw = true;
    }
    REQUIRE(threw);
}

TEST_CASE("multiple undo and redo steps traverse history in order") {
    HistoryManager history;
    Polynomial a = make(1, 0);
    Polynomial b = make(2, 0);
    Polynomial c = make(3, 0);

    history.record(a);
    history.record(b);
    REQUIRE_EQ(history.undoDepth(), 2u);

    Polynomial afterFirstUndo = history.undo(c);
    REQUIRE(afterFirstUndo == b);

    Polynomial afterSecondUndo = history.undo(afterFirstUndo);
    REQUIRE(afterSecondUndo == a);
    REQUIRE(!history.canUndo());
    REQUIRE_EQ(history.redoDepth(), 2u);

    Polynomial afterFirstRedo = history.redo(afterSecondUndo);
    REQUIRE(afterFirstRedo == b);

    Polynomial afterSecondRedo = history.redo(afterFirstRedo);
    REQUIRE(afterSecondRedo == c);
    REQUIRE(!history.canRedo());
}

TEST_CASE("clear resets both stacks") {
    HistoryManager history;
    history.record(make(1, 0));
    history.undo(make(2, 0));
    history.clear();
    REQUIRE(!history.canUndo());
    REQUIRE(!history.canRedo());
}
