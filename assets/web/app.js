"use strict";

/* ---------- tiny API client ---------- */

async function apiGet(path) {
  const response = await fetch(path);
  const json = await response.json();
  if (!json.ok) throw new Error(json.error || "Request failed");
  return json;
}

async function apiPost(path, body) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body || {}),
  });
  const json = await response.json();
  if (!json.ok) throw new Error(json.error || "Request failed");
  return json;
}

/* ---------- DOM helpers ---------- */

const $ = (id) => document.getElementById(id);

function requireNumber(value, fieldName) {
  const n = Number(value);
  if (value === "" || Number.isNaN(n)) {
    throw new Error(`Invalid ${fieldName}: '${value}'`);
  }
  return n;
}

function requireInt(value, fieldName) {
  const n = requireNumber(value, fieldName);
  if (!Number.isInteger(n)) {
    throw new Error(`Invalid ${fieldName}: '${value}' (must be a whole number)`);
  }
  return n;
}

/* ---------- activity log ---------- */

const logEl = $("log");

function log(tag, message) {
  const entry = document.createElement("div");
  entry.className = "log-entry";

  const tagEl = document.createElement("span");
  tagEl.className = `log-tag log-tag-${tag.toLowerCase()}`;
  tagEl.textContent = `[${tag}]`;

  const msgEl = document.createElement("span");
  msgEl.className = "log-message";
  msgEl.textContent = message;

  entry.append(tagEl, msgEl);
  logEl.append(entry);
  logEl.scrollTop = logEl.scrollHeight;
}

const logSuccess = (message) => log("OK", message);
const logError = (message) => log("ERROR", message);
const logInfo = (message) => log("INFO", message);

$("btn-clear-log").addEventListener("click", () => { logEl.innerHTML = ""; });

/* ---------- state rendering ---------- */

function applyState(state) {
  $("current-expr").textContent = state.currentText;
  $("current-meta").innerHTML =
    `<span class="badge">degree ${state.currentDegree}</span>` +
    `<span class="badge">${state.termCount} term(s)</span>`;

  $("secondary-expr").textContent = `Q(x) = ${state.secondaryText}`;

  $("history-status").textContent =
    `Undo available: ${state.undoDepth}    Redo available: ${state.redoDepth}`;
  $("btn-undo").disabled = !state.canUndo;
  $("btn-redo").disabled = !state.canRedo;

  $("btn-use-result").disabled = !state.hasArithmeticResult;
}

function applyStatistics(stats) {
  const grid = $("stats-grid");
  grid.querySelector('[data-stat="termCount"]').textContent = stats.termCount;
  grid.querySelector('[data-stat="degreeText"]').textContent = stats.degreeText;
  grid.querySelector('[data-stat="memoryBytes"]').textContent = `${stats.memoryBytes} bytes`;
  grid.querySelector('[data-stat="sortMs"]').textContent = `${stats.sortMs.toFixed(4)} ms`;
  grid.querySelector('[data-stat="mergeMs"]').textContent = `${stats.mergeMs.toFixed(4)} ms`;
  grid.querySelector('[data-stat="simplifyMs"]').textContent = `${stats.simplifyMs.toFixed(4)} ms`;
  grid.querySelector('[data-stat="evaluateMs"]').textContent = `${stats.evaluateMs.toFixed(4)} ms`;
}

// Wraps a button/form handler: runs `action`, logs success via its
// returned message, applies fresh state if present, and reports any thrown
// error to the log instead of letting it escape - the same try/catch every
// MainWindow::onXxx used to do individually in the old Win32 GUI.
async function run(action) {
  try {
    const result = await action();
    if (result && result.state) applyState(result.state);
    if (result && result.message) logSuccess(result.message);
    return result;
  } catch (err) {
    logError(err.message || String(err));
    return null;
  }
}

/* ---------- Build & Edit P(x) ---------- */

$("form-expression").addEventListener("submit", (event) => {
  event.preventDefault();
  run(() => apiPost("/api/expression", { expression: $("input-expression").value }));
});

$("btn-insert-term").addEventListener("click", () => run(async () => {
  const coefficient = requireNumber($("input-coefficient").value, "coefficient");
  const exponent = requireInt($("input-exponent").value, "exponent");
  return apiPost("/api/term/insert", { coefficient, exponent });
}));

$("btn-delete-term").addEventListener("click", () => run(async () => {
  const exponent = requireInt($("input-exponent").value, "exponent");
  return apiPost("/api/term/delete", { exponent });
}));

