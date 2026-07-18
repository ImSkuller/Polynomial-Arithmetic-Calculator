# Polynomial Arithmetic Calculator

A Windows desktop app for building, editing, and computing with polynomials,
backed by a singly linked list. Built as an educational mathematics tool
that also demonstrates production-quality C++ practices: RAII, the rule of
five, exception safety, and a clean modular architecture - the math engine
never touches a window, and the GUI never touches polynomial math directly.

Everything is one always-visible window: no menus to navigate, no commands
to memorize. Every panel is labelled, every field has a sensible default
where one makes sense, and a **Help / How it works** button in the top
corner opens a full walkthrough of every control plus the algorithm and
time complexity behind it - so the app documents itself; you don't need to
read this file (or any source code) to use it.

## Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
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

- Build a polynomial by typing an expression (`3x^2 + 4x - 8`), or edit it
  term by term (insert, delete, update a coefficient, update an exponent)
- Live display in proper mathematical notation (`5x⁴ + 3x² - 7`, not
  `5x4+3x2-7`)
- Addition, subtraction, and multiplication against a second polynomial
- Sort by exponent, merge like terms, and simplify, each with a live timing
- Evaluate at any `x`
- Degree, term count, clear, deep copy, and a leak-free destructor
  underneath it all

### Bonus

- File-based save/load, with a native "Browse..." file picker
- Undo/redo history with a live depth readout
- An expression parser with Unicode-superscript round-tripping (typing what
  the display shows always parses back to the same polynomial)
- A random polynomial generator with configurable bounds
- A Statistics panel: term count, degree, an estimated memory footprint,
  and timings for sort/merge/simplify/evaluate
