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

    def test_luminance_cap(self) -> None:
        for hue in range(0, 360, 30):
            l = gen.cap_luminance(0.8, 0.15, hue, 0.175)
            self.assertLess(l, 0.8)
            self.assertLessEqual(gen.relative_luminance(l, 0.15, hue), 0.175 + 1e-6)
        self.assertEqual(gen.cap_luminance(0.3, 0.1, 120, 0.175), 0.3)


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

    def test_high_contrast_values_are_complete(self) -> None:
        for name, role in self.tokens["color"]["roles"].items():
            if "highContrast" in role:
                self.assertEqual(len(role["highContrast"]["light"]), 4, name)
                self.assertEqual(len(role["highContrast"]["dark"]), 4, name)

    def _contrast(self, fg: str, bg: str, appearance: str, high_contrast: bool, accent_hue: float) -> float:
        """WCAG contrast of role `fg` composited over opaque role `bg`."""
        roles = self.tokens["color"]["roles"]

        def resolved(name: str) -> tuple[tuple[float, float, float], float]:
            role = roles[name]
            values = (role.get("highContrast") or role)[appearance] if high_contrast else role[appearance]
            l, c, h, a = values
            h = (h + (accent_hue if role.get("followAccent") else 0)) % 360
            if "maxLuminance" in role:
                l = gen.cap_luminance(l, c, h, role["maxLuminance"])
            return gen.oklch_to_srgb(l, c, h), a

        def luminance(rgb: tuple[float, float, float]) -> float:
            lin = [x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4 for x in rgb]
            return 0.2126 * lin[0] + 0.7152 * lin[1] + 0.0722 * lin[2]

        (fg_rgb, alpha), (bg_rgb, _) = resolved(fg), resolved(bg)
        shown = tuple(alpha * f + (1 - alpha) * b for f, b in zip(fg_rgb, bg_rgb))
        hi, lo = sorted((luminance(shown), luminance(bg_rgb)), reverse=True)
        return (hi + 0.05) / (lo + 0.05)

    def test_text_on_accent_meets_aa_for_every_preset(self) -> None:
        for hue in self.tokens["color"]["accentPresets"]:
            for appearance in ("light", "dark"):
                for high_contrast in (False, True):
                    for fill in ("accent", "accentHover"):
                        ratio = self._contrast("onAccent", fill, appearance, high_contrast, hue)
                        self.assertGreaterEqual(ratio, 4.5, f"onAccent/{fill} {appearance} hc={high_contrast} hue={hue}")

    def test_high_contrast_text_targets(self) -> None:
        targets = [("textPrimary", 7), ("textSecondary", 7), ("textTertiary", 4.5), ("accentText", 7),
                   ("positive", 4.5), ("warning", 4.5), ("critical", 4.5), ("focusRing", 3)]
        for appearance in ("light", "dark"):
            for name, minimum in targets:
                for bg in ("background", "surface"):
                    ratio = self._contrast(name, bg, appearance, True, self.tokens["color"]["accentDefaultHue"])
                    self.assertGreaterEqual(ratio, minimum, f"{name}/{bg} {appearance}")

    # QOR-0016: Demiurge's one look on Lumen's roles.
    def test_the_products_colours_are_every_colour_of_demiurges_look(self) -> None:
        look = json.loads((REPO / "docs" / "design" / "tokens.json").read_text(encoding="utf-8"))["color"]
        kebab = {"".join("-" + ch.lower() if ch.isupper() else ch for ch in name) for name in look}
        mapping = self.tokens["products"]["colors"]
        self.assertEqual(set(mapping), kebab)
        for name, role in mapping.items():
            self.assertIn(role, self.tokens["color"]["roles"], name)

    def test_the_products_inks_read_on_their_grounds(self) -> None:
        mapping = self.tokens["products"]["colors"]
        grounds = ("void", "base", "well", "surface", "raised")
        # Every ink at 4.5:1 on every ground, but the faintest on the raised, which is held to 4.0, as Lumen's own
        # tertiary text is there.
        inks = ("ink", "ink-body", "ink-muted", "ink-faint", "accent-bright", "gilt", "gilt-bright", "ok", "warn", "bad")
        for hue in self.tokens["color"]["accentPresets"]:
            for high_contrast in (False, True):
                for ink in inks:
                    for ground in grounds:
                        floor = 4.0 if (ink, ground) == ("ink-faint", "raised") else 4.5
                        ratio = self._contrast(mapping[ink], mapping[ground], "dark", high_contrast, hue)
                        self.assertGreaterEqual(ratio, floor, f"{ink} on {ground} hc={high_contrast} hue={hue}")
                for fill in ("accent", "accent-dim"):
                    ratio = self._contrast(mapping["on-accent"], mapping[fill], "dark", high_contrast, hue)
                    self.assertGreaterEqual(ratio, 4.5, f"on-accent on {fill} hc={high_contrast} hue={hue}")

    def test_generated_files_are_current(self) -> None:
        self.assertEqual(gen.TOKENS_QML.read_text(encoding="utf-8"), gen.emit_qml(self.tokens),
                         "kit/Tokens.qml is stale: run tools/tokens/generate_tokens.py")
        self.assertEqual(gen.TOKENS_CPP.read_text(encoding="utf-8"), gen.emit_cpp(self.tokens),
                         "platform/common/lumentokens.h is stale: run tools/tokens/generate_tokens.py")
        self.assertEqual(gen.TOKENS_MD.read_text(encoding="utf-8"), gen.emit_markdown(self.tokens),
                         "docs/design/tokens.md is stale: run tools/tokens/generate_tokens.py")


if __name__ == "__main__":
    unittest.main()
