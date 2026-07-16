#include "polycalc/core/Polynomial.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>

#include "polycalc/core/Formatting.hpp"

namespace polycalc {

Polynomial::Polynomial(const Polynomial& other) {
    Node* previous = nullptr;
    try {
        for (Node* current = other.head_; current != nullptr; current = current->next) {
            Node* copy = new Node(current->coefficient, current->exponent);
            if (previous == nullptr) {
                head_ = copy;
            } else {
                previous->next = copy;
            }
            previous = copy;
        }
    } catch (...) {
        clear();
        throw;
    }
}

Polynomial::Polynomial(Polynomial&& other) noexcept : head_(other.head_) {
    other.head_ = nullptr;
}

Polynomial& Polynomial::operator=(const Polynomial& other) {
    if (this != &other) {
        Polynomial temporary(other);
        swap(temporary);
    }
    return *this;
}

Polynomial& Polynomial::operator=(Polynomial&& other) noexcept {
    if (this != &other) {
        clear();
        head_ = other.head_;
        other.head_ = nullptr;
    }
    return *this;
}

Polynomial::~Polynomial() {
    clear();
}

void Polynomial::swap(Polynomial& other) noexcept {
    std::swap(head_, other.head_);
}

bool Polynomial::insertTerm(double coefficient, int exponent) {
    if (exponent < 0) {
        throw std::invalid_argument("Polynomial term exponent must be non-negative");
    }
    if (std::fabs(coefficient) < kEpsilon) {
        return false;
    }

    Node* previous = nullptr;
    Node* current = head_;
    while (current != nullptr && current->exponent != exponent) {
        previous = current;
        current = current->next;
    }

    if (current != nullptr) {
        const double merged = current->coefficient + coefficient;
        if (std::fabs(merged) < kEpsilon) {
            if (previous == nullptr) {
                head_ = current->next;
            } else {
                previous->next = current->next;
            }
            delete current;
        } else {
            current->coefficient = merged;
        }
        return true;
    }

    head_ = new Node(coefficient, exponent, head_);
    return true;
}

void Polynomial::appendTermRaw(double coefficient, int exponent) {
    if (exponent < 0) {
        throw std::invalid_argument("Polynomial term exponent must be non-negative");
    }
    if (std::fabs(coefficient) < kEpsilon) {
        return;
    }
    head_ = new Node(coefficient, exponent, head_);
}

bool Polynomial::deleteTerm(int exponent) {
    Node* previous = nullptr;
    Node* current = head_;
    while (current != nullptr && current->exponent != exponent) {
        previous = current;
        current = current->next;
    }
    if (current == nullptr) {
        return false;
    }
    if (previous == nullptr) {
        head_ = current->next;
    } else {
        previous->next = current->next;
    }
    delete current;
    return true;
}

bool Polynomial::updateCoefficient(int exponent, double newCoefficient) {
    Node* previous = nullptr;
    Node* current = head_;
    while (current != nullptr && current->exponent != exponent) {
        previous = current;
        current = current->next;
    }
    if (current == nullptr) {
        return false;
    }
    if (std::fabs(newCoefficient) < kEpsilon) {
        if (previous == nullptr) {
            head_ = current->next;
        } else {
            previous->next = current->next;
        }
        delete current;
    } else {
        current->coefficient = newCoefficient;
    }
    return true;
}

bool Polynomial::updateExponent(int oldExponent, int newExponent) {
    if (newExponent < 0) {
        throw std::invalid_argument("Polynomial term exponent must be non-negative");
    }

    Node* previous = nullptr;
    Node* current = head_;
    while (current != nullptr && current->exponent != oldExponent) {
        previous = current;
        current = current->next;
    }
    if (current == nullptr) {
        return false;
    }

    const double coefficient = current->coefficient;
    if (previous == nullptr) {
        head_ = current->next;
    } else {
        previous->next = current->next;
    }
    delete current;

    insertTerm(coefficient, newExponent);
    return true;
}

void Polynomial::clear() noexcept {
    while (head_ != nullptr) {
        Node* next = head_->next;
        delete head_;
        head_ = next;
    }
}

bool Polynomial::isZero() const noexcept {
    return head_ == nullptr;
}

int Polynomial::degree() const noexcept {
    if (head_ == nullptr) {
        return -1;
    }
    int maxExponent = head_->exponent;
    for (Node* current = head_->next; current != nullptr; current = current->next) {
        if (current->exponent > maxExponent) {
            maxExponent = current->exponent;
        }
    }
    return maxExponent;
}

std::size_t Polynomial::termCount() const noexcept {
    std::size_t count = 0;
    for (Node* current = head_; current != nullptr; current = current->next) {
        ++count;
    }
    return count;
}

double Polynomial::evaluate(double x) const {
    double result = 0.0;
    for (Node* current = head_; current != nullptr; current = current->next) {
        result += current->coefficient * std::pow(x, current->exponent);
    }
    return result;
}

void Polynomial::pruneZeroCoefficients() noexcept {
    while (head_ != nullptr && std::fabs(head_->coefficient) < kEpsilon) {
        Node* toDelete = head_;
        head_ = head_->next;
        delete toDelete;
    }
    if (head_ == nullptr) {
        return;
    }
    Node* current = head_;
    while (current->next != nullptr) {
        if (std::fabs(current->next->coefficient) < kEpsilon) {
            Node* toDelete = current->next;
            current->next = toDelete->next;
            delete toDelete;
        } else {
            current = current->next;
        }
    }
}

Node* Polynomial::splitInHalf(Node* head) {
    Node* slow = head;
    Node* fast = head->next;
    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;
        fast = fast->next->next;
    }
    Node* secondHalf = slow->next;
    slow->next = nullptr;
    return secondHalf;
}

