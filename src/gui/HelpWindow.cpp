#include "polycalc/gui/HelpWindow.hpp"

#include <sstream>
#include <string>

namespace polycalc::gui {

namespace {

constexpr wchar_t kHelpWindowClassName[] = L"PolycalcHelpWindow";
constexpr int kHelpWidth = 760;
constexpr int kHelpHeight = 680;

// Win32 EDIT controls need "\r\n" (not bare "\n") between lines, so every
// line built by buildHelpText() goes through this helper instead of raw
// string concatenation.
void addLine(std::ostringstream& out, const std::string& text = "") {
    out << text << "\r\n";
}

// The full contents of the Help window: what each panel does, how to use
// it, and the algorithm/complexity behind it. Mirrors the "Algorithms
// Used" / "Complexity Reference" sections of the project README, but
// organized panel-by-panel so it reads as a walkthrough rather than a
// reference doc.
std::string buildHelpText() {
    std::ostringstream out;

    addLine(out, "POLYNOMIAL ARITHMETIC CALCULATOR - HELP / HOW IT WORKS");
    addLine(out, "========================================================");
    addLine(out);
    addLine(out, "A polynomial here is stored as a singly linked list of terms");
    addLine(out, "(coefficient, exponent), not an array - so a polynomial of any");
    addLine(out, "degree grows and shrinks one node at a time, with no wasted");
    addLine(out, "space and no fixed size limit. Every panel below edits or");
    addLine(out, "reads the SAME current polynomial P(x), shown at the top of");
    addLine(out, "the main window.");
    addLine(out);

    addLine(out, "1) BUILD & EDIT P(x)");
    addLine(out, "--------------------");
    addLine(out, "Expression + \"Set P(x)\": type an expression such as");
    addLine(out, "  3x^2 + 4x - 8        or        5x\xE2\x81\xB4 - 7");
    addLine(out, "and click the button to replace P(x) with it entirely.");
    addLine(out, "Exponents may be written as ^N or with the same Unicode");
    addLine(out, "superscript digits the display uses, so re-typing what is");
    addLine(out, "shown at the top always parses back to the same polynomial.");
    addLine(out);
    addLine(out, "Insert Term: adds coefficient*x^exponent. If a term at that");
    addLine(out, "exponent already exists, the coefficients are combined (and");
    addLine(out, "the term is removed entirely if that cancels to zero) -");
    addLine(out, "duplicate exponents are never created by Insert.");
    addLine(out);
    addLine(out, "Delete Term / Update Coefficient / Update Exponent: locate");
    addLine(out, "the first term at the given exponent and remove it, change");
    addLine(out, "its coefficient, or move it to a new exponent (merging if a");
    addLine(out, "term is already there). Each reports whether anything");
    addLine(out, "actually changed.");
    addLine(out);
    addLine(out, "Clear P(x): resets to the zero polynomial.");
    addLine(out);
    addLine(out, "Note on order: Insert Term keeps P(x) duplicate-free but does");
    addLine(out, "NOT keep it sorted; parsing an expression preserves terms");
    addLine(out, "exactly as typed, including duplicate exponents (so typing");
    addLine(out, "\"3x^2 + 4x^2\" keeps both terms until you Sort/Merge/Simplify");
    addLine(out, "in the Analyze panel). Addition, subtraction, multiplication,");
    addLine(out, "and evaluation are always correct regardless of current order.");
    addLine(out);

    addLine(out, "2) SECOND POLYNOMIAL Q(x) & ARITHMETIC");
    addLine(out, "---------------------------------------");
    addLine(out, "Set Q(x) the same way as P(x), an independent second slot");
    addLine(out, "used only for the operations below.");
    addLine(out);
    addLine(out, "P + Q / P - Q: normalizes both operands (sorted, duplicate-");
    addLine(out, "free) and performs a single left-to-right merge over both");
    addLine(out, "term lists, combining matching exponents and copying the");
    addLine(out, "rest - the same merge step used by merge sort.");
    addLine(out, "  Time: O(n log n + m log m) for the normalize step, then");
    addLine(out, "        O(n + m) for the merge.");
    addLine(out);
    addLine(out, "P * Q: every term of P is multiplied against every term of");
    addLine(out, "Q (coefficients multiply, exponents add), then the raw");
    addLine(out, "result is merged into canonical form in one pass.");
    addLine(out, "  Time: O(n*m log(n*m)).");
    addLine(out);
    addLine(out, "The result is shown but does not change P(x) until you");
    addLine(out, "click \"Use Result as P(x)\", which also records the change");
    addLine(out, "in History so it can be undone.");
    addLine(out);

    addLine(out, "3) ANALYZE");
    addLine(out, "-----------");
    addLine(out, "Evaluate P(x): substitutes the given x and computes the sum");
    addLine(out, "of coefficient * x^exponent over every term (one pass).");
    addLine(out, "  Time: O(n).");
    addLine(out);
    addLine(out, "Sort: reorders terms by descending exponent using merge");
    addLine(out, "sort adapted to a linked list (split with a slow/fast");
    addLine(out, "pointer pair, recurse, relink existing nodes back together -");
    addLine(out, "no array, no extra allocation). Merge sort is the standard");
    addLine(out, "choice here because linked lists cannot be randomly indexed,");
    addLine(out, "which rules out array-style sorts like quicksort/heapsort.");
    addLine(out, "  Time: O(n log n).");
    addLine(out);
    addLine(out, "Merge Like Terms: sorts, then walks once combining any runs");
    addLine(out, "of equal exponents into a single term (dropping any that");
    addLine(out, "cancel to zero).");
    addLine(out, "  Time: O(n log n).");
    addLine(out);
    addLine(out, "Simplify: sort + merge like terms + drop zero-coefficient");
    addLine(out, "terms in one call - the \"put this in canonical form\" button.");
    addLine(out, "  Time: O(n log n).");
    addLine(out);
    addLine(out, "Each button reports how long it took in milliseconds, timed");
    addLine(out, "live against the polynomial currently loaded.");
    addLine(out);

    addLine(out, "4) HISTORY (UNDO / REDO)");
    addLine(out, "-------------------------");
    addLine(out, "Every action that actually changes P(x) pushes the prior");
    addLine(out, "state onto an undo stack first (actions that change nothing -");
    addLine(out, "e.g. deleting an exponent that was never present - do not");
    addLine(out, "add a no-op step). Undo pops the most recent snapshot and");
    addLine(out, "makes it current again, pushing the state you left onto the");
    addLine(out, "redo stack; Redo does the reverse. Making a new change after");
    addLine(out, "an undo clears the redo stack, the same rule any text editor");
    addLine(out, "or spreadsheet uses.");
    addLine(out);

    addLine(out, "5) SAVE & LOAD");
    addLine(out, "---------------");
    addLine(out, "Saves P(x) to a small, explicit text format:");
    addLine(out, "  POLYCALC 1");
    addLine(out, "  <term count>");
    addLine(out, "  <coefficient> <exponent>");
    addLine(out, "  ...");
    addLine(out, "This is deliberately not the pretty-printed display string -");
    addLine(out, "an explicit line format can't be ambiguous the way re-parsing");
    addLine(out, "formatted output could be. Use \"Browse...\" to pick a file");
    addLine(out, "with the native file picker, or type a path directly.");
    addLine(out);

    addLine(out, "6) RANDOM GENERATOR");
    addLine(out, "--------------------");
    addLine(out, "Fills P(x) with a random polynomial for demos or for timing");
    addLine(out, "the operations above on a non-trivial term count:");
    addLine(out, "  Terms      - how many terms to generate");
    addLine(out, "  Max exp    - exponents are drawn from [0, Max exp]");
    addLine(out, "  Min/Max c  - coefficients are drawn from [Min c, Max c]");
    addLine(out, "The generator inserts terms one at a time (the same");
    addLine(out, "duplicate-merging Insert Term uses), so the result always");
    addLine(out, "comes out with distinct exponents.");
    addLine(out);

    addLine(out, "7) STATISTICS & PERFORMANCE");
    addLine(out, "-----------------------------");
    addLine(out, "\"Refresh Statistics\" reports, for the current P(x):");
    addLine(out, "  Term count / Degree  - current shape of the polynomial");
    addLine(out, "  Estimated memory     - term count * sizeof(Node), since");
    addLine(out, "                         each term is one heap allocation");
    addLine(out, "                         with no unused capacity");
    addLine(out, "  Timings              - sort/merge/simplify/evaluate(2),");
    addLine(out, "                         measured live against a scratch");
    addLine(out, "                         copy so refreshing never mutates");
    addLine(out, "                         the polynomial you're inspecting");
    addLine(out);

    addLine(out, "COMPLEXITY REFERENCE (n, m = term counts of the operand(s))");
    addLine(out, "-------------------------------------------------------------");
    addLine(out, "  Insert / Delete / Update coefficient or exponent   O(n)");
    addLine(out, "  Degree / term count / evaluate                     O(n)");
    addLine(out, "  Sort by Exponent                                    O(n log n)");
    addLine(out, "  Merge Like Terms / Simplify                         O(n log n)");
    addLine(out, "  P + Q / P - Q                                       O(n log n + m log m)");
    addLine(out, "  P * Q                                               O(n*m log(n*m))");
    addLine(out, "  Memory                                              O(n)");

    return out.str();
}

LRESULT CALLBACK HelpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_SIZE: {
            RECT client;
            GetClientRect(hwnd, &client);
            HWND edit = GetWindow(hwnd, GW_CHILD);
            if (edit != nullptr) {
                MoveWindow(edit, 8, 8, client.right - 16, client.bottom - 16, TRUE);
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

} // namespace

void ShowHelpWindow(HWND owner, HINSTANCE instance) {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &HelpWndProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        windowClass.lpszClassName = kHelpWindowClassName;
        RegisterClassExW(&windowClass);
        classRegistered = true;
    }

    RECT ownerRect{};
    GetWindowRect(owner, &ownerRect);
    const int x = ownerRect.left + (ownerRect.right - ownerRect.left - kHelpWidth) / 2;
    const int y = ownerRect.top + (ownerRect.bottom - ownerRect.top - kHelpHeight) / 2;

    const DWORD style =
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    RECT windowRect{0, 0, kHelpWidth, kHelpHeight};
    AdjustWindowRect(&windowRect, style, FALSE);

    HWND helpWindow =
        CreateWindowExW(0, kHelpWindowClassName, L"Help / How it Works", style, x, y,
                        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                        owner, nullptr, instance, nullptr);
    if (helpWindow == nullptr) {
        return;
    }

    RECT client{};
    GetClientRect(helpWindow, &client);
    HWND edit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 8, 8,
        client.right - 16, client.bottom - 16, helpWindow, nullptr, instance, nullptr);

    static HFONT monospaceFont = CreateFontW(
        -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH, L"Consolas");
    SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(monospaceFont), TRUE);
    SetControlTextUtf8(edit, buildHelpText());

    ShowWindow(helpWindow, SW_SHOW);
    SetForegroundWindow(helpWindow);
}

} // namespace polycalc::gui
