#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
"""End-to-end window management in Prism with real Wayland clients.

Starts a nested Prism (in a Wayland session or WSLg) with a JSON UI script
(`prism --script`). It connects two clients: prism-test-client, a Qt app with
server-side decorations and a menu at a known spot, and weston-terminal, which
draws its own decorations. The script drives them with synthetic pointer and
key input that takes the same path as a person's: maximise, restore, snap,
move by a client's own title bar, minimise, restore from the Shelf, popup,
Overview and close. This file then checks the window-manager state the script
recorded at each step.

    tools/dev/prism-e2e.py --build-dir ~/qtos-build/linux-release [--keep]

Exit status is the number of failed checks (100+ for setup errors).
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

A = {"title": "Prism Test A"}
TERMINAL = {"appId": "weston"}
TITLE_BAR = 40


def script(with_terminal: bool) -> list[dict]:
    steps: list[dict] = [
        {"waitFor": {"windows": 2 if with_terminal else 1, "timeout": 15000}},
        {"wait": 1500},
        {"state": "00-mapped.json"},
        {"click": {"window": A, "part": "maximize"}},
        {"wait": 1500},
        {"state": "01-maximized.json"},
        {"click": {"window": A, "part": "maximize"}},
        {"wait": 1500},
        {"state": "02-unmaximized.json"},
        {"drag": {"from": {"window": A, "part": "titlebar", "at": [200, 20]}, "to": [3, 500], "steps": 24}},
        {"wait": 900},
        {"state": "03-snapped.json"},
    ]
    if with_terminal:
        steps += [
            # On the terminal's own title bar, right of the test client, which
            # is snapped to the left half at this point and would get the press.
            {"drag": {"from": {"window": TERMINAL, "part": "content", "at": [600, 14]}, "by": [150, 90],
                      "steps": 20}},
            {"wait": 600},
            {"state": "04-terminal-moved.json"},
        ]
    steps += [
        {"click": {"window": A, "part": "minimize"}},
        {"wait": 900},
        {"state": "05-minimized.json"},
        {"restore": A},
        {"wait": 900},
        {"state": "06-restored.json"},
        # Shrink far below the client's minimum (480 x 320) with the corner grip.
        {"drag": {"from": {"window": A, "part": "frame", "at": [-6, -6]}, "by": [-700, -700], "steps": 20}},
        {"wait": 900},
        {"state": "06b-resized.json"},
        # The test client's "Menu" button: (20, 20), 120 × 32, in its window.
        {"click": {"window": A, "part": "content", "at": [80, 36]}},
        {"waitFor": {"popups": 1, "timeout": 5000}},
        {"wait": 400},
        {"screenshot": "07-popup.png"},
        {"state": "07-popup.json"},
        {"key": "Escape"},
        {"waitFor": {"popups": 0, "timeout": 5000}},
        {"state": "08-popup-closed.json"},
        {"call": "setSurface", "args": ["overview"]},
        {"wait": 1000},
        {"screenshot": "09-overview.png"},
        {"state": "09-overview.json"},
        {"call": "setSurface", "args": [""]},
        {"wait": 800},
        {"click": {"window": A, "part": "close"}},
        {"waitFor": {"gone": A, "timeout": 8000}},
        {"state": "10-closed.json"},
    ]
    if with_terminal:
        steps += [
            {"close": TERMINAL},
            {"waitFor": {"windows": 0, "timeout": 8000}},
            {"state": "11-all-closed.json"},
        ]
    return steps


class Checks:
    def __init__(self) -> None:
        self.failures = 0

    def check(self, name: str, ok: bool, detail: str = "") -> None:
        print(f"  {'ok  ' if ok else 'FAIL'}  {name}" + (f"  ({detail})" if detail and not ok else ""))
        self.failures += 0 if ok else 1


def find(state: dict, spec: dict) -> dict | None:
    for w in state["windows"]:
        if w["closing"]:
            continue
        if ("title" in spec and spec["title"] in w["title"]) or ("appId" in spec and spec["appId"] in w["appId"]):
            return w
    return None


def near(a: list[int], b: list[int], tolerance: int = 2) -> bool:
    return len(a) == len(b) and all(abs(x - y) <= tolerance for x, y in zip(a, b))


def verify(out: Path, with_terminal: bool) -> int:
    c = Checks()

    def load(name: str) -> dict:
        return json.loads((out / name).read_text())

    s = load("00-mapped.json")
    work = s["workArea"]
    a = find(s, A)
    print("Mapping and decorations:")
    c.check("test client mapped as a Wayland client", bool(a and a["isClient"]))
    c.check("Qt client gets Prism's title bar (xdg-decoration)", bool(a and a["decorated"]))
    c.check("client honours the configured size", bool(a and a["clientGeometry"][2:] == a["configuredSize"]),
            f"{a and a['clientGeometry']} vs {a and a['configuredSize']}")
    c.check("window placed inside the work area", bool(a and a["frameRect"][1] >= work[1]),
            f"frame {a and a['frameRect']} work area {work}")
    initial = a["frameRect"] if a else []
    c.check("app id from the client (xdg_toplevel.set_app_id)", bool(a and a["appId"] == "org.qtos.PrismTestA"),
            f"{a and a['appId']}")
    c.check("Shelf name from the app's desktop entry", bool(a and a["identity"]["name"] == "Prism Test App"),
            f"{a and a['identity']}")
    c.check("Shelf icon from the XDG icon theme",
            bool(a and a["identity"]["icon"].endswith("/hicolor/scalable/apps/org.qtos.PrismTestA.svg")),
            f"{a and a['identity']}")
    if with_terminal:
        t = find(s, TERMINAL)
        c.check("terminal keeps its own decorations (no Prism title bar)", bool(t and not t["decorated"]))

    print("Maximise and restore:")
    a = find(load("01-maximized.json"), A)
    c.check("maximise button maximises", bool(a and a["maximized"]))
    c.check("maximised frame fills the work area", bool(a and near(a["frameRect"], work)), f"{a and a['frameRect']}")
    c.check("client reconfigured to the work area", bool(a and a["clientGeometry"][2:] == [work[2], work[3] - TITLE_BAR]),
            f"{a and a['clientGeometry']}")
    a = find(load("02-unmaximized.json"), A)
    c.check("second click restores", bool(a and not a["maximized"] and near(a["frameRect"], initial)),
            f"{a and a['frameRect']} vs {initial}")

    print("Snap and move:")
    s = load("03-snapped.json")
    a = find(s, A)
    left_half = [work[0], work[1], work[2] // 2, work[3]]
    c.check("dragging the title bar to the left edge snaps to the left half",
            bool(a and near(a["frameRect"], left_half)), f"{a and a['frameRect']} vs {left_half}")
    if with_terminal:
        before = find(s, TERMINAL)
        after = find(load("04-terminal-moved.json"), TERMINAL)
        moved = (after and before and [after["frameRect"][0] - before["frameRect"][0],
                                       after["frameRect"][1] - before["frameRect"][1]])
        c.check("dragging the terminal's own title bar moves it (xdg_toplevel.move)",
                bool(moved and near(moved, [150, 90], 4)), f"moved by {moved}")

    print("Minimise and restore:")
    a = find(load("05-minimized.json"), A)
    c.check("minimise button minimises", bool(a and a["minimized"]))
    a = find(load("06-restored.json"), A)
    c.check("Shelf restore brings the client back focused", bool(a and not a["minimized"] and a["focused"]))

    print("Size limits:")
    a = find(load("06b-resized.json"), A)
    c.check("resizing stops at the client's minimum size",
            bool(a and near(a["frameRect"][2:], [480, 320 + TITLE_BAR])), f"{a and a['frameRect']}")
    c.check("client content matches its minimum", bool(a and a["clientGeometry"][2:] == [480, 320]),
            f"{a and a['clientGeometry']}")

    print("Popups:")
    s = load("07-popup.json")
    a = find(s, A)
    c.check("menu opens as an xdg_popup", len(s["popups"]) == 1, f"{len(s['popups'])} popups")
    if s["popups"] and a:
        content = a["contentOnScreen"]
        expected = [content[0] + 20, content[1] + 52]
        c.check("popup placed directly below the button", near(s["popups"][0][:2], expected, 3),
                f"{s['popups'][0][:2]} vs {expected}")
    c.check("Escape closes the popup", len(load("08-popup-closed.json")["popups"]) == 0)

    print("Overview:")
    s = load("09-overview.json")
    a = find(s, A)
    c.check("Overview shows client windows", bool(s["overview"] and a and a["inOverview"]))

    print("Close:")
    c.check("Prism's close button closes the client (xdg_toplevel.close)", find(load("10-closed.json"), A) is None)
    if with_terminal:
        c.check("closing a client-decorated window works", not load("11-all-closed.json")["windows"]
                or all(w["closing"] for w in load("11-all-closed.json")["windows"]))
    return c.failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--keep", action="store_true", help="keep the output directory (states, screenshots)")
    args = parser.parse_args()

    prism = args.build_dir / "prism" / "compositor" / "prism"
    client = args.build_dir / "tests" / "prism-test-client"
    for binary in (prism, client):
        if not binary.is_file():
            print(f"missing: {binary}")
            return 100
    if not (os.environ.get("WAYLAND_DISPLAY") or os.environ.get("DISPLAY")):
        print("no display: run inside a Wayland/X11 session or WSLg")
        return 101
    terminal = shutil.which("weston-terminal")
    if not terminal:
        print("weston-terminal not found: skipping the client-side decoration checks")

    out = Path(tempfile.mkdtemp(prefix="prism-e2e-"))
    (out / "script.json").write_text(json.dumps({"steps": script(terminal is not None)}, indent=1))
    # A desktop entry and icon for the test client, found through XDG_DATA_DIRS.
    data = out / "data"
    icons = data / "icons" / "hicolor" / "scalable" / "apps"
    (data / "applications").mkdir(parents=True)
    icons.mkdir(parents=True)
    (data / "applications" / "org.qtos.PrismTestA.desktop").write_text(
        "[Desktop Entry]\nType=Application\nName=Prism Test App\nIcon=org.qtos.PrismTestA\nExec=true\n")
    (icons / "org.qtos.PrismTestA.svg").write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">'
        '<rect width="64" height="64" rx="14" fill="#5a68e6"/></svg>')
    data_dirs = f"{data}:{os.environ.get('XDG_DATA_DIRS', '/usr/local/share:/usr/share')}"
    socket = f"wayland-prism-e2e-{os.getpid()}"
    log = (out / "prism.log").open("w")
    compositor = subprocess.Popen([str(prism), "--unlocked", "--socket", socket, "--size", "1600x1000",
                                   "--script", str(out / "script.json")], stdout=log, stderr=subprocess.STDOUT,
                                  env=dict(os.environ, XDG_DATA_DIRS=data_dirs))
    clients: list[subprocess.Popen] = []
    try:
        deadline = time.monotonic() + 15
        while "Prism is running" not in (out / "prism.log").read_text():
            if compositor.poll() is not None or time.monotonic() > deadline:
                print("prism did not start:\n" + (out / "prism.log").read_text())
                return 102
            time.sleep(0.1)
        env = dict(os.environ, WAYLAND_DISPLAY=socket, QT_QPA_PLATFORM="wayland")
        quiet = {"stdout": subprocess.DEVNULL, "stderr": subprocess.DEVNULL, "env": env}
        clients.append(subprocess.Popen([str(client), "--title", "Prism Test A", "--app-id", "org.qtos.PrismTestA",
                                         "--size", "640x420", "--min", "480x320"], **quiet))
        if terminal:
            time.sleep(0.8)
            clients.append(subprocess.Popen([terminal], **quiet))
        status = compositor.wait(timeout=150)
    except subprocess.TimeoutExpired:
        compositor.kill()
        print("prism script timed out")
        return 103
    finally:
        for p in clients:
            p.kill()
        log.close()

    if status != 0:
        print(f"prism --script failed ({status}):")
        print("\n".join(line for line in (out / "prism.log").read_text().splitlines() if "script:" in line))
        print(f"output kept in {out}")
        return 104

    failures = verify(out, terminal is not None)
    print(f"{failures} failure(s)")
    if args.keep or failures:
        print(f"output kept in {out}")
    else:
        shutil.rmtree(out)
    return failures


if __name__ == "__main__":
    sys.exit(main())
