# Polynomial Arithmetic Calculator

A Windows app for building, editing, and computing with polynomials, backed
by a singly linked list. Built as an educational mathematics tool that also
demonstrates production-quality C++ practices: RAII, the rule of five,
exception safety, and a clean modular architecture - the math engine never
touches HTTP or JSON, and the web layer never touches polynomial math
directly.

The interface is a modern, minimal single-page web UI, served by the app's
own C++ backend on `127.0.0.1` and opened in your default browser
automatically - there's no separate server to run, no internet connection
involved, and no framework or package manager on either side (see
[Design Notes and Trade-offs](#design-notes-and-trade-offs) for why).
Every panel is labelled, every field has a sensible default where one makes
sense, and a **Help / How it works** button opens a full walkthrough of
every control plus the algorithm and time complexity behind it - so the app
documents itself; you don't need to read this file (or any source code) to
use it.

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

- File-based save/load, with the **native Windows file picker** (not an
  HTML `<input type=file>` - see [Design Notes](#design-notes-and-trade-offs)
  for why that distinction matters)
- Undo/redo history with a live depth readout
- An expression parser with Unicode-superscript round-tripping (typing what
  the display shows always parses back to the same polynomial)
- A random polynomial generator with configurable bounds
- A Statistics panel: term count, degree, an estimated memory footprint,
  and timings for sort/merge/simplify/evaluate
- An in-app Help modal covering every panel, control, algorithm, and
  complexity bound - see [Getting Started](#getting-started)
- A light/dark theme toggle that follows your OS preference by default

## Getting Started

1. Launch `polycalc.exe` (see [Building](#building) if you need to compile
   it first). A console window opens and prints the local address it's
   serving on, then your default browser opens to it automatically.
2. Type an expression like `3x^2 + 4x - 8` into the **Expression** field at
   the top-left and press Enter (or click "Set P(x)").
3. Click **Help / How it works** at any time for a full walkthrough of
   every panel below - what each control does, the algorithm behind it, and
   its time complexity. That panel is the primary reference for using this
   app; everything past this point in the README is background for anyone
   extending the code.
4. To stop the app, close the console window, press Ctrl+C in it, or click
   **Quit** in the page itself - all three shut the local server down the
   same way closing the old desktop window used to end the process.

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
                      and the operations the UI calls; no UI or I/O code of
                      its own - identical to what the old Win32 GUI called

Json                - minimal JSON value type: parse a request body, build
                      a response
HttpServer          - minimal loopback-only HTTP/1.1 server (Winsock2):
                      routing plus embedded static-asset serving
FileDialog          - wraps the native GetOpenFileNameW/GetSaveFileNameW
                      common dialogs
ApiServer           - wires every Application operation to an HTTP+JSON
                      route under /api/...; the web-era equivalent of the
                      old gui::MainWindow
assets/web/         - index.html, styles.css, app.js: the frontend, baked
                      into the .exe at build time (see Building below)
```

Each class has one responsibility: `Polynomial` never touches a socket,
`HttpServer` never touches polynomial math, and `ApiServer` only
orchestrates the two - the same separation the old Win32 `MainWindow` kept
between the window and the math engine, just with HTTP+JSON in place of
window messages. This keeps the math layer trivially unit-testable (see
[tests/](tests/)) independent of any UI concerns; `Application` itself is
fully unit-tested too, since it does no I/O of its own - only `ApiServer`
and the frontend changed when the GUI did.

## Folder Structure

```
include/polycalc/         Public headers, mirroring src/
  core/                   Node, Polynomial, Formatting, PolynomialParser
  services/               Timer, HistoryManager, PolynomialGenerator, PolynomialStorage
  web/                    Json, HttpServer, FileDialog, ApiServer, Win32/Utf8 helpers
  Application.hpp         Session state and the operations the UI calls
  Version.hpp             App name/version constants
src/                      Implementation, same layout as include/
  main.cpp                Entry point (console main())
assets/web/               index.html, styles.css, app.js - the frontend
cmake/EmbedWebAssets.cmake  Bakes assets/web/* into a generated C++ header
tests/                    Custom lightweight test framework + test suites
CMakeLists.txt            Top-level build configuration
```

Every module is a `.hpp`/`.cpp` pair under its own subdirectory - nothing is
dumped into a single file.

## Building

Requires a C++17 compiler and CMake 3.20+ on Windows (the backend is built
directly on Winsock2 and the Win32 common-dialog API, so this project
targets Windows specifically). Tested with MSVC 19.44 (Visual Studio 2022
Build Tools) and Ninja.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Without Ninja, drop `-G Ninja` to use your platform's default generator
(e.g. Visual Studio solutions).

On Windows without a pre-configured shell, run this from a "Developer
Command Prompt for VS" (or after calling `vcvars64.bat`) so `cl.exe` and
`cmake` are on `PATH`.

The build has one extra step versus a plain C++ compile: a
`cmake --build`-time custom command (`cmake/EmbedWebAssets.cmake`) reads
every file under `assets/web/` and generates
`build/generated/polycalc/web/WebAssets.hpp`, a header embedding each file's
bytes as a `constexpr` C++ string. This is what lets `polycalc.exe` remain a
single, self-contained file with no `web/` folder to keep alongside it at
runtime. Editing an existing file under `assets/web/` and rebuilding
regenerates the header automatically; **adding a new file** there requires
re-running the `cmake -S . -B build` configure step once (a known CMake
`file(GLOB)` limitation - see the comment in `src/CMakeLists.txt`).

## Running

```sh
build\src\polycalc.exe
```

It opens a console window (its status display - see
[Design Notes](#design-notes-and-trade-offs) for why this is a console app
now, not a windowless one) and your default browser opens to the app
automatically. See [Getting Started](#getting-started) above.

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
polynomial, and the statistics snapshot) - everything except the web/HTTP
layer itself, which (like the old Win32 window code before it) has no
meaningful logic to unit-test in isolation; it's a thin wrapper that calls
straight through to the already-tested `Application` methods. It has also
been run under AddressSanitizer (`/fsanitize=address` with MSVC) with zero
reported issues.

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

The in-app Help modal explains each of these in the context of the control
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
  them, and every `ApiServer` route wraps its handler in one shared
  try/catch (`ApiServer::wrap`) that turns any `std::exception` into a
  `{ok:false, error}` JSON response instead of crashing - the same job every
  individual `MainWindow::onXxx` handler used to do by hand in the old
  Win32 GUI.
- **First-match semantics for duplicates**: if a polynomial currently holds
  duplicate exponents (e.g. right after parsing `4x^2 + 3x^2`),
  `deleteTerm`/`updateCoefficient`/`updateExponent` act on the first match
  found. Merge like terms first for unambiguous single-term edits.
  `evaluate`, `toString`, and arithmetic are all correct regardless.
- **A local server, not a hosted one**: `ApiServer` binds `127.0.0.1` on an
  OS-assigned free port (never a fixed port, so two copies of the app can
  run side by side without a conflict) and is never reachable from outside
  the machine. There is exactly one `Application` per process - like the
  old single-window GUI, this app models one session, not a multi-user
  server.
- **No third-party HTTP or JSON library**: `HttpServer` and `Json` are both
  small, purpose-built for exactly what this app needs (loopback-only,
  small JSON payloads, a couple dozen fixed routes) rather than general-
  purpose libraries like cpp-httplib or nlohmann/json. This mirrors the
  project's existing expression parser, which was already hand-rolled for
  the same reason (see `PolynomialParser`) - and the test suite's own
  framework, built without internet access to fetch Catch2/doctest. Pulling
  in a package manager (vcpkg/Conan) to fetch either would have been the
  more common choice for a larger project, but would be a lot of new
  machinery for the actual surface area involved here.
- **A console-subsystem executable now, not a windowless one**: the old
  `polycalc` was built `WIN32`-subsystem (no attached console) because a
  Win32 window was its entire interface. Now that the interface is a web
  page, the console *is* the interface for the process itself - it reports
  the URL being served and stays open for the life of the server, the same
  role the old always-visible window played. See `src/CMakeLists.txt` and
  `src/main.cpp`.
- **The native file dialog survived the move to a browser UI on purpose**:
  a browser's `<input type=file>` can only *upload file contents* and can't
  preselect or return an arbitrary server-side path - it's built for a
  remote server that doesn't otherwise have filesystem access, which isn't
  this app's situation. Since the process serving the page is still a
  normal Windows program, `POST /api/browse/open` and `/api/browse/save`
  just call `GetOpenFileNameW`/`GetSaveFileNameW` directly (see
  `FileDialog.hpp`) and hand the browser back the chosen path, so Save &
  Load keeps the same native picker the desktop GUI always had.
- **Static assets are embedded, not deployed alongside the .exe**: see
  [Building](#building) - `cmake/EmbedWebAssets.cmake` runs at build time
  so the shipped artifact is still just `polycalc.exe`, matching how the
  old Win32 build shipped as a single file.

## Future Improvements

- A Horner's-method evaluation variant for dense polynomials (current
  `evaluate` uses `std::pow` per term, which is simpler and correct but
  does slightly more work than necessary for consecutive integer exponents)
- Multi-variable polynomial support
- Named polynomial slots (more than just "current" and "second") for
  juggling several polynomials in one session
- Per-tab/per-client sessions, if this ever needs to serve more than one
  browser tab's worth of state at a time (today, like the old single-window
  GUI, every open tab shares the same one `Application` instance)
