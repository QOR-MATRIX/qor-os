#!/usr/bin/env python3
"""Unattended install of the commercial Qt used by QTOS CI.

Downloads the Qt Online Installer and installs the packages listed in
.github/qt-packages.json for the requested platform. Credentials come from the
environment (GitHub Actions secrets) and are never printed:

    QT_INSTALLER_JWT_TOKEN                      preferred (Qt Account → JWT)
    QT_INSTALLER_EMAIL + QT_INSTALLER_PASSWORD  fallback

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


def run_installer(installer: Path, root: Path, packages: list[str], required: bool) -> bool:
    command = [
        str(installer), "--root", str(root),
        "--accept-licenses", "--accept-obligations", "--default-answer", "--confirm-command",
        "--auto-answer", "telemetry-question=No",
    ]
    if not os.environ.get("QT_INSTALLER_JWT_TOKEN"):
        email = os.environ.get("QT_INSTALLER_EMAIL")
        password = os.environ.get("QT_INSTALLER_PASSWORD")
        if not email or not password:
            sys.exit("No Qt credentials: set QT_INSTALLER_JWT_TOKEN or QT_INSTALLER_EMAIL/QT_INSTALLER_PASSWORD.")
        command += ["--email", email, "--pw", password]
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

    with tempfile.TemporaryDirectory() as tmp:
        installer = download(INSTALLERS[args.platform], Path(tmp))
        run_installer(installer, args.root, spec["packages"], required=True)
        for package in spec.get("optional", []):
            if not run_installer(installer, args.root, [package], required=False):
                print(f"Optional package not installed: {package}")

    kit = args.root / manifest["version"] / spec["kit"]
    print(f"Qt kit: {kit}")
    github_env = os.environ.get("GITHUB_ENV")
    if github_env:
        with open(github_env, "a", encoding="utf-8") as handle:
            handle.write(f"QTOS_QT_DIR={kit}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