- An in-app Help window covering every panel, control, algorithm, and
  complexity bound - see [Getting Started](#getting-started)

## Getting Started

1. Launch `polycalc.exe` (see [Building](#building) if you need to compile
   it first).
2. Type an expression like `3x^2 + 4x - 8` into the **Expression** field at
   the top-left and press Enter (or click "Set P(x) from expression").
3. Click **Help / How it works** at any time for a full walkthrough of
   every panel below - what each button does, the algorithm behind it, and
   its time complexity. That window is the primary reference for using this
   app; everything past this point in the README is background for anyone
   extending the code.

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
Application         - session state (current/secondary polynomial, history)
                      and the operations the UI calls; no UI code of its own
MainWindow          - the Win32 main window: builds every control and wires
                      it to Application
HelpWindow          - the in-app Help / How it works window
WinUtil             - UTF-8/UTF-16 conversion and small layout helpers
                      shared by MainWindow and HelpWindow
```

Each class has one responsibility: `Polynomial` never touches a window,
`MainWindow` never touches polynomial math, and `Application` only
orchestrates the two. This keeps the math layer trivially unit-testable
(see [tests/](tests/)) independent of any UI concerns - `Application` itself
is now fully unit-tested too, since it no longer does any I/O of its own.

## Folder Structure

```
include/polycalc/         Public headers, mirroring src/
  core/                   Node, Polynomial, Formatting, PolynomialParser
  services/               Timer, HistoryManager, PolynomialGenerator, PolynomialStorage
  gui/                    WinUtil, MainWindow, HelpWindow
  Application.hpp         Session state and the operations the UI calls
  Version.hpp             App name/version constants
src/                      Implementation, same layout as include/
  main.cpp                Entry point (WinMain)
tests/                    Custom lightweight test framework + test suites
assets/                   Where to add real screenshots
CMakeLists.txt            Top-level build configuration
```

Every module is a `.hpp`/`.cpp` pair under its own subdirectory - nothing is
dumped into a single file.

## Building

Requires a C++17 compiler and CMake 3.20+ on Windows (the GUI is built
directly on the Win32 API, so this project targets Windows specifically).
Tested with MSVC 19.44 (Visual Studio 2022 Build Tools) and Ninja.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Without Ninja, drop `-G Ninja` to use your platform's default generator
(e.g. Visual Studio solutions).

On Windows without a pre-configured shell, run this from a "Developer
Command Prompt for VS" (or after calling `vcvars64.bat`) so `cl.exe` and
`cmake` are on `PATH`.

## Running

```sh
build\src\polycalc.exe
```

It opens as a normal window - no console attaches, and there's nothing to
pipe input into. See [Getting Started](#getting-started) above.

## Running the Tests

```sh
cd build
ctest --output-on-failure
```

The suite covers `Polynomial`'s rule-of-five and arithmetic, the parser
(including malformed-input rejection and a round-trip property test),
storage, the random generator, the history manager, the `Formatting`
utilities, and `Application`'s session logic (expression parsing,
insert/delete/update, undo/redo, arithmetic against the secondary
polynomial, and the statistics snapshot) - everything except the Win32
window code itself, which has no meaningful logic to unit-test in
isolation. It has also been run under AddressSanitizer
(`/fsanitize=address` with MSVC) with zero reported issues.

## How the Linked List Works

A polynomial is a singly linked list of `Node { coefficient, exponent, next }`.
There is no array and no fixed size - a polynomial of any degree with any
number of terms grows and shrinks one heap-allocated node at a time.

Two insertion primitives exist, with deliberately different contracts:

- **`insertTerm`** searches the list for an existing node with the same
  exponent and merges into it (removing the node if the merge cancels to
  zero); otherwise it prepends a new node. This guarantees **no duplicate
  exponents**, but does **not** impose any particular order. The Build &
  Edit panel's Insert Term button calls this.
- **`appendTermRaw`** unconditionally appends a new node at the tail, even
  if a term with that exponent already exists. The expression parser and the
  file loader use this, so that parsing `3x^2 + 4x^2` faithfully preserves
  both terms in the order they were typed - exactly as a student working
  through the algorithm by hand would expect - until `mergeLikeTerms()` or
  `simplify()` is explicitly invoked from the Analyze panel.

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

The in-app Help window explains each of these in the context of the button
that triggers it, along with its complexity.

## Complexity Reference

Let `n` and `m` be the term counts of the operands (`n` for a single
operand where only one is involved).

| Operation                          | Time              | Notes |
|-------------------------------------|-------------------|-------|
| `insertTerm` / `deleteTerm` / `updateCoefficient` / `updateExponent` | O(n) | Linear search by exponent |
| `appendTermRaw`                     | O(n)              | Unconditional tail append (walks to the tail so input order is preserved) |
| `degree`                            | O(n)              | Scans for the maximum exponent |
| `termCount`, `evaluate`, `toString`, `terms()` | O(n)   | Single traversal |
| `sortByExponent`                    | O(n log n)        | Merge sort |
| `mergeLikeTerms` / `simplify`       | O(n log n)        | Sort, then one linear combine pass |
| `operator+` / `operator-`           | O(n log n + m log m) | Normalize both operands, then O(n+m) merge |
| `operator*`                         | O(n·m log(n·m))   | All pairs, then one merge-sort-based combine |
| `operator==`                        | O(n log n + m log m) | Compares normalized copies |

**Memory**: O(n) - one heap allocation per term (`sizeof(Node)` bytes each;
no arrays, no unused capacity). The Statistics panel reports a live estimate
for the current polynomial.

## Design Notes and Trade-offs

- **Exceptions over error codes**: `Polynomial` throws `std::invalid_argument`
  for a negative exponent, the parser throws with a message naming the
  offending substring, and storage/generator throw on invalid input or I/O
  failure. `Application`'s methods propagate these rather than swallowing
  them, and every `MainWindow` handler catches `std::exception` and reports
  it through the activity log rather than crashing.
- **First-match semantics for duplicates**: if a polynomial currently holds
  duplicate exponents (e.g. right after parsing `4x^2 + 3x^2`),
  `deleteTerm`/`updateCoefficient`/`updateExponent` act on the first match
  found. Merge like terms first for unambiguous single-term edits.
  `evaluate`, `toString`, and arithmetic are all correct regardless.
- **A GUI-subsystem executable, on purpose**: `polycalc` is built with
  `add_executable(polycalc WIN32 ...)`, so it launches with no attached
  console. All input and output go through window controls; there's no
  console fallback to maintain.

## Future Improvements

- A Horner's-method evaluation variant for dense polynomials (current
  `evaluate` uses `std::pow` per term, which is simpler and correct but
  does slightly more work than necessary for consecutive integer exponents)
- Multi-variable polynomial support
- Named polynomial slots (more than just "current" and "second") for
  juggling several polynomials in one session
- A resizable/DPI-aware layout for the main window (currently a fixed-size
  window sized to fit every panel without scrolling)
