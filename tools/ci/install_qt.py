#!/usr/bin/env python3
"""Unattended install of the commercial Qt used by QTOS CI.

Installs the kit and add-ons listed in .github/qt-packages.json for the
requested platform with the Qt Online Installer (or, when the root already has
an installation, its MaintenanceTool), then verifies that the expected CMake
packages exist. Credentials are never printed. They come from, in order:

    QT_INSTALLER_JWT_TOKEN                      preferred in CI (Qt Account → JWT)
    QT_INSTALLER_EMAIL + QT_INSTALLER_PASSWORD  CI fallback
    qtaccount.ini                               a machine already signed in to the Qt installer
                                                (Linux: ~/.local/share/Qt, Windows: %APPDATA%\\Qt,
                                                macOS: ~/Library/Application Support/Qt)

Usage:
    python tools/ci/install_qt.py --platform windows --root C:/Qt
    python tools/ci/install_qt.py --platform linux --root ~/Qt
"""

from __future__ import annotations

import argparse
import json
import os
import stat
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
MANIFEST = REPO / ".github" / "qt-packages.json"
INSTALLERS = {
    "windows": "https://download.qt.io/official_releases/online_installers/qt-online-installer-windows-x64-online.exe",
    "linux": "https://download.qt.io/official_releases/online_installers/qt-online-installer-linux-x64-online.run",
}


def download(url: str, directory: Path) -> Path:
    target = directory / url.rsplit("/", 1)[-1]
    print(f"Downloading {url}")
    urllib.request.urlretrieve(url, target)  # noqa: S310 - fixed https URL
    target.chmod(target.stat().st_mode | stat.S_IEXEC)
    return target


def account_file() -> Path:
    if sys.platform == "win32":
        return Path(os.environ.get("APPDATA", "")) / "Qt" / "qtaccount.ini"
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support" / "Qt" / "qtaccount.ini"
    return Path.home() / ".local" / "share" / "Qt" / "qtaccount.ini"


def run_installer(installer: Path, root: Path, packages: list[str], required: bool) -> bool:
    command = [
        str(installer), "--root", str(root),
        "--accept-licenses", "--accept-obligations", "--default-answer", "--confirm-command",
        "--auto-answer", "telemetry-question=No",
    ]
    email = os.environ.get("QT_INSTALLER_EMAIL")
    password = os.environ.get("QT_INSTALLER_PASSWORD")
    if os.environ.get("QT_INSTALLER_JWT_TOKEN"):
        pass  # the installer reads the token from the environment
    elif email and password:
        command += ["--email", email, "--pw", password]
    elif not account_file().is_file():
        sys.exit(
            "No Qt credentials: set QT_INSTALLER_JWT_TOKEN or QT_INSTALLER_EMAIL/QT_INSTALLER_PASSWORD, "
            f"or sign in to the Qt installer once on this machine ({account_file()})."
        )
    command += ["install", *packages]

    redacted = [("***" if i > 0 and command[i - 1] in ("--email", "--pw") else part) for i, part in enumerate(command)]
    print("Running:", " ".join(redacted), flush=True)
    result = subprocess.run(command, check=False)
    if result.returncode != 0 and required:
        sys.exit(f"Qt installer failed with exit code {result.returncode}")
    return result.returncode == 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--platform", choices=sorted(INSTALLERS), required=True)
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    spec = manifest[args.platform]
    args.root.mkdir(parents=True, exist_ok=True)

    # The online installer only installs into an empty root; later additions go
    # through the MaintenanceTool it leaves behind.
    maintenance_tool = args.root / ("MaintenanceTool.exe" if sys.platform == "win32" else "MaintenanceTool")
    packages = spec["packages"] + manifest.get("addons", [])
    with tempfile.TemporaryDirectory() as tmp:
        installer = maintenance_tool if maintenance_tool.is_file() else download(INSTALLERS[args.platform], Path(tmp))
        run_installer(installer, args.root, packages, required=True)
        for package in spec.get("optional", []):
            if not run_installer(maintenance_tool, args.root, [package], required=False):
                print(f"Optional package not installed: {package}")

    # The installer exits 0 even when it skips a package it cannot resolve, so
    # check the result rather than trusting the exit code.
    kit = args.root / manifest["version"] / spec["kit"]
    cmake_dir = kit / "lib" / "cmake"
    missing = [name for name in manifest.get("cmake", []) if not (cmake_dir / name).is_dir()]
    if missing:
        sys.exit(f"Qt install incomplete, missing CMake packages in {cmake_dir}: {', '.join(missing)}")
    for name in spec.get("optionalCmake", []):
        if not (cmake_dir / name).is_dir():
            print(f"Optional module not available: {name}")
    print(f"Qt kit: {kit}")
    github_env = os.environ.get("GITHUB_ENV")
    if github_env:
        with open(github_env, "a", encoding="utf-8") as handle:
            handle.write(f"QTOS_QT_DIR={kit}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
