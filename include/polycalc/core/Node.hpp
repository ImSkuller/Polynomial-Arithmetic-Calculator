#pragma once

namespace polycalc {

// A single term of a polynomial: coefficient * x^exponent.
struct Node {
    double coefficient;
    int exponent;
    Node* next;

    Node(double coefficientValue, int exponentValue, Node* nextNode = nullptr)
        : coefficient(coefficientValue), exponent(exponentValue), next(nextNode) {}
};

} // namespace polycalc
