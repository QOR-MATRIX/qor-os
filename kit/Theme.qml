// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma Singleton

import QtQuick
import QTOS.Kit

// Live Lumen theme. Every color is resolved from an OKLCH token at runtime, so
// light/dark and the accent hue animate continuously: `darkness` and the
// accent hue are springs, and all roles re-resolve as they move.
//
// The shell binds the appearance state to qtos-settingsd; apps only read it.
QtObject {
    id: theme

    // ── Appearance state ──────────────────────────────────────────────────
    property bool dark: true
    property real accentHue: Tokens.accentDefaultHue
    property bool reduceMotion: false
    property bool reduceTransparency: false
    property bool increaseContrast: false
    property bool livingWallpaper: false

    // Glass and frost render as solid surfaces. Increase Contrast implies it:
    // a blurred, tinted backdrop cannot guarantee text contrast.
    readonly property bool solidMaterials: reduceTransparency || increaseContrast

    // ── Animated drivers ──────────────────────────────────────────────────
    readonly property real darkness: _appearance.value
    readonly property real hue: _hue.value
    readonly property real contrast: _contrast.value

    readonly property Spring _appearance: Spring {
        target: theme.dark ? 1 : 0
        preset: Tokens.motion.gentle
    }
    readonly property Spring _hue: Spring {
        preset: Tokens.motion.smooth
    }
    readonly property Spring _contrast: Spring {
        target: theme.increaseContrast ? 1 : 0
        preset: Tokens.motion.smooth
    }

    onAccentHueChanged: {
        // Travel the short way around the hue circle.
        const delta = ((((accentHue - _hue.value) % 360) + 540) % 360) - 180;
        _hue.target = _hue.value + delta;
    }
    onReduceMotionChanged: Kit.reducedMotion = reduceMotion
    Component.onCompleted: {
        Kit.reducedMotion = reduceMotion;
        _hue.snap(accentHue);
    }

    function resolve(token: var): color {
        return Oklch.themed(token, darkness, hue, contrast);
    }

    function elevation(level: int): var {
        return Tokens.elevation[Math.max(0, Math.min(Tokens.elevation.length - 1, level))];
    }

    // ── Color roles ───────────────────────────────────────────────────────
    readonly property color background: resolve(Tokens.color.background)
    readonly property color surface: resolve(Tokens.color.surface)
    readonly property color surfaceRaised: resolve(Tokens.color.surfaceRaised)
    readonly property color surfaceSunken: resolve(Tokens.color.surfaceSunken)
    readonly property color glassTint: solidMaterials ? Oklch.alpha(resolve(Tokens.color.glassTint), 0.97)
                                                      : resolve(Tokens.color.glassTint)
    readonly property color glassStroke: resolve(Tokens.color.glassStroke)
    readonly property color frostTint: solidMaterials ? Oklch.alpha(resolve(Tokens.color.frostTint), 0.98)
                                                      : resolve(Tokens.color.frostTint)
    readonly property color windowStroke: resolve(Tokens.color.windowStroke)
    readonly property color textPrimary: resolve(Tokens.color.textPrimary)
    readonly property color textSecondary: resolve(Tokens.color.textSecondary)
    readonly property color textTertiary: resolve(Tokens.color.textTertiary)
    readonly property color textDisabled: resolve(Tokens.color.textDisabled)
    readonly property color separator: resolve(Tokens.color.separator)
    readonly property color fillControl: resolve(Tokens.color.fillControl)
    readonly property color fillControlHover: resolve(Tokens.color.fillControlHover)
    readonly property color fillControlPressed: resolve(Tokens.color.fillControlPressed)
    readonly property color fillSelected: resolve(Tokens.color.fillSelected)
    readonly property color accent: resolve(Tokens.color.accent)
    readonly property color accentHover: resolve(Tokens.color.accentHover)
    readonly property color accentText: resolve(Tokens.color.accentText)
    readonly property color onAccent: resolve(Tokens.color.onAccent)
    readonly property color focusRing: resolve(Tokens.color.focusRing)
    readonly property color shadow: resolve(Tokens.color.shadow)
    readonly property color scrim: resolve(Tokens.color.scrim)
    readonly property color positive: resolve(Tokens.color.positive)
    readonly property color warning: resolve(Tokens.color.warning)
    readonly property color critical: resolve(Tokens.color.critical)

    // ── Typography ────────────────────────────────────────────────────────
    readonly property string fontFamily: Kit.uiFontFamily
    readonly property string monoFamily: Kit.monoFontFamily

    function typeface(style: var, family: string): font {
        return Qt.font({
            family: family.length > 0 ? family : fontFamily,
            pixelSize: style.size,
            weight: style.weight,
            letterSpacing: style.tracking,
            variableAxes: { "wght": style.weight, "opsz": style.opsz }
        });
    }

    readonly property font caption: typeface(Tokens.typography.caption, "")
    readonly property font footnote: typeface(Tokens.typography.footnote, "")
    readonly property font body: typeface(Tokens.typography.body, "")
    readonly property font bodyStrong: typeface(Tokens.typography.bodyStrong, "")
    readonly property font callout: typeface(Tokens.typography.callout, "")
    readonly property font headline: typeface(Tokens.typography.headline, "")
    readonly property font title3: typeface(Tokens.typography.title3, "")
    readonly property font title2: typeface(Tokens.typography.title2, "")
    readonly property font title1: typeface(Tokens.typography.title1, "")
    readonly property font display: typeface(Tokens.typography.display, "")
    readonly property font hero: typeface(Tokens.typography.hero, "")
    readonly property font mono: typeface(Tokens.typography.body, monoFamily)
}
