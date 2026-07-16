# Polynomial Arithmetic Calculator

A modern C++17 console application for building, editing, and computing with
polynomials, backed by a singly linked list. Built as an educational
mathematics tool that also demonstrates production-quality C++ practices:
RAII, the rule of five, exception safety, and a clean modular architecture.

```
╭──────────────────────────────────────╮
│   Polynomial Arithmetic Calculator   │
│                v1.0.0                │
╰──────────────────────────────────────╯

╭───────────────────────╮
│ Current Polynomial    │
├───────────────────────┤
│ P(x) = 5x⁴ + 3x² - 7  │
│ Degree: 4    Terms: 3 │
╰───────────────────────╯
╭─────────────────────────────╮
│ Main Menu                   │
├─────────────────────────────┤
│ 1. Build & Edit Polynomial  │
│ 2. View & Analyze           │
│ 3. Arithmetic Operations    │
│ 4. History (Undo / Redo)    │
│ 5. Save & Load              │
│ 6. Random Generator         │
│ 7. Statistics & Performance │
│ 8. Exit                     │
╰─────────────────────────────╯
❯ Select an option (1-8):
```

Colored output and Unicode box-drawing are used where the terminal supports
them, with an automatic plain-text fallback otherwise (see [assets/](assets/)
for where to add real screenshots).

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Folder Structure](#folder-structure)
- [Building](#building)
- [Running](#running)
- [Running the Tests](#running-the-tests)
- [How the Linked List Works](#how-the-linked-list-works)
- [Algorithms Used](#algorithms-used)
- [Complexity Reference](#complexity-reference)
- [Design Notes and Trade-offs](#design-notes-and-trade-offs)
- [Future Improvements](#future-improvements)

## Features

### Core

- Create a polynomial by typing an expression (`3x^2 + 4x - 8`)
- Insert, delete, update the coefficient of, or update the exponent of a term
- Display in proper mathematical notation (`5x⁴ + 3x² - 7`, not `5x4+3x2-7`)
- Addition, subtraction, multiplication
- Merge like terms, sort by descending exponent, simplify
- Evaluate at a given `x`
- Degree, term count, clear, deep copy, and a leak-free destructor

### Bonus

- File-based save/load (`PolynomialStorage`)
- Undo/redo history (`HistoryManager`)
- Expression parser with Unicode-superscript round-tripping
- Random polynomial generator with configurable bounds
- Millisecond-precision performance timer (`Timer`), used live in the
  Statistics screen and when running sort/merge/simplify from the menu
- A statistics screen: term count, degree, an estimated memory footprint, and
  timings for the operations above

## Architecture

```
Node               - a single term: coefficient, exponent, next pointer
Polynomial         - singly linked list of Node; all arithmetic, editing,
                     comparison, and formatting logic
PolynomialParser    - "3x^2 + 4x - 8" -> Polynomial
PolynomialStorage   - Polynomial <-> text file
PolynomialGenerator - randomized Polynomial for demos/timing
HistoryManager      - undo/redo snapshot stacks
Timer               - millisecond stopwatch, also usable as Timer::measureMilliseconds(fn)
Terminal / ConsoleUI - ANSI color detection, box/table/menu rendering, input prompts
Application         - owns the session state and the menu loop
```

Each class has one responsibility: `Polynomial` never touches `std::cout`,
`ConsoleUI` never touches polynomial math, and `Application` only
orchestrates the two. This keeps the math layer trivially unit-testable
(see [tests/](tests/)) independent of any console/formatting concerns.

## Folder Structure

```
include/polycalc/         Public headers, mirroring src/
  core/                   Node, Polynomial, Formatting, PolynomialParser
  services/               Timer, HistoryManager, PolynomialGenerator, PolynomialStorage
  console/                Terminal (color), ConsoleUI (layout/input)
  Application.hpp         Session/menu orchestration
  Version.hpp             App name/version constants
src/                      Implementation, same layout as include/
  main.cpp                Entry point
tests/                    Custom lightweight test framework + test suites
docs/                     Developer documentation
assets/                   Where to add real terminal screenshots
CMakeLists.txt            Top-level build configuration
```

Every module is a `.hpp`/`.cpp` pair under its own subdirectory - nothing is
dumped into a single file.

## Building

Requires a C++17 compiler and CMake 3.20+. Tested with MSVC 19.44 (Visual
Studio 2022 Build Tools) and Ninja; any C++17-capable compiler (GCC, Clang)
works the same way.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Without Ninja, drop `-G Ninja` to use your platform's default generator
(e.g. Visual Studio solutions on Windows, Makefiles on Linux/macOS).

On Windows without a pre-configured shell, run this from a "Developer
Command Prompt for VS" (or after calling `vcvars64.bat`) so `cl.exe` and
`cmake` are on `PATH`.

## Running

```sh
./build/src/polycalc        # Linux/macOS
build\src\polycalc.exe      # Windows
```

## Running the Tests

```sh
cd build
ctest --output-on-failure
```

The suite (68 cases as of this writing) covers `Polynomial`'s rule-of-five
and arithmetic, the parser (including malformed-input rejection and a
round-trip property test), storage, the random generator, the history
manager, and the `Formatting`/`Terminal` utilities. It has also been run
under AddressSanitizer (`/fsanitize=address` with MSVC) with zero reported
issues; Valgrind is Linux-only and wasn't available in this Windows
development environment, so ASan plus manual review stood in for it.

## How the Linked List Works

A polynomial is a singly linked list of `Node { coefficient, exponent, next }`.
There is no array and no fixed size - a polynomial of any degree with any
number of terms grows and shrinks one heap-allocated node at a time.

Two insertion primitives exist, with deliberately different contracts:

- **`insertTerm`** searches the list for an existing node with the same
  exponent and merges into it (removing the node if the merge cancels to
  zero); otherwise it prepends a new node. This guarantees **no duplicate
  exponents**, but does **not** impose any particular order.
- **`appendTermRaw`** unconditionally prepends a new node, even if a term
  with that exponent already exists. The expression parser and the file
  loader use this, so that parsing `3x^2 + 4x^2` faithfully preserves both
  terms - exactly as a student working through the algorithm by hand would
  expect - until `mergeLikeTerms()` or `simplify()` is explicitly invoked.

This is a deliberate design choice: an earlier version of this project had
`insertTerm` also maintain sort order automatically, which made "Sort by
Exponent" and "Merge Like Terms" provably no-ops in every reachable state.
Relaxing the invariant makes those operations - required, named features -
actually do something observable, while `operator==`, `operator+`, and
`operator-` normalize their operands internally so arithmetic and equality
remain correct regardless of a polynomial's current order.

## Algorithms Used

- **Merge sort on the linked list** (`sortByExponent`) - classic
  divide-the-list-in-two-with-slow/fast-pointers, recurse, and merge by
  relinking existing nodes (no allocation). This is the textbook approach
  for sorting a linked list, since linked lists don't support the random
  access an in-place array sort needs.
- **Merge-based addition** (`operator+`) - after normalizing both operands,
  addition is a single left-to-right walk over two sorted lists, combining
  matching exponents and copying the rest, mirroring the merge step of merge
  sort itself.
- **Term-by-term multiplication** (`operator*`) - every pair of terms is
  multiplied (coefficients multiply, exponents add) into a raw result list,
  then `mergeLikeTerms()` sorts and combines it in one pass.
- **Recursive-descent-free expression parsing** - the parser splits on
  top-level `+`/`-`, then parses each term's optional sign, coefficient,
  optional `*`, variable, and exponent (either `^N` or Unicode superscript
  digits) with straightforward substring scanning - no external parser
  library needed.

## Complexity Reference

Let `n` and `m` be the term counts of the operands (`n` for a single
operand where only one is involved).

| Operation                          | Time              | Notes |
|-------------------------------------|-------------------|-------|
| `insertTerm` / `deleteTerm` / `updateCoefficient` / `updateExponent` | O(n) | Linear search by exponent |
| `appendTermRaw`                     | O(1)              | Unconditional prepend |
| `degree`                            | O(n)              | Scans for the maximum exponent |
| `termCount`, `evaluate`, `toString`, `terms()` | O(n)   | Single traversal |
| `sortByExponent`                    | O(n log n)        | Merge sort |
| `mergeLikeTerms` / `simplify`       | O(n log n)        | Sort, then one linear combine pass |
| `operator+` / `operator-`           | O(n log n + m log m) | Normalize both operands, then O(n+m) merge |
| `operator*`                         | O(n·m log(n·m))   | All pairs, then one merge-sort-based combine |
| `operator==`                        | O(n log n + m log m) | Compares normalized copies |

**Memory**: O(n) - one heap allocation per term (`sizeof(Node)` bytes each;
no arrays, no unused capacity). The Statistics screen reports a live
estimate for the current polynomial.

## Design Notes and Trade-offs

- **Exceptions over error codes**: `Polynomial` throws `std::invalid_argument`
  for a negative exponent, the parser throws with a message naming the
  offending substring, and storage/generator throw on invalid input or I/O
  failure. Every menu handler in `Application` catches `std::exception` and
  reports it via `ConsoleUI::printError` rather than crashing.
- **First-match semantics for duplicates**: if a polynomial currently holds
  duplicate exponents (e.g. right after parsing `4x^2 + 3x^2`),
  `deleteTerm`/`updateCoefficient`/`updateExponent` act on the first match
  found. Merge like terms first for unambiguous single-term edits.
  `evaluate`, `toString`, and arithmetic are all correct regardless.
- **EOF is not an error condition to spin on**: input prompts throw a
  dedicated `InputClosedException` (deliberately not derived from
  `std::exception`) when stdin is exhausted, so it passes straight through
  every menu's generic error handler and exits the program cleanly instead
  of looping forever on reads that can never succeed again.

## Future Improvements

- A Horner's-method evaluation variant for dense polynomials (current
  `evaluate` uses `std::pow` per term, which is simpler and correct but
  does slightly more work than necessary for consecutive integer exponents)
- Multi-variable polynomial support
- A scripted/batch mode (read a sequence of commands from a file) alongside
  the interactive menu
- Named polynomial slots (more than just "current" and "second") for
  juggling several polynomials in one session
