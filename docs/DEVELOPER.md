# Developer Documentation

This document is for anyone extending or maintaining the codebase. For a
user-facing overview, see the top-level [README.md](../README.md).

## Module Reference

### `polycalc::Node` (`include/polycalc/core/Node.hpp`)

A plain struct: `double coefficient`, `int exponent`, `Node* next`. No
behavior lives here on purpose - it's a data record, not an object.

### `polycalc::Polynomial` (`core/Polynomial.hpp` / `.cpp`)

The central class. Owns a `Node* head_` and manages its lifetime with the
rule of five (copy ctor/assign deep-copy the list; move ctor/assign steal
the pointer; the destructor frees every node).

Key invariant, and why it's *not* "always sorted and duplicate-free" - see
[README.md § How the Linked List Works](../README.md#how-the-linked-list-works)
for the full reasoning. In short: `insertTerm` dedupes but doesn't sort;
`appendTermRaw` doesn't do either; `sortByExponent`/`mergeLikeTerms`/
`simplify` are what restore canonical form; `operator+`, `operator-`, and
`operator==` normalize internally so they're correct regardless.

When adding a new mutating method, ask: does it need to find an existing
term by exponent (use the `deleteTerm`-style linear scan pattern), or does
it need to build a fresh list (use the `operator+`-style local `append`
lambda with a `tail` pointer, or `appendTermRaw` for prepend)? Don't mix the
two styles in one method.

Exception safety: any method that allocates (`new Node`, or a `Polynomial`
copy) can throw `std::bad_alloc`. The copy constructor explicitly catches,
calls `clear()` on the partially-built list, and rethrows, since a
half-constructed object's destructor never runs. Methods that mutate an
already-fully-constructed `Polynomial` (like `operator+`'s local `result`)
don't need that try/catch - if an exception escapes, the object's own
destructor still runs during stack unwinding.

### `polycalc::Formatting` (`core/Formatting.hpp` / `.cpp`)

Free functions, not a class - there's no state to encapsulate.
`toSuperscript`/`fromSuperscript` are inverses of each other (tested
directly in `tests/test_formatting.cpp`); `displayWidth` counts UTF-8
codepoints rather than bytes, which is what lets `ConsoleUI`'s box borders
line up around text containing superscript digits.

### `polycalc::PolynomialParser` (`core/PolynomialParser.hpp` / `.cpp`)

One static method, `parse(const std::string&) -> Polynomial`. Internal
helpers (`stripWhitespace`, `parseCoefficientMagnitude`, `parseExponent`,
`parseSingleTerm`) live in an anonymous namespace in the `.cpp` - they're
implementation details, not part of the public surface.

If you extend the grammar (e.g. to support parentheses or multiple
variables), the term-splitting loop in `parse()` (which finds top-level `+`
and `-`) is the place to revisit first; it currently assumes a flat sum of
terms.

### Services (`include/polycalc/services/`)

- **`Timer`**: wraps `std::chrono::steady_clock`. `measureMilliseconds` is a
  template because it takes an arbitrary callable; everything else is a
  compiled, non-template method in `Timer.cpp`.
- **`PolynomialGenerator`**: validates its `Options` (positive term count,
  non-negative max exponent, `min <= max`) before drawing anything, and uses
  `insertTerm` (not `appendTermRaw`) since a generated polynomial should come
  out duplicate-free, not requiring a follow-up merge.
- **`PolynomialStorage`**: a deliberately simple line-based format
  (`POLYCALC <version>`, then a count, then `coefficient exponent` pairs).
  It does not attempt to parse a pretty-printed display string, to avoid
  coupling the file format to `toString()`'s formatting choices.
- **`HistoryManager`**: two `std::vector<Polynomial>` stacks. `record()` is
  the caller's responsibility to invoke *before* mutating, with the
  pre-mutation state - see `Application::recordIfChanged` for the pattern
  used throughout the UI layer (only record when a mutation actually
  changed something, so undo/redo doesn't accumulate no-op steps).

### Console layer (`include/polycalc/console/`)

- **`Terminal`**: detects ANSI escape support once (a function-local static,
  so it's computed lazily and only once per process) and enables Windows'
  `ENABLE_VIRTUAL_TERMINAL_PROCESSING` if needed. `colorize()` is a no-op
  wrapper when color is unsupported.
- **`ConsoleUI`**: all layout math (box widths, table column widths,
  centering) is computed from `Formatting::displayWidth()` on the *plain*
  text, and ANSI color codes are only added afterward, around the exact
  substring being displayed. Never call `displayWidth` on a string that
  already has color codes baked in - the escape bytes are ASCII and will
  inflate the count and break alignment. This is why `printMenu` does not
  bold individual option numbers: doing so would require per-substring
  coloring inside an already width-padded line.
- `ConsoleUI::readLine` (and everything built on it - `readInt`,
  `readDouble`, `readMenuChoice`) throws `InputClosedException` when
  `std::getline` fails (EOF). That type deliberately does not derive from
  `std::exception`, so it passes through every submenu's
  `catch (const std::exception&)` untouched and propagates to `main()`.

### `polycalc::Application` (`include/polycalc/Application.hpp` / `src/Application.cpp`)

Owns `current_` (the working polynomial), `secondary_` (used only by the
Arithmetic submenu), and a `HistoryManager`. Each `handleX()` method runs its
own submenu loop until the user picks "Back". The pattern for any mutating
action is:

```cpp
const Polynomial before = current_;
const bool changed = /* call the mutating method, or compare before/after */;
recordIfChanged(before, changed);
```

Add new menu items by following this pattern and wrapping the whole
`switch` in the existing `try { ... } catch (const std::exception& ex) {
ConsoleUI::printError(ex.what()); }` block, so a malformed expression or
storage error never crashes the session.

## Testing

`tests/TestFramework.hpp` is a ~70-line self-registering test framework
(no external dependency - the build environment this project was developed
in had no internet access for fetching Catch2/doctest). `TEST_CASE("name") {
... }` registers itself via a static `Registrar`; `REQUIRE`/`REQUIRE_EQ`
throw `std::runtime_error` with the failing expression and location on
failure, which `main_test.cpp`'s runner catches and reports per-case.

Each module has its own `test_*.cpp`. When adding a class, add a matching
test file and list it in `tests/CMakeLists.txt`.

Memory safety was checked with MSVC's AddressSanitizer
(`/fsanitize=address`) rather than Valgrind, since this project was
developed on Windows where Valgrind isn't available. If you're on Linux,
`valgrind --leak-check=full ./build/tests/polycalc_tests` and the same
against `polycalc` after piping in a session should both report zero
leaks, given the RAII design.

## Build System

`CMakeLists.txt` (top level) sets C++17, enables warnings (`/W4` on MSVC,
`-Wall -Wextra -Wpedantic` elsewhere), and delegates to `src/CMakeLists.txt`
(the `polycalc_core` static library plus the `polycalc` executable) and
`tests/CMakeLists.txt` (the `polycalc_tests` executable, registered with
CTest). There's no dependency on any package manager - everything needed is
in the standard library.
