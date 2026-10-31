"""Tests for tools/ci/check_secrets.py. The values here are built at run time, so this file holds no secret's shape."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("check_secrets", REPO / "tools" / "ci" / "check_secrets.py")
secrets_check = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(secrets_check)

BEGIN = "-----BEGIN "
FILLER = "a1B2c3D4e5F6g7H8i9J0" * 3


SECRETS = {
    "a private key": BEGIN + "PRIVATE KEY-----",
    "an RSA private key": BEGIN + "RSA PRIVATE KEY-----",
    "a GitHub token": "gh" + "p_" + FILLER[:36],
    "a fine-grained GitHub token": "github" + "_pat_" + FILLER[:45],
    "an Anthropic key": "sk-" + "ant-" + FILLER[:30],
    "an AWS key": "AK" + "IA" + "ABCDEFGHIJKLMNOP",
    "a Google key": "AI" + "za" + FILLER[:35],
    "a signed token": "ey" + "J" + FILLER[:20] + ".ey" + "J" + FILLER[:20] + "." + FILLER[:20],
}


class Shapes(unittest.TestCase):
    def test_every_shape_is_found_and_its_value_never_said(self) -> None:
        for name, value in SECRETS.items():
            found = secrets_check.scan("src/app.cpp", f"const auto x = {value};\n".encode())
            self.assertTrue(found, name)
            self.assertNotIn(value, repr(found), name)

    def test_the_line_is_reported(self) -> None:
        found = secrets_check.scan("a.txt", f"one\ntwo\n{SECRETS['a private key']}\n".encode())
        self.assertEqual(found, [(3, "a private key")])

    def test_what_only_looks_alike_is_not_a_secret(self) -> None:
        for text in (
            "BEGIN PUBLIC KEY",
            "-----BEGIN PUBLIC KEY-----",
            "ghp_short",
            "the sk-ant- prefix alone",
            "AKIA is how such keys begin",
            "secret-service",
        ):
            self.assertEqual(secrets_check.scan("a.md", text.encode()), [], text)

    def test_a_marked_line_is_a_made_up_value(self) -> None:
        line = f"{SECRETS['a GitHub token']}  // {secrets_check.MARK}\n"
        self.assertEqual(secrets_check.scan("a.cpp", line.encode()), [])

    def test_files_named_like_keys_are_refused_and_the_example_is_not(self) -> None:
        for path in ("signing/developer.pem", ".env", "tools/.env.local", "qtaccount.ini", "id_ed25519", "a/b.p12"):
            self.assertEqual(secrets_check.scan(path, b"")[0][0], 0, path)
        for path in ("store/keys/store.pub", ".env.example", "keyboard.cpp", "environment.txt"):
            self.assertEqual(secrets_check.scan(path, b""), [], path)

    def test_a_binary_is_not_read_as_text(self) -> None:
        self.assertEqual(secrets_check.scan("a.png", b"\x89PNG\0" + SECRETS["a private key"].encode()), [])


class Allowed(unittest.TestCase):
    def setUp(self) -> None:
        self.dir = tempfile.TemporaryDirectory()
        self.repo = Path(self.dir.name)
        (self.repo / "tools" / "ci").mkdir(parents=True)

    def tearDown(self) -> None:
        self.dir.cleanup()

    def allow(self, text: str) -> None:
        (self.repo / secrets_check.ALLOWED).write_text(text, encoding="utf-8")

    def test_an_allowed_file_passes_and_another_does_not(self) -> None:
        self.allow("# Made-up keys.\ntests/fake.txt: a made-up key for the store's tests\n")
        files = {path: SECRETS["a private key"].encode() for path in ("tests/fake.txt", "src/real.txt")}
        self.assertEqual(secrets_check.check(self.repo, files), ["src/real.txt:1: a private key"])

    def test_an_entry_without_a_reason_is_refused(self) -> None:
        self.allow("tests/fake.txt\n")
        with self.assertRaises(SystemExit):
            secrets_check.check(self.repo, {})


class ThisRepository(unittest.TestCase):
    def test_no_secret_is_in_the_tracked_files(self) -> None:
        offenders = secrets_check.check(REPO, secrets_check.tracked(REPO))
        self.assertEqual(offenders, [])


if __name__ == "__main__":
    unittest.main()
