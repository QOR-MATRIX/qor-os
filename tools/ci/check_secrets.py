#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Refuses a secret in the repository's files, before it reaches a commit.

AGENTS.md section 6: nothing secret reaches a commit. This runs in pre-commit and
in CI, over every file git tracks (and, with --staged, what is about to be
committed), with Python alone, so it needs no download on the owner's machine.
It looks for the shapes of the credentials this project meets, and for files
named like keys. It never prints a value: only the file, the line and the kind.

CI's Security workflow also runs gitleaks over the whole history, which knows far
more shapes; this is the quick one that stops a secret at the door.

A file that holds a shape on purpose (a test's made-up key) is listed in
tools/ci/secrets-allowed.txt with its reason, one "path: reason" a line. A line
of its own can say "qor-secrets: allow" instead, in a comment beside a made-up
value.

Usage: python tools/ci/check_secrets.py [--staged]   (exit code 1 lists offenders)
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
ALLOWED = "tools/ci/secrets-allowed.txt"
MARK = "qor-secrets: allow"

# Files larger than this are assets or fixtures, not where a credential is pasted.
LARGEST = 2 * 1024 * 1024

SHAPES: list[tuple[str, re.Pattern[str]]] = [
    ("a private key", re.compile(r"-----BEGIN (?:[A-Z0-9]+ )*PRIVATE KEY(?: BLOCK)?-----")),
    ("a GitHub token", re.compile(r"\b(?:gh[pousr]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{40,})\b")),
    ("an Anthropic key", re.compile(r"\bsk-ant-[A-Za-z0-9_-]{20,}")),
    ("a provider's secret key", re.compile(r"\bsk-(?:proj-)?[A-Za-z0-9]{32,}\b")),
    ("an AWS access key", re.compile(r"\b(?:AKIA|ASIA)[0-9A-Z]{16}\b")),
    ("a Google API key", re.compile(r"\bAIza[0-9A-Za-z_-]{35}\b")),
    ("a Slack token", re.compile(r"\bxox[abeoprs]-[0-9A-Za-z-]{10,}")),
    ("a Stripe key", re.compile(r"\b[rs]k_live_[0-9A-Za-z]{16,}\b")),
    ("a Resend key", re.compile(r"\bre_[0-9A-Za-z]{8}_[0-9A-Za-z_]{16,}\b")),
    ("a signed token (JWT)", re.compile(r"\beyJ[A-Za-z0-9_-]{10,}\.eyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}")),
]

NAMES = re.compile(r"(^|/)(\.env(\.[^/]*)?|qtaccount\.ini|id_(rsa|dsa|ecdsa|ed25519)|[^/]*\.(pem|key|p12|pfx|pk8))$")
# The example of an environment file names its variables and holds no values.
NAMES_FINE = re.compile(r"\.env\.example$")


def allowed(repo: Path) -> set[str]:
    """The files that hold a shape on purpose, with a reason given."""
    listed: set[str] = set()
    file = repo / ALLOWED
    if not file.is_file():
        return listed
    for line in file.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        path, _, reason = line.partition(":")
        if not reason.strip():
            raise SystemExit(f"{ALLOWED}: '{path}' gives no reason")
        listed.add(path.strip())
    return listed


def scan(path: str, data: bytes) -> list[tuple[int, str]]:
    """Each (line, kind) found in one file's content. A line of 0 is the file itself."""
    found: list[tuple[int, str]] = []
    if NAMES.search(path) and not NAMES_FINE.search(path):
        found.append((0, "a file named like a key or a secret"))
    if len(data) > LARGEST or b"\0" in data[:8192]:
        return found
    text = data.decode("utf-8", "replace")
    for number, line in enumerate(text.splitlines(), 1):
        if MARK in line:
            continue
        for kind, shape in SHAPES:
            if shape.search(line):
                found.append((number, kind))
    return found


def git(repo: Path, *arguments: str) -> bytes:
    return subprocess.run(["git", "-C", str(repo), *arguments], check=True, capture_output=True).stdout


def tracked(repo: Path) -> dict[str, bytes]:
    """Every tracked file that exists, as it stands in the working tree."""
    files: dict[str, bytes] = {}
    for name in git(repo, "ls-files", "-z").split(b"\0"):
        path = name.decode("utf-8")
        file = repo / path
        if path and file.is_file():
            files[path] = file.read_bytes()
    return files


def staged(repo: Path) -> dict[str, bytes]:
    """What is about to be committed, as it stands in the index."""
    files: dict[str, bytes] = {}
    names = git(repo, "diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z")
    for name in names.split(b"\0"):
        path = name.decode("utf-8")
        if path:
            files[path] = git(repo, "show", f":{path}")
    return files


def check(repo: Path, files: dict[str, bytes]) -> list[str]:
    """One line for each offender, none holding a value."""
    skip = allowed(repo)
    offenders: list[str] = []
    for path, data in sorted(files.items()):
        if path in skip:
            continue
        for number, kind in scan(path, data):
            offenders.append(f"{path}:{number}: {kind}" if number else f"{path}: {kind}")
    return offenders


def main(argv: list[str]) -> int:
    files = staged(REPO) if "--staged" in argv else tracked(REPO)
    offenders = check(REPO, files)
    for line in offenders:
        print(line, file=sys.stderr)
    if offenders:
        print(
            f"{len(offenders)} possible secret(s). Remove it, and treat a real one as exposed: replace it. "
            f"A made-up value goes in {ALLOWED} with its reason.",
            file=sys.stderr,
        )
        return 1
    print(f"{len(files)} files read: no secret found")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