Node* Polynomial::mergeTwoSorted(Node* lhs, Node* rhs) {
    Node dummy(0.0, 0);
    Node* tail = &dummy;
    while (lhs != nullptr && rhs != nullptr) {
        if (lhs->exponent >= rhs->exponent) {
            tail->next = lhs;
            lhs = lhs->next;
        } else {
            tail->next = rhs;
            rhs = rhs->next;
        }
        tail = tail->next;
    }
    tail->next = (lhs != nullptr) ? lhs : rhs;
    return dummy.next;
}

Node* Polynomial::mergeSortRecursive(Node* head) {
    if (head == nullptr || head->next == nullptr) {
        return head;
    }
    Node* secondHalf = splitInHalf(head);
    Node* left = mergeSortRecursive(head);
    Node* right = mergeSortRecursive(secondHalf);
    return mergeTwoSorted(left, right);
}

void Polynomial::sortByExponent() {
    head_ = mergeSortRecursive(head_);
}

void Polynomial::mergeLikeTerms() {
    sortByExponent();
    Node* current = head_;
    while (current != nullptr && current->next != nullptr) {
        if (current->exponent == current->next->exponent) {
            current->coefficient += current->next->coefficient;
            Node* duplicate = current->next;
            current->next = duplicate->next;
            delete duplicate;
        } else {
            current = current->next;
        }
    }
    pruneZeroCoefficients();
}

void Polynomial::simplify() {
    // Simplifying a polynomial is exactly: canonicalize the term order and
    // combine duplicates, dropping anything that cancels to zero.
    mergeLikeTerms();
}

Polynomial Polynomial::operator+(const Polynomial& rhs) const {
    // Normalize local copies first: *this or rhs may currently hold
    // unmerged duplicates or be in arbitrary order (e.g. freshly parsed
    // input), and the merge below requires two sorted, duplicate-free lists.
    Polynomial lhs(*this);
    Polynomial other(rhs);
    lhs.mergeLikeTerms();
    other.mergeLikeTerms();

    Polynomial result;
    Node* tail = nullptr;
    const auto append = [&](double coefficient, int exponent) {
        if (std::fabs(coefficient) < kEpsilon) {
            return;
        }
        Node* node = new Node(coefficient, exponent);
        if (tail == nullptr) {
            result.head_ = node;
        } else {
            tail->next = node;
        }
        tail = node;
    };

    const Node* a = lhs.head_;
    const Node* b = other.head_;
    while (a != nullptr && b != nullptr) {
        if (a->exponent == b->exponent) {
            append(a->coefficient + b->coefficient, a->exponent);
            a = a->next;
            b = b->next;
        } else if (a->exponent > b->exponent) {
            append(a->coefficient, a->exponent);
            a = a->next;
        } else {
            append(b->coefficient, b->exponent);
            b = b->next;
        }
    }
    for (; a != nullptr; a = a->next) {
        append(a->coefficient, a->exponent);
    }
    for (; b != nullptr; b = b->next) {
        append(b->coefficient, b->exponent);
    }
    return result;
}