$("btn-update-coefficient").addEventListener("click", () => run(async () => {
  const exponent = requireInt($("input-exponent").value, "exponent");
  const coefficient = requireNumber($("input-coefficient").value, "coefficient");
  return apiPost("/api/term/update-coefficient", { exponent, coefficient });
}));

$("btn-update-exponent").addEventListener("click", () => run(async () => {
  const oldExponent = requireInt($("input-exponent").value, "exponent");
  const newExponent = requireInt($("input-new-exponent").value, "new exponent");
  return apiPost("/api/term/update-exponent", { oldExponent, newExponent });
}));

$("btn-clear").addEventListener("click", () => run(() => apiPost("/api/clear")));

/* ---------- Q(x) & arithmetic ---------- */

$("form-secondary").addEventListener("submit", (event) => {
  event.preventDefault();
  run(() => apiPost("/api/secondary", { expression: $("input-secondary").value }));
});

async function doArithmetic(op) {
  return run(async () => {
    const result = await apiPost("/api/arithmetic", { op });
    $("arithmetic-result").textContent = `Result: ${result.label} = ${result.resultText}`;
    return result;
  });
}
$("btn-add").addEventListener("click", () => doArithmetic("add"));
$("btn-subtract").addEventListener("click", () => doArithmetic("subtract"));
$("btn-multiply").addEventListener("click", () => doArithmetic("multiply"));

$("btn-use-result").addEventListener("click", () => run(() => apiPost("/api/use-result")));

/* ---------- Analyze ---------- */

$("form-evaluate").addEventListener("submit", (event) => {
  event.preventDefault();
  run(async () => {
    const x = requireNumber($("input-x").value, "x");
    const result = await apiPost("/api/evaluate", { x });
    $("evaluate-result").textContent = `Result: ${result.message}`;
    return result;
  });
});

async function doCanonicalize(path) {
  return run(async () => {
    const result = await apiPost(path);
    $("timing-result").textContent = `Last operation took: ${result.elapsedMs.toFixed(4)} ms`;
    return result;
  });
}
$("btn-sort").addEventListener("click", () => doCanonicalize("/api/sort"));
$("btn-merge").addEventListener("click", () => doCanonicalize("/api/merge"));
$("btn-simplify").addEventListener("click", () => doCanonicalize("/api/simplify"));

/* ---------- History ---------- */

$("btn-undo").addEventListener("click", () => run(() => apiPost("/api/undo")));
$("btn-redo").addEventListener("click", () => run(() => apiPost("/api/redo")));

/* ---------- Save & Load ---------- */

$("btn-save").addEventListener("click", () => run(() =>
  apiPost("/api/save", { path: $("input-filepath").value })));

$("btn-load").addEventListener("click", () => run(() =>
  apiPost("/api/load", { path: $("input-filepath").value })));

async function browse(kind) {
  return run(async () => {
    const result = await apiPost(`/api/browse/${kind}`, { path: $("input-filepath").value });
    if (result.picked) {
      $("input-filepath").value = result.path;
      return { message: `Selected: ${result.path}` };
    }
    return { message: "Browse cancelled." };
  });
}
$("btn-browse-open").addEventListener("click", () => browse("open"));
$("btn-browse-save").addEventListener("click", () => browse("save"));

/* ---------- Random generator ---------- */

$("btn-generate").addEventListener("click", () => run(async () => {
  const termCount = requireInt($("input-gen-terms").value, "term count");
  const maxExponent = requireInt($("input-gen-max-exp").value, "max exponent");
  const minCoefficient = requireNumber($("input-gen-min-coef").value, "min coefficient");
  const maxCoefficient = requireNumber($("input-gen-max-coef").value, "max coefficient");
  return apiPost("/api/generate", { termCount, maxExponent, minCoefficient, maxCoefficient });
}));

/* ---------- Statistics ---------- */

$("btn-refresh-stats").addEventListener("click", async () => {
  try {
    const result = await apiGet("/api/statistics");
    applyStatistics(result.stats);
    logInfo("Statistics refreshed.");
  } catch (err) {
    logError(err.message || String(err));
  }
});

/* ---------- theme toggle ---------- */

const THEME_KEY = "polycalc-theme";

function applyStoredTheme() {
  const stored = localStorage.getItem(THEME_KEY);
  if (stored === "light" || stored === "dark") {
    document.documentElement.setAttribute("data-theme", stored);
  }
}

$("btn-theme").addEventListener("click", () => {
  const prefersDark = window.matchMedia("(prefers-color-scheme: dark)").matches;
  const current = document.documentElement.getAttribute("data-theme") || (prefersDark ? "dark" : "light");
  const next = current === "dark" ? "light" : "dark";
  document.documentElement.setAttribute("data-theme", next);
  localStorage.setItem(THEME_KEY, next);
});

