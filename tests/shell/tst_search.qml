// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtTest
import "../../prism/shell/search.js" as Search

TestCase {
    name: "BeamSearch"

    function test_fuzzyScore_data() {
        return [
            { tag: "exact", query: "files", text: "Files", min: 1000 },
            { tag: "prefix", query: "fil", text: "Files", min: 800 },
            { tag: "word start", query: "light", text: "Switch to Light Appearance", min: 600 },
            { tag: "subsequence", query: "stg", text: "Settings", min: 1 },
            { tag: "no match", query: "xyz", text: "Settings", min: 0, max: 0 }
        ];
    }

    function test_fuzzyScore(data) {
        const score = Search.fuzzyScore(data.query, data.text);
        verify(score >= data.min, "score " + score);
        if (data.max !== undefined)
            compare(score, data.max);
    }

    function test_rankingPrefersPrefix() {
        verify(Search.fuzzyScore("mus", "Music") > Search.fuzzyScore("mus", "Monitor usage"));
        verify(Search.fuzzyScore("set", "Settings") > Search.fuzzyScore("set", "Reset network"));
    }

    function test_calculator_data() {
        return [
            { tag: "addition", expr: "2+2", result: 4 },
            { tag: "precedence", expr: "2+3*4", result: 14 },
            { tag: "parentheses", expr: "(2+3)*4", result: 20 },
            { tag: "power right assoc", expr: "2^3^2", result: 512 },
            { tag: "unary minus", expr: "-3+5", result: 2 },
            { tag: "decimals", expr: "0.1*3", result: 0.30000000000000004 },
            { tag: "functions", expr: "sqrt(16)+abs(-2)", result: 6 },
            { tag: "constants", expr: "2*pi", result: 2 * Math.PI },
            { tag: "unicode ops", expr: "6×7÷2", result: 21 }
        ];
    }

    function test_calculator(data) {
        fuzzyCompare(Search.evaluate(data.expr), data.result, 1e-9);
    }

    function test_calculatorRejects_data() {
        return [
            { tag: "plain number", expr: "42" },
            { tag: "words", expr: "settings" },
            { tag: "unknown identifier", expr: "2+foo" },
            { tag: "dangling operator", expr: "2+" },
            { tag: "unbalanced", expr: "(2+3" },
            { tag: "code injection", expr: "1+alert(1)" },
            { tag: "division by zero", expr: "1/0" }
        ];
    }

    function test_calculatorRejects(data) {
        compare(Search.evaluate(data.expr), null);
    }

    function test_searchGroupsSections() {
        const catalog = {
            apps: [{ kind: "app", id: "music", title: "Music", keywords: "audio" },
                   { kind: "app", id: "monitor", title: "Monitor", keywords: "system" }],
            actions: [{ kind: "action", id: "media.toggle", title: "Play Music", keywords: "media" }],
            settings: [{ kind: "setting", id: "sound", title: "Settings › Sound", keywords: "volume audio" }],
            files: []
        };
        const results = Search.search("mu", catalog, 10);
        verify(results.length >= 2);
        compare(results[0].id, "music");
        // Sections must be contiguous for ListView section headers.
        const seen = [];
        let last = "";
        for (const r of results) {
            if (r.section !== last) {
                verify(seen.indexOf(r.section) < 0, "section " + r.section + " split");
                seen.push(r.section);
                last = r.section;
            }
        }
    }

    function test_searchCalculatorFirst() {
        const results = Search.search("12*12", { apps: [], actions: [], settings: [], files: [] }, 5);
        compare(results[0].kind, "calc");
        compare(results[0].title, "144");
    }

    function test_emptyQuerySuggests() {
        const catalog = { apps: [{ kind: "app", id: "a", title: "A" }], actions: [], settings: [], files: [] };
        compare(Search.search("", catalog, 5).length, 1);
    }
}
