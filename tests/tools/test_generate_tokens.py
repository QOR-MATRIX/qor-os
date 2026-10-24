"""Tests for tools/tokens/generate_tokens.py."""

from __future__ import annotations

import importlib.util
import json
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("generate_tokens", REPO / "tools" / "tokens" / "generate_tokens.py")
gen = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(gen)


class ColorMath(unittest.TestCase):
    def test_white_and_black(self) -> None:
        self.assertEqual(gen.to_hex(1.0, 0.0, 0.0, 1.0), "#ffffff")
        self.assertEqual(gen.to_hex(0.0, 0.0, 0.0, 1.0), "#000000")

    def test_reference_red(self) -> None:
        # sRGB red is approximately OKLCH(0.628, 0.2577, 29.23).
        r, g, b = gen.oklch_to_srgb(0.62796, 0.25768, 29.2339)
        self.assertAlmostEqual(r, 1.0, places=2)
        self.assertAlmostEqual(g, 0.0, places=2)
        self.assertAlmostEqual(b, 0.0, places=2)

    def test_gamut_mapping_stays_in_range(self) -> None:
        for hue in range(0, 360, 15):
            for channel in gen.oklch_to_srgb(0.7, 0.5, hue):
                self.assertGreaterEqual(channel, 0.0)
                self.assertLessEqual(channel, 1.0)

    def test_alpha_suffix(self) -> None:
        self.assertTrue(gen.to_hex(0.5, 0.0, 0.0, 0.5).endswith("@ 50%"))


class Generation(unittest.TestCase):
    def setUp(self) -> None:
        self.tokens = json.loads(gen.TOKENS_JSON.read_text(encoding="utf-8"))

    def test_every_role_has_light_and_dark(self) -> None:
        for name, role in self.tokens["color"]["roles"].items():
            self.assertEqual(len(role["light"]), 4, name)
            self.assertEqual(len(role["dark"]), 4, name)

    def test_qml_declares_every_role(self) -> None:
        qml = gen.emit_qml(self.tokens)
        self.assertIn("pragma Singleton", qml)
        for name in self.tokens["color"]["roles"]:
            self.assertIn(f"readonly property var {name}:", qml)

    def test_generated_files_are_current(self) -> None:
        self.assertEqual(gen.TOKENS_QML.read_text(encoding="utf-8"), gen.emit_qml(self.tokens),
                         "kit/Tokens.qml is stale: run tools/tokens/generate_tokens.py")
        self.assertEqual(gen.TOKENS_CPP.read_text(encoding="utf-8"), gen.emit_cpp(self.tokens),
                         "platform/common/lumentokens.h is stale: run tools/tokens/generate_tokens.py")
        self.assertEqual(gen.TOKENS_MD.read_text(encoding="utf-8"), gen.emit_markdown(self.tokens),
                         "docs/design/tokens.md is stale: run tools/tokens/generate_tokens.py")


if __name__ == "__main__":
    unittest.main()