Polynomial Polynomial::operator-() const {
    Polynomial result(*this);
    for (Node* current = result.head_; current != nullptr; current = current->next) {
        current->coefficient = -current->coefficient;
    }
    return result;
}

Polynomial Polynomial::operator-(const Polynomial& rhs) const {
    return *this + (-rhs);
}

Polynomial Polynomial::operator*(const Polynomial& rhs) const {
    // Correct regardless of input order/duplicates: every pair of terms is
    // visited exactly once, then a single mergeLikeTerms() pass combines
    // whatever exponent collisions the products produced.
    Polynomial result;
    for (Node* a = head_; a != nullptr; a = a->next) {
        for (Node* b = rhs.head_; b != nullptr; b = b->next) {
            result.appendTermRaw(a->coefficient * b->coefficient, a->exponent + b->exponent);
        }
    }
    result.mergeLikeTerms();
    return result;
}

Polynomial& Polynomial::operator+=(const Polynomial& rhs) {
    *this = *this + rhs;
    return *this;
}

Polynomial& Polynomial::operator-=(const Polynomial& rhs) {
    *this = *this - rhs;
    return *this;
}

Polynomial& Polynomial::operator*=(const Polynomial& rhs) {
    *this = *this * rhs;
    return *this;
}

bool Polynomial::operator==(const Polynomial& rhs) const {
    // Order and duplicate exponents in either operand must not affect
    // equality, so compare normalized copies rather than the raw lists.
    Polynomial lhs(*this);
    Polynomial other(rhs);
    lhs.mergeLikeTerms();
    other.mergeLikeTerms();

    Node* a = lhs.head_;
    Node* b = other.head_;
    while (a != nullptr && b != nullptr) {
        if (a->exponent != b->exponent || std::fabs(a->coefficient - b->coefficient) >= kEpsilon) {
            return false;
        }
        a = a->next;
        b = b->next;
    }
    return a == nullptr && b == nullptr;
}

bool Polynomial::operator!=(const Polynomial& rhs) const {
    return !(*this == rhs);
}

std::string Polynomial::toString() const {
    if (head_ == nullptr) {
        return "0";
    }

    std::ostringstream stream;
    bool first = true;
    for (Node* current = head_; current != nullptr; current = current->next) {
        const double coefficient = current->coefficient;
        const int exponent = current->exponent;
        const double magnitude = std::fabs(coefficient);

        if (first) {
            if (coefficient < 0) {
                stream << "-";
            }
        } else {
            stream << (coefficient < 0 ? " - " : " + ");
        }

        const bool isUnitMagnitude = std::fabs(magnitude - 1.0) < kEpsilon;
        if (!isUnitMagnitude || exponent == 0) {
            stream << formatCoefficient(magnitude);
        }

        if (exponent > 0) {
            stream << "x";
            if (exponent != 1) {
                stream << toSuperscript(exponent);
            }
        }
        first = false;
    }
    return stream.str();
}

std::ostream& operator<<(std::ostream& os, const Polynomial& polynomial) {
    return os << polynomial.toString();
}

std::vector<std::pair<double, int>> Polynomial::terms() const {
    std::vector<std::pair<double, int>> result;
    result.reserve(termCount());
    for (Node* current = head_; current != nullptr; current = current->next) {
        result.emplace_back(current->coefficient, current->exponent);
    }
    return result;
}

} // namespace polycalc