applyStoredTheme();

/* ---------- quit ---------- */

$("btn-quit").addEventListener("click", async () => {
  if (!window.confirm("Stop the Polynomial Arithmetic Calculator server? This closes the session for every tab.")) {
    return;
  }
  try {
    await apiPost("/api/shutdown");
    document.body.innerHTML =
      '<main style="padding:48px;text-align:center;font-family:system-ui">' +
      "<h1>Server stopped</h1><p>You can close this tab.</p></main>";
  } catch (err) {
    logError(err.message || String(err));
  }
});

/* ---------- help modal ---------- */

const HELP_SECTIONS = [
  {
    title: "How a polynomial is stored",
    html: `<p>A polynomial here is a <strong>singly linked list</strong> of terms
      <code>(coefficient, exponent)</code>, not an array &mdash; so a polynomial of
      any degree grows and shrinks one node at a time, with no wasted space and
      no fixed size limit. Every panel below edits or reads the <strong>same</strong>
      current polynomial P(x), shown at the top of the page.</p>`,
  },
  {
    title: "1. Build &amp; Edit P(x)",
    html: `<p><strong>Expression + "Set P(x)"</strong>: type an expression such as
      <code>3x^2 + 4x - 8</code> or <code>5x&#8308; - 7</code> and submit to replace
      P(x) with it entirely. Exponents may be written as <code>^N</code> or with the
      same Unicode superscript digits the display uses, so re-typing what's shown
      always parses back to the same polynomial.</p>
      <p><strong>Insert Term</strong> adds <code>coefficient&middot;x^exponent</code>.
      If a term at that exponent already exists, the coefficients are combined (and
      the term is removed entirely if that cancels to zero) &mdash; duplicate
      exponents are never created by Insert.</p>
      <p><strong>Delete Term / Update Coefficient / Update Exponent</strong> locate
      the first term at the given exponent and remove it, change its coefficient, or
      move it to a new exponent (merging if a term is already there).</p>
      <p><strong>Clear P(x)</strong> resets to the zero polynomial.</p>
      <p><em>Note on order:</em> Insert Term keeps P(x) duplicate-free but does
      <strong>not</strong> keep it sorted; parsing an expression preserves terms
      exactly as typed, including duplicate exponents, until you Sort / Merge /
      Simplify in the Analyze panel. Addition, subtraction, multiplication, and
      evaluation are always correct regardless of current order.</p>`,
  },
  {
    title: "2. Second Polynomial Q(x) &amp; Arithmetic",
    html: `<p>Set Q(x) the same way as P(x) &mdash; an independent second slot used
      only for the operations below.</p>
      <p><strong>P + Q / P &minus; Q</strong> normalizes both operands (sorted,
      duplicate-free) and performs a single left-to-right merge over both term
      lists &mdash; the same merge step used by merge sort.
      Time: <code>O(n log n + m log m)</code> to normalize, then <code>O(n + m)</code>
      to merge.</p>
      <p><strong>P &times; Q</strong> multiplies every term of P against every term
      of Q (coefficients multiply, exponents add), then merges the raw result into
      canonical form in one pass. Time: <code>O(n&middot;m log(n&middot;m))</code>.</p>
      <p>The result is shown but does not change P(x) until you click
      <strong>"Use Result as P(x)"</strong>, which also records the change in
      History so it can be undone.</p>`,
  },
  {
    title: "3. Analyze",
    html: `<p><strong>Evaluate P(x)</strong> substitutes the given x and sums
      <code>coefficient &middot; x^exponent</code> over every term in one pass.
      Time: <code>O(n)</code>.</p>
      <p><strong>Sort</strong> reorders terms by descending exponent using merge
      sort adapted to a linked list (split with a slow/fast pointer pair, recurse,
      relink existing nodes &mdash; no array, no extra allocation). Merge sort is
      the standard choice here because linked lists can't be randomly indexed,
      ruling out array-style sorts like quicksort/heapsort. Time: <code>O(n log n)</code>.</p>
      <p><strong>Merge Like Terms</strong> sorts, then walks once combining any runs
      of equal exponents into a single term (dropping any that cancel to zero).
      Time: <code>O(n log n)</code>.</p>
      <p><strong>Simplify</strong> is sort + merge like terms + drop zero-coefficient
      terms in one call &mdash; the "put this in canonical form" button.
      Time: <code>O(n log n)</code>.</p>
      <p>Each button reports how long it took, timed live against the polynomial
      currently loaded.</p>`,
  },
  {
    title: "4. History (Undo / Redo)",
    html: `<p>Every action that actually changes P(x) pushes the prior state onto
      an undo stack first (actions that change nothing &mdash; e.g. deleting an
      exponent that was never present &mdash; do not add a no-op step). Undo pops
      the most recent snapshot and makes it current again, pushing the state you
      left onto the redo stack; Redo does the reverse. Making a new change after an
      undo clears the redo stack, the same rule any text editor or spreadsheet
      uses.</p>`,
  },
  {
    title: "5. Save &amp; Load",
    html: `<p>Saves P(x) to a small, explicit text format:</p>
      <pre style="font-family:var(--mono);font-size:12px;background:color-mix(in srgb, var(--accent) 10%, transparent);padding:10px 12px;border-radius:8px;">POLYCALC 1
&lt;term count&gt;
&lt;coefficient&gt; &lt;exponent&gt;
...</pre>
      <p>This is deliberately not the pretty-printed display string &mdash; an
      explicit line format can't be ambiguous the way re-parsing formatted output
      could be. "Browse (Open)&hellip;" and "Browse (Save)&hellip;" pop the same
      native Windows file picker the old desktop GUI used &mdash; even though the
      interface is now a web page, the process behind it is still a native Windows
      program, so it can still show that dialog and hand back a real filesystem
      path (something a browser's own file input can't do), or type a path
      directly.</p>`,
  },
  {
    title: "6. Random Generator",
    html: `<p>Fills P(x) with a random polynomial for demos or for timing the
      operations above on a non-trivial term count: <strong>Terms</strong> &mdash;
      how many terms to generate; <strong>Max exponent</strong> &mdash; exponents
      are drawn from <code>[0, Max exponent]</code>; <strong>Min/Max coefficient</strong>
      &mdash; coefficients are drawn from that range. The generator inserts terms
      one at a time (the same duplicate-merging Insert Term uses), so the result
      always comes out with distinct exponents.</p>`,
  },
  {
    title: "7. Statistics &amp; Performance",
    html: `<p>"Refresh Statistics" reports, for the current P(x): term count and
      degree; estimated memory (<code>term count &times; sizeof(Node)</code>, since
      each term is one heap allocation with no unused capacity); and timings for
      sort/merge/simplify/evaluate(2), measured live against a scratch copy so
      refreshing never mutates the polynomial you're inspecting.</p>`,
  },
  {
    title: "Complexity reference",
    html: `<table>
      <thead><tr><th>Operation</th><th>Time</th></tr></thead>
      <tbody>
        <tr><td>Insert / Delete / Update coefficient or exponent</td><td>O(n)</td></tr>
        <tr><td>Degree / term count / evaluate</td><td>O(n)</td></tr>
        <tr><td>Sort by Exponent</td><td>O(n log n)</td></tr>
        <tr><td>Merge Like Terms / Simplify</td><td>O(n log n)</td></tr>
        <tr><td>P + Q / P &minus; Q</td><td>O(n log n + m log m)</td></tr>
        <tr><td>P &times; Q</td><td>O(n&middot;m log(n&middot;m))</td></tr>
        <tr><td>Memory</td><td>O(n)</td></tr>
      </tbody>
    </table>
    <p><em>n, m</em> = term counts of the operand(s).</p>`,
  },
];

function renderHelp() {
  const body = $("help-body");
  body.innerHTML = HELP_SECTIONS.map(
    (section) => `<h3>${section.title}</h3>${section.html}`
  ).join("");
}

function openHelp() {
  $("help-backdrop").hidden = false;
  document.body.style.overflow = "hidden";
}
function closeHelp() {
  $("help-backdrop").hidden = true;
  document.body.style.overflow = "";
}

$("btn-help").addEventListener("click", openHelp);
$("btn-help-close").addEventListener("click", closeHelp);
$("help-backdrop").addEventListener("click", (event) => {
  if (event.target.id === "help-backdrop") closeHelp();
});
document.addEventListener("keydown", (event) => {
  if (event.key === "Escape" && !$("help-backdrop").hidden) closeHelp();
});

renderHelp();

/* ---------- initial load ---------- */

(async function init() {
  try {
    const result = await apiGet("/api/state");
    applyState(result.state);
    logInfo("Welcome! Build a polynomial below, or click Help for a full walkthrough.");
  } catch (err) {
    logError(`Failed to reach the backend: ${err.message || err}`);
  }
})();
