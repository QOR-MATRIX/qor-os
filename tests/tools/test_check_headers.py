"""Tests for tools/ci/check_headers.py."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("check_headers", REPO / "tools" / "ci" / "check_headers.py")
headers = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(headers)

HEADER = "// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary\n"


class Products(unittest.TestCase):
    def setUp(self) -> None:
        self.dir = tempfile.TemporaryDirectory()
        self.repo = Path(self.dir.name)
        self.write("kit/Button.qml", HEADER + "Item {}\n")
        self.write("products/qq/runtime/clips.cpp", '#include "clips.h"\n')
        self.list("products/qq/runtime/clips.cpp")

    def tearDown(self) -> None:
        self.dir.cleanup()

    def write(self, path: str, text: str) -> None:
        (self.repo / path).parent.mkdir(parents=True, exist_ok=True)
        (self.repo / path).write_text(text, encoding="utf-8")

    def list(self, *paths: str) -> None:
        self.write(headers.FROM_DEMIURGE, "# Files that came from demiurge-chain\n\n" + "".join(p + "\n" for p in paths))

    def test_a_listed_file_keeps_its_own_notice(self) -> None:
        self.assertEqual(headers.check(self.repo), (1, [], []))

    def test_a_file_added_to_a_product_needs_the_header(self) -> None:
        self.write("products/qq/runtime/new.cpp", "int x;\n")
        self.write("tools/qor-media/src/new.h", HEADER + "#pragma once\n")
        checked, missing, stale = headers.check(self.repo)
        self.assertEqual((checked, missing, stale), (3, ["products/qq/runtime/new.cpp"], []))

    def test_a_listed_file_that_is_gone_is_reported(self) -> None:
        self.list("products/qq/runtime/clips.cpp", "products/qq/runtime/gone.cpp")
        self.assertEqual(headers.check(self.repo)[2], ["products/qq/runtime/gone.cpp"])

    def test_only_the_products_may_be_listed(self) -> None:
        self.write("kit/Old.qml", "Item {}\n")
        self.list("products/qq/runtime/clips.cpp", "kit/Old.qml")
        checked, missing, stale = headers.check(self.repo)
        self.assertEqual((missing, stale), ([], ["kit/Old.qml"]))

    def test_the_repository_passes(self) -> None:
        checked, missing, stale = headers.check(REPO)
        self.assertEqual((missing, stale), ([], []))
        self.assertGreater(checked, 0)


if __name__ == "__main__":
    unittest.main()
