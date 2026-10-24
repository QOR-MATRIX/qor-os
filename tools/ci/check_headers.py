#!/usr/bin/env python3
"""Verifies every QTOS source file carries an SPDX license identifier.

Usage: python tools/ci/check_headers.py   (exit code 1 lists offenders)
"""

from __future__ import annotations

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROOTS = ["kit", "services", "sim", "prism", "apps", "platform", "tools", "tests"]
EXTENSIONS = {".cpp", ".h", ".qml", ".frag", ".vert", ".js", ".rep"}
MARKER = "SPDX-License-Identifier:"
HEAD_LINES = 5


def main() -> int:
    missing: list[str] = []
    checked = 0
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in EXTENSIONS or not path.is_file():
                continue
            checked += 1
            with path.open(encoding="utf-8") as handle:
                head = [next(handle, "") for _ in range(HEAD_LINES)]
            if not any(MARKER in line for line in head):
                missing.append(str(path.relative_to(REPO)))
    if missing:
        print(f"Missing '{MARKER}' in the first {HEAD_LINES} lines of:", file=sys.stderr)
        for item in missing:
            print(f"  {item}", file=sys.stderr)
        return 1
    print(f"SPDX headers present in all {checked} source files.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
