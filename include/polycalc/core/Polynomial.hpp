#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "polycalc/core/Node.hpp"

namespace polycalc {

// A polynomial represented as a singly linked list of terms.
//
// insertTerm() guarantees no two nodes share an exponent (merging into an
// existing term, or removing it if the merge cancels to zero) but does not
// maintain any particular node order - a new, non-duplicate term is simply
// prepended. appendTermRaw() is more primitive still: it appends a node at
// the tail unconditionally, without checking for an existing same-exponent term, so
// duplicate exponents and arbitrary order can arise. That is exactly what
// the expression parser and file loader use, so that parsing
// "3x^2 + 4x^2" faithfully preserves both terms until the user explicitly
// merges them - mirroring how a student would work through the algorithm by
// hand. sortByExponent(), mergeLikeTerms(), and simplify() are the
// operations that restore canonical (descending-exponent, duplicate-free,
// zero-free) form. operator+ and operator- normalize their operands
// internally first, so addition and subtraction are correct regardless of a
// polynomial's current order or duplicate state.
class Polynomial {
public:
    static constexpr double kEpsilon = 1e-9;

    Polynomial() = default;
    Polynomial(const Polynomial& other);
    Polynomial(Polynomial&& other) noexcept;
    Polynomial& operator=(const Polynomial& other);
    Polynomial& operator=(Polynomial&& other) noexcept;
    ~Polynomial();

    void swap(Polynomial& other) noexcept;

    // Inserts coefficient*x^exponent, merging into an existing same-exponent
    // term if one exists anywhere in the list (removing it if the merge
    // cancels to zero), otherwise prepending a new node. Returns true if the
    // polynomial's state changed. Throws std::invalid_argument if exponent
    // is negative.
    bool insertTerm(double coefficient, int exponent);

    // Unconditionally appends a new node for coefficient*x^exponent at the
    // tail, even if a term with the same exponent already exists. Used
    // internally by the parser and file loader to preserve input order
    // exactly as given; prefer insertTerm() for ordinary single-term edits.
    // Throws std::invalid_argument if exponent is negative. A zero
    // coefficient is silently ignored, since a zero term is not a term.
    void appendTermRaw(double coefficient, int exponent);

    // Removes the first term found with the given exponent. Returns true if
    // it existed.
    bool deleteTerm(int exponent);

    // Sets the coefficient of the first existing term found with the given
    // exponent (removing it if the new coefficient is zero). Returns true if
    // the term existed.
    bool updateCoefficient(int exponent, double newCoefficient);

    // Moves the first existing term found at oldExponent to newExponent,
    // merging if a term already occupies it. Returns true if the term
    // existed. Throws std::invalid_argument if newExponent is negative.
    bool updateExponent(int oldExponent, int newExponent);

    void clear() noexcept;
    bool isZero() const noexcept;

    // -1 for the zero polynomial, otherwise the highest exponent present.
    int degree() const noexcept;
    std::size_t termCount() const noexcept;

    double evaluate(double x) const;

    void sortByExponent();
    void mergeLikeTerms();
    void simplify();

    Polynomial operator+(const Polynomial& rhs) const;
    Polynomial operator-(const Polynomial& rhs) const;
    Polynomial operator-() const;
    Polynomial operator*(const Polynomial& rhs) const;

    Polynomial& operator+=(const Polynomial& rhs);
    Polynomial& operator-=(const Polynomial& rhs);
    Polynomial& operator*=(const Polynomial& rhs);

    // Compares polynomials by mathematical value: order and duplicate
    // exponents in either operand do not affect the result.
    bool operator==(const Polynomial& rhs) const;
    bool operator!=(const Polynomial& rhs) const;

    // Renders terms in the polynomial's current internal order (which may
    // contain unmerged duplicates if it has not been simplified).
    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os, const Polynomial& polynomial);

    // Read-only snapshot of (coefficient, exponent) pairs in current order.
    std::vector<std::pair<double, int>> terms() const;

private:
    Node* head_ = nullptr;

    void pruneZeroCoefficients() noexcept;

    // Merge-sort helpers used by sortByExponent(); relink existing nodes,
    // they never allocate.
    static Node* mergeSortRecursive(Node* head);
    static Node* splitInHalf(Node* head);
    static Node* mergeTwoSorted(Node* lhs, Node* rhs);
};

} // namespace polycalc
