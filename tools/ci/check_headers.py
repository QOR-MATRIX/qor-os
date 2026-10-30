#!/usr/bin/env python3
"""Verifies every QOR OS source file carries an SPDX license identifier.

The products developed here since QOR-0015 (QQ, ARQADE's app and games, QOR
Media) hold files that came from demiurge-chain under Apache-2.0. Those keep
their own notices and are not checked; LICENSES/from-demiurge-chain.txt lists
them. A file added to the products since carries QOR OS's header like any
other, and the list may name only files that are still in the products.

Usage: python tools/ci/check_headers.py   (exit code 1 lists offenders)
"""

from __future__ import annotations

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ROOTS = ["kit", "services", "sim", "prism", "apps", "platform", "tools", "tests", "store", "input", "audio", "products"]
PRODUCTS = ("products/qq/", "products/arqade/qt/", "products/arqade/games/", "tools/qor-media/")
FROM_DEMIURGE = "LICENSES/from-demiurge-chain.txt"
EXTENSIONS = {".cpp", ".h", ".qml", ".frag", ".vert", ".js", ".rep"}
MARKER = "SPDX-License-Identifier:"
HEAD_LINES = 5


def from_demiurge(repo: Path) -> set[str]:
    """The files listed as having come from demiurge-chain."""
    lines = (repo / FROM_DEMIURGE).read_text(encoding="utf-8").splitlines()
    return {line.strip() for line in lines if line.strip() and not line.startswith("#")}


def check(repo: Path) -> tuple[int, list[str], list[str]]:
    """Returns how many files were checked, those without a header, and list entries that are not product files."""
    listed = from_demiurge(repo)
    missing: list[str] = []
    checked = 0
    for root in ROOTS:
        for path in sorted((repo / root).rglob("*")):
            if path.suffix not in EXTENSIONS or not path.is_file():
                continue
            if path.relative_to(repo).as_posix() in listed:
                continue
            checked += 1
            with path.open(encoding="utf-8") as handle:
                head = [next(handle, "") for _ in range(HEAD_LINES)]
            if not any(MARKER in line for line in head):
                missing.append(path.relative_to(repo).as_posix())
    stale = sorted(entry for entry in listed if not entry.startswith(PRODUCTS) or not (repo / entry).is_file())
    return checked, missing, stale


def main() -> int:
    checked, missing, stale = check(REPO)
    if missing:
        print(f"Missing '{MARKER}' in the first {HEAD_LINES} lines of:", file=sys.stderr)
        for item in missing:
            print(f"  {item}", file=sys.stderr)
    if stale:
        print(f"{FROM_DEMIURGE} names what is not a file in the products:", file=sys.stderr)
        for item in stale:
            print(f"  {item}", file=sys.stderr)
    if missing or stale:
        return 1
    print(f"SPDX headers present in all {checked} source files.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
