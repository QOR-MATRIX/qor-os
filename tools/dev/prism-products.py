#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Prism hosting the products, with nothing shown and no graphics card.

Runs Prism on Qt's offscreen platform, drawing in software, in a runtime
folder of its own, with stand-ins for ARQADE and QOR Media (prism-test-arqade
and prism-test-media) where QOR_ARQADE and QOR_MEDIA point. A JSON UI script
(`prism --script`) opens them as a person would and checks that Prism starts
them, places their windows and names them from its own records. ctest runs it
as prism_products.

    tools/dev/prism-products.py --prism <prism> --arqade <exe> --media <exe> \\
        --script tests/prism/products.e2e.json

Exit status: Prism's (0 when every step passed, 4 when one failed), or 100+
when it could not run.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prism", required=True)
    parser.add_argument("--arqade", required=True)
    parser.add_argument("--media", required=True)
    parser.add_argument("--script", required=True)
    parser.add_argument("--timeout", type=int, default=100)
    args = parser.parse_args()

    for program in (args.prism, args.arqade, args.media):
        if not os.access(program, os.X_OK):
            print(f"Cannot run {program}", file=sys.stderr)
            return 100

    with tempfile.TemporaryDirectory(prefix="qor-prism-products-") as home:
        runtime = os.path.join(home, "runtime")
        os.mkdir(runtime, 0o700)
        env = dict(os.environ)
        # Nothing of the session it runs in: no display to show on, and no
        # socket a real session uses.
        for name in ("DISPLAY", "WAYLAND_DISPLAY", "XDG_ACTIVATION_TOKEN", "QT_QPA_PLATFORM"):
            env.pop(name, None)
        env.update({
            "XDG_RUNTIME_DIR": runtime,
            "XDG_CONFIG_HOME": os.path.join(home, "config"),
            "XDG_DATA_HOME": os.path.join(home, "data"),
            "QT_QPA_PLATFORM": "offscreen",
            "QT_QUICK_BACKEND": "software",
            # Software only, should anything ask for OpenGL: under WSL a run
            # on the graphics card can hang the PC (tools/dev/products.sh).
            "GALLIUM_DRIVER": "llvmpipe",
            "LIBGL_ALWAYS_SOFTWARE": "1",
            "QT_FORCE_STDERR_LOGGING": "1",
            "QOR_AUDIO_OUTPUT": "null",
            "QOR_GAMEPADS": "virtual",
            "QOR_TOUCHPAD": "off",
            "QOR_ARQADE": os.path.abspath(args.arqade),
            "QOR_MEDIA": os.path.abspath(args.media),
        })
        command = [args.prism, "--unlocked", "--size", "1920x1200", "--socket", "wayland-products",
                   "--script", os.path.abspath(args.script)]
        try:
            return subprocess.run(command, env=env, timeout=args.timeout, check=False).returncode
        except subprocess.TimeoutExpired:
            print(f"Prism did not finish the script in {args.timeout} s", file=sys.stderr)
            return 101


if __name__ == "__main__":
    sys.exit(main())
