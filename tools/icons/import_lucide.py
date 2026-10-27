#!/usr/bin/env python3
"""Imports the curated Lucide icon subset into kit/icons.

Each icon is rewritten for GPU tinting: strokes become pure white (the
QOR.Kit `Icon` shader turns alpha into any theme color) and the stroke width
is set to Lumen's 1.75 px optical weight.

Usage::

    npm pack lucide-static            # or download the tarball from npm
    tar -xzf lucide-static-*.tgz
    python tools/icons/import_lucide.py package

The icon list lives in kit/icons/icons.txt.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ICON_DIR = REPO / "kit" / "icons"
ICON_LIST = ICON_DIR / "icons.txt"
STROKE_WIDTH = "1.75"


def wanted_icons() -> list[str]:
    names = []
    for raw in ICON_LIST.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line and not line.startswith("#"):
            names.append(line)
    return names


def convert(svg: str, name: str, version: str) -> str:
    svg = re.sub(r"<!--.*?-->\s*", "", svg, flags=re.S)
    svg = re.sub(r'\s+class="[^"]*"', "", svg)
    svg = svg.replace('stroke="currentColor"', 'stroke="#ffffff"')
    svg = re.sub(r'stroke-width="[^"]*"', f'stroke-width="{STROKE_WIDTH}"', svg)
    svg = re.sub(r"\n\s*", " ", svg).replace("> <", "><").strip()
    return f"<!-- Lucide '{name}' v{version} (ISC). Recolored for QOR OS. -->\n{svg}\n"


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(__doc__, file=sys.stderr)
        return 2
    package = Path(argv[0])
    version = json.loads((package / "package.json").read_text(encoding="utf-8"))["version"]
    source_dir = package / "icons"

    missing = []
    for name in wanted_icons():
        src = source_dir / f"{name}.svg"
        if not src.exists():
            missing.append(name)
            continue
        dst = ICON_DIR / f"{name}.svg"
        dst.write_text(convert(src.read_text(encoding="utf-8"), name, version), encoding="utf-8", newline="\n")

    license_src = package / "LICENSE"
    if license_src.exists():
        (ICON_DIR / "LICENSE.lucide.txt").write_text(license_src.read_text(encoding="utf-8"),
                                                    encoding="utf-8", newline="\n")

    if missing:
        print("Missing icons: " + ", ".join(missing), file=sys.stderr)
        return 1
    print(f"Imported {len(wanted_icons())} icons from lucide-static {version}.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
