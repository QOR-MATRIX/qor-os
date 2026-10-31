#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Prism hosting the products, with nothing shown and no graphics card.

Runs Prism on Qt's offscreen platform, drawing in software, in a runtime
folder of its own, with stand-ins for ARQADE, QOR Media, QQ Studio and the QQ
Player (prism-test-arqade, prism-test-media, prism-test-qq-studio and
prism-test-qq-player) where QOR_ARQADE, QOR_MEDIA, QOR_QQ_STUDIO and
QOR_QQ_PLAYER point. A JSON UI script (`prism --script`) opens them as a person
would and checks that Prism starts them, lays out their windows and names them
from its own records. ctest runs it as prism_products. With --client, it also
starts a Wayland client of Prism's of its own, not one Prism knows, as
prism_untrusted does.

    tools/dev/prism-products.py --prism <prism> --arqade <exe> --media <exe> \\
        --qq-studio <exe> --qq-player <exe> --script tests/prism/products.e2e.json

Exit status: Prism's (0 when every step passed, 4 when one failed), or 100+
when it could not run.
"""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
import tempfile
import time


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--prism", required=True)
    parser.add_argument("--arqade", required=True)
    parser.add_argument("--media", required=True)
    parser.add_argument("--qq-studio", required=True)
    parser.add_argument("--qq-player", required=True)
    parser.add_argument("--script", required=True)
    parser.add_argument("--timeout", type=int, default=100)
    parser.add_argument("--client", action="append", default=[], help="A command line for a client that connects by itself")
    args = parser.parse_args()

    for program in (args.prism, args.arqade, args.media, args.qq_studio, args.qq_player):
        if not os.access(program, os.X_OK):
            print(f"Cannot run {program}", file=sys.stderr)
            return 100

    with tempfile.TemporaryDirectory(prefix="qor-prism-products-") as home:
        runtime = os.path.join(home, "runtime")
        os.mkdir(runtime, 0o700)
        # The Library's (prism/shell/library.h): a QQ game where QOR OS keeps its
        # own, in the run's data folder, and the person's folders, the run's
        # own too, empty but for one video.
        scenes = os.path.join(home, "data", "qor-os", "games", "orb-run", "scenes")
        os.makedirs(scenes)
        with open(os.path.join(scenes, "orb-run.qml"), "w", encoding="utf-8") as scene:
            scene.write("import QQ\nScene {}\n")
        folders = {"DOCUMENTS": "Documents", "VIDEOS": "Videos", "MUSIC": "Music", "PICTURES": "Pictures"}
        for name in folders.values():
            os.makedirs(os.path.join(home, name))
        open(os.path.join(home, "Videos", "clip.mkv"), "wb").close()
        os.makedirs(os.path.join(home, "config"))
        with open(os.path.join(home, "config", "user-dirs.dirs"), "w", encoding="utf-8") as dirs:
            for key, name in folders.items():
                dirs.write(f'XDG_{key}_DIR="{os.path.join(home, name)}"\n')
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
            "QOR_QQ_STUDIO": os.path.abspath(args.qq_studio),
            "QOR_QQ_PLAYER": os.path.abspath(args.qq_player),
        })
        command = [args.prism, "--unlocked", "--size", "1920x1200", "--socket", "wayland-products",
                   "--script", os.path.abspath(args.script)]
        prism = subprocess.Popen(command, env=env)
        clients = []
        try:
            if args.client:
                socket = os.path.join(runtime, "wayland-products")
                deadline = time.monotonic() + 30
                while not os.path.exists(socket) and time.monotonic() < deadline and prism.poll() is None:
                    time.sleep(0.05)
                client_env = dict(env, WAYLAND_DISPLAY="wayland-products", QT_QPA_PLATFORM="wayland")
                clients = [subprocess.Popen(shlex.split(line), env=client_env) for line in args.client]
            return prism.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(f"Prism did not finish the script in {args.timeout} s", file=sys.stderr)
            return 101
        finally:
            for process in [prism, *clients]:
                if process.poll() is None:
                    process.kill()
                process.wait()


if __name__ == "__main__":
    sys.exit(main())
