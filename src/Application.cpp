#include "polycalc/Application.hpp"

#include <utility>

#include "polycalc/core/Node.hpp"
#include "polycalc/core/PolynomialParser.hpp"
#include "polycalc/services/PolynomialStorage.hpp"
#include "polycalc/services/Timer.hpp"

namespace polycalc {

void Application::recordIfChanged(const Polynomial& before, bool changed) {
    if (changed) {
        history_.record(before);
    }
}

std::string Application::degreeText() const {
    return current_.isZero() ? "-" : std::to_string(current_.degree());
}

void Application::setCurrentFromExpression(const std::string& expression) {
    Polynomial parsed = PolynomialParser::parse(expression);
    const Polynomial before = current_;
    const bool changed = !(parsed == current_);
    current_ = std::move(parsed);
    recordIfChanged(before, changed);
}

bool Application::insertTerm(double coefficient, int exponent) {
    const Polynomial before = current_;
    const bool changed = current_.insertTerm(coefficient, exponent);
    recordIfChanged(before, changed);
    return changed;
}

bool Application::deleteTerm(int exponent) {
    const Polynomial before = current_;
    const bool changed = current_.deleteTerm(exponent);
    recordIfChanged(before, changed);
    return changed;
}

bool Application::updateCoefficient(int exponent, double newCoefficient) {
    const Polynomial before = current_;
    const bool changed = current_.updateCoefficient(exponent, newCoefficient);
    recordIfChanged(before, changed);
    return changed;
}

bool Application::updateExponent(int oldExponent, int newExponent) {
    const Polynomial before = current_;
    const bool changed = current_.updateExponent(oldExponent, newExponent);
    recordIfChanged(before, changed);
    return changed;
}

void Application::clearCurrent() {
    const Polynomial before = current_;
    const bool changed = !current_.isZero();
    current_.clear();
    recordIfChanged(before, changed);
}

double Application::evaluate(double x) const {
    return current_.evaluate(x);
}

double Application::sortByExponent() {
    return Timer::measureMilliseconds([this] { current_.sortByExponent(); });
}

double Application::mergeLikeTerms() {
    return Timer::measureMilliseconds([this] { current_.mergeLikeTerms(); });
}

double Application::simplify() {
    return Timer::measureMilliseconds([this] { current_.simplify(); });
}

void Application::setSecondaryFromExpression(const std::string& expression) {
    secondary_ = PolynomialParser::parse(expression);
}

Polynomial Application::add() const {
    return current_ + secondary_;
}

Polynomial Application::subtract() const {
    return current_ - secondary_;
}

Polynomial Application::multiply() const {
    return current_ * secondary_;
}

void Application::adoptAsCurrent(Polynomial result) {
    const Polynomial before = current_;
    current_ = std::move(result);
    recordIfChanged(before, true);
}

bool Application::canUndo() const noexcept {
    return history_.canUndo();
}

bool Application::canRedo() const noexcept {
    return history_.canRedo();
}

std::size_t Application::undoDepth() const noexcept {
    return history_.undoDepth();
}

std::size_t Application::redoDepth() const noexcept {
    return history_.redoDepth();
}

void Application::undo() {
    current_ = history_.undo(current_);
}

void Application::redo() {
    current_ = history_.redo(current_);
}

void Application::saveToFile(const std::string& path) const {
    PolynomialStorage::save(current_, path);
}

void Application::loadFromFile(const std::string& path) {
    Polynomial loaded = PolynomialStorage::load(path);
    const Polynomial before = current_;
    current_ = std::move(loaded);
    recordIfChanged(before, true);
}

void Application::generateRandom(const PolynomialGenerator::Options& options) {
    Polynomial generated = PolynomialGenerator::generate(options);
    const Polynomial before = current_;
    current_ = std::move(generated);
    recordIfChanged(before, true);
}

Statistics Application::statistics() const {
    Statistics stats;
    stats.termCount = current_.termCount();
    stats.degree = current_.isZero() ? -1 : current_.degree();
    stats.memoryBytes = current_.termCount() * sizeof(Node);

    {
        Polynomial scratch = current_;
        stats.sortMs = Timer::measureMilliseconds([&scratch] { scratch.sortByExponent(); });
    }
    {
        Polynomial scratch = current_;
        stats.mergeMs = Timer::measureMilliseconds([&scratch] { scratch.mergeLikeTerms(); });
    }
    {
        Polynomial scratch = current_;
        stats.simplifyMs = Timer::measureMilliseconds([&scratch] { scratch.simplify(); });
    }
    stats.evaluateMs = Timer::measureMilliseconds([this] { current_.evaluate(2.0); });

    return stats;
}

} // namespace polycalc
