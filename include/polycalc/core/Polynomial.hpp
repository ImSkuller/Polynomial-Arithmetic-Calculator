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
// Invariant maintained by every public mutator: nodes are kept sorted by
// strictly descending exponent, no two nodes share an exponent, and no node
// holds a coefficient of (approximately) zero. This invariant is what lets
// degree() run in O(1), lets equality and display walk the list directly
// without normalizing first, and lets addition be implemented as a single
// merge pass over two already-sorted lists (mirroring the merge step of
// merge sort). multiply() is the one operation that temporarily breaks the
// invariant (it builds an unsorted, unmerged product list for speed) before
// restoring it via mergeLikeTerms().
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
    // term if present (removing it if the merge cancels to zero). Returns
    // true if the polynomial's state changed. Throws std::invalid_argument
    // if exponent is negative.
    bool insertTerm(double coefficient, int exponent);

    // Removes the term with the given exponent. Returns true if it existed.
    bool deleteTerm(int exponent);

    // Sets the coefficient of an existing term (removing it if the new
    // coefficient is zero). Returns true if the term existed.
    bool updateCoefficient(int exponent, double newCoefficient);

    // Moves an existing term to a new exponent, merging if one already
    // occupies it. Returns true if the term existed. Throws
    // std::invalid_argument if newExponent is negative.
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

    bool operator==(const Polynomial& rhs) const noexcept;
    bool operator!=(const Polynomial& rhs) const noexcept;

    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os, const Polynomial& polynomial);

    // Read-only snapshot of (coefficient, exponent) pairs in current order.
    std::vector<std::pair<double, int>> terms() const;

private:
    Node* head_ = nullptr;

    void pushFront(double coefficient, int exponent);
    void pruneZeroCoefficients() noexcept;

    // Merge-sort helpers used by sortByExponent(); relink existing nodes,
    // they never allocate.
    static Node* mergeSortRecursive(Node* head);
    static Node* splitInHalf(Node* head);
    static Node* mergeTwoSorted(Node* lhs, Node* rhs);
};

} // namespace polycalc
