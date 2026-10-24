.pragma library
// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
//
// Beam search: fuzzy ranking and a safe arithmetic evaluator.
// Pure functions only, so they are unit-tested in tests/shell/tst_search.qml.

// Scores how well `query` matches `text` as an in-order subsequence.
// Returns 0 for no match. Prefix, word-start and consecutive matches score
// higher, and shorter targets win ties.
function fuzzyScore(query, text) {
    if (!query || !text)
        return 0;
    const q = query.toLowerCase();
    const t = text.toLowerCase();
    if (t === q)
        return 1000;
    if (t.startsWith(q))
        return 900 - Math.min(t.length - q.length, 100);

    const wordStart = t.indexOf(" " + q);
    if (wordStart >= 0)
        return 760 - Math.min(wordStart, 100);

    let score = 0;
    let ti = 0;
    let streak = 0;
    for (let qi = 0; qi < q.length; ++qi) {
        const ch = q[qi];
        if (ch === " ")
            continue;
        let found = -1;
        for (let j = ti; j < t.length; ++j) {
            if (t[j] === ch) {
                found = j;
                break;
            }
        }
        if (found < 0)
            return 0;
        const atWordStart = found === 0 || t[found - 1] === " " || t[found - 1] === "-";
        streak = found === ti ? streak + 1 : 0;
        score += 10 + streak * 8 + (atWordStart ? 25 : 0);
        ti = found + 1;
    }
    return Math.max(1, Math.min(600, score - Math.min(t.length, 60)));
}

// Best score of `query` against a title and optional keyword string.
function matchEntry(query, title, keywords) {
    const titleScore = fuzzyScore(query, title);
    const keywordScore = keywords ? fuzzyScore(query, keywords) * 0.7 : 0;
    return Math.max(titleScore, keywordScore);
}

// ── Calculator ──────────────────────────────────────────────────────────────

const FUNCTIONS = {
    "sqrt": Math.sqrt, "sin": Math.sin, "cos": Math.cos, "tan": Math.tan,
    "log": Math.log10, "ln": Math.log, "abs": Math.abs, "round": Math.round,
    "floor": Math.floor, "ceil": Math.ceil
};
const CONSTANTS = { "pi": Math.PI, "e": Math.E, "tau": 2 * Math.PI };
const PRECEDENCE = { "+": 1, "-": 1, "*": 2, "/": 2, "%": 2, "^": 3, "u-": 4 };
const RIGHT_ASSOC = { "^": true, "u-": true };

function tokenize(expr) {
    const tokens = [];
    let i = 0;
    while (i < expr.length) {
        const ch = expr[i];
        if (ch === " ") { ++i; continue; }
        if ("0123456789.".indexOf(ch) >= 0) {
            let j = i;
            while (j < expr.length && "0123456789.".indexOf(expr[j]) >= 0) ++j;
            const value = parseFloat(expr.slice(i, j));
            if (isNaN(value)) return null;
            tokens.push({ type: "num", value: value });
            i = j;
            continue;
        }
        if (/[a-z]/i.test(ch)) {
            let j = i;
            while (j < expr.length && /[a-z]/i.test(expr[j])) ++j;
            const name = expr.slice(i, j).toLowerCase();
            if (FUNCTIONS[name]) tokens.push({ type: "fn", name: name });
            else if (CONSTANTS[name] !== undefined) tokens.push({ type: "num", value: CONSTANTS[name] });
            else return null;
            i = j;
            continue;
        }
        if ("+-*/%^()×÷".indexOf(ch) >= 0) {
            const op = ch === "×" ? "*" : ch === "÷" ? "/" : ch;
            tokens.push({ type: op === "(" || op === ")" ? op : "op", op: op });
            ++i;
            continue;
        }
        return null;
    }
    return tokens;
}

// Evaluates an arithmetic expression; returns a number or null when the input
// is not a well-formed expression. Never uses eval().
function evaluate(expr) {
    if (!expr || !/[0-9]/.test(expr) || !/[+\-*/%^()×÷a-z]/i.test(expr.replace(/^\s*-?[\d.]+\s*$/, "")))
        return null;
    const tokens = tokenize(expr.trim());
    if (!tokens || tokens.length === 0)
        return null;

    const output = [];
    const ops = [];
    let expectOperand = true;
    for (const tok of tokens) {
        if (tok.type === "num") {
            if (!expectOperand) return null;
            output.push(tok);
            expectOperand = false;
        } else if (tok.type === "fn") {
            if (!expectOperand) return null;
            ops.push(tok);
        } else if (tok.type === "(") {
            if (!expectOperand) return null;
            ops.push(tok);
        } else if (tok.type === ")") {
            if (expectOperand) return null;
            while (ops.length && ops[ops.length - 1].type !== "(") output.push(ops.pop());
            if (!ops.length) return null;
            ops.pop();
            if (ops.length && ops[ops.length - 1].type === "fn") output.push(ops.pop());
        } else {
            let op = tok.op;
            if (expectOperand) {
                if (op === "-") op = "u-";
                else if (op === "+") continue;
                else return null;
            }
            while (ops.length) {
                const top = ops[ops.length - 1];
                if (top.type !== "op") break;
                const p1 = PRECEDENCE[op], p2 = PRECEDENCE[top.op];
                if (p2 > p1 || (p2 === p1 && !RIGHT_ASSOC[op])) output.push(ops.pop());
                else break;
            }
            ops.push({ type: "op", op: op });
            expectOperand = true;
        }
    }
    if (expectOperand) return null;
    while (ops.length) {
        const top = ops.pop();
        if (top.type === "(") return null;
        output.push(top);
    }

    const stack = [];
    for (const tok of output) {
        if (tok.type === "num") { stack.push(tok.value); continue; }
        if (tok.type === "fn") {
            if (stack.length < 1) return null;
            stack.push(FUNCTIONS[tok.name](stack.pop()));
            continue;
        }
        if (tok.op === "u-") {
            if (stack.length < 1) return null;
            stack.push(-stack.pop());
            continue;
        }
        if (stack.length < 2) return null;
        const b = stack.pop(), a = stack.pop();
        switch (tok.op) {
        case "+": stack.push(a + b); break;
        case "-": stack.push(a - b); break;
        case "*": stack.push(a * b); break;
        case "/": stack.push(a / b); break;
        case "%": stack.push(a % b); break;
        case "^": stack.push(Math.pow(a, b)); break;
        default: return null;
        }
    }
    if (stack.length !== 1 || !isFinite(stack[0]))
        return null;
    return stack[0];
}

function formatNumber(value) {
    if (Math.abs(value) >= 1e15 || (Math.abs(value) < 1e-6 && value !== 0))
        return value.toExponential(6);
    return String(Math.round(value * 1e10) / 1e10);
}

// ── Ranking ─────────────────────────────────────────────────────────────────

// catalog: { apps: [...], actions: [...], settings: [...], files: [...] }
// Each entry: { kind, id, title, subtitle, keywords, icon, hue, chroma, description, args }
// Returns ranked results with a `section` field, at most `limit` entries.
function search(query, catalog, limit) {
    const max = limit || 12;
    const q = (query || "").trim();
    const results = [];

    if (q.length === 0) {
        for (const app of catalog.apps)
            results.push(Object.assign({ section: "Apps", score: 0 }, app));
        for (const action of catalog.actions.slice(0, 4))
            results.push(Object.assign({ section: "Suggested actions", score: 0 }, action));
        return results.slice(0, max);
    }

    const value = evaluate(q);
    if (value !== null) {
        results.push({
            section: "Calculator", kind: "calc", id: "calc", score: 2000,
            title: formatNumber(value), subtitle: q + " =", icon: "calculator",
            hue: 200, chroma: 0.12, description: "Press Enter to copy the result.", keywords: "",
            action: "", args: "{}"
        });
    }

    const groups = [
        ["Apps", catalog.apps, 1.15],
        ["Actions", catalog.actions, 1.0],
        ["Settings", catalog.settings, 0.95],
        ["Files", catalog.files, 0.9]
    ];
    for (const group of groups) {
        for (const entry of group[1]) {
            const score = matchEntry(q, entry.title, entry.keywords) * group[2];
            if (score > 0)
                results.push(Object.assign({ section: group[0], score: score }, entry));
        }
    }

    results.sort((a, b) => b.score - a.score);
    const top = results.slice(0, max);
    // Keep sections contiguous (ListView sections require grouping) while
    // ordering sections by their best result.
    const order = [];
    for (const r of top)
        if (order.indexOf(r.section) < 0)
            order.push(r.section);
    return top.sort((a, b) => order.indexOf(a.section) - order.indexOf(b.section) || b.score - a.score);
}
