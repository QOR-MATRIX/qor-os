// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma Singleton

import QtQuick
import QOR.Kit

// Live Lumen theme. Every color is resolved from an OKLCH token at runtime, so
// light/dark and the accent hue animate continuously: `darkness` and the
// accent hue are springs, and all roles re-resolve as they move.
//
// The shell binds the appearance state to qor-settingsd; apps only read it.
QtObject {
    id: theme

    // ── Appearance state ──────────────────────────────────────────────────
    property bool dark: true
    property real accentHue: Tokens.accentDefaultHue
    property bool reduceMotion: false
    property bool reduceTransparency: false
    property bool increaseContrast: false
    property bool livingWallpaper: false

    // The theme's tint for the neutral roles (surfaces, text, strokes): their
    // hue, and a multiplier for their chroma. Lumen's own neutrals are a cool
    // blue-grey at hue 262. Store themes set these (QOR.Store Themes).
    property real neutralHue: 262
    property real neutralTint: 1
    // The theme's Aurora wallpaper: where its pattern starts, and how bright.
    property real wallpaperSeed: 42
    property real wallpaperIntensity: 1

    // Glass and frost render as solid surfaces. Increase Contrast implies it:
    // a blurred, tinted backdrop cannot guarantee text contrast.
    readonly property bool solidMaterials: reduceTransparency || increaseContrast

    // ── Animated drivers ──────────────────────────────────────────────────
    readonly property real darkness: _appearance.value
    readonly property real hue: _hue.value
    readonly property real contrast: _contrast.value
    readonly property real neutralHueValue: _neutralHue.value
    readonly property real neutralTintValue: _neutralTint.value

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
    readonly property Spring _neutralHue: Spring {
        preset: Tokens.motion.smooth
    }
    readonly property Spring _neutralTint: Spring {
        target: theme.neutralTint
        preset: Tokens.motion.smooth
    }

    onAccentHueChanged: {
        // Travel the short way around the hue circle.
        const delta = ((((accentHue - _hue.value) % 360) + 540) % 360) - 180;
        _hue.target = _hue.value + delta;
    }
    onNeutralHueChanged: {
        const delta = ((((neutralHue - _neutralHue.value) % 360) + 540) % 360) - 180;
        _neutralHue.target = _neutralHue.value + delta;
    }
    onReduceMotionChanged: Kit.reducedMotion = reduceMotion
    Component.onCompleted: {
        Kit.reducedMotion = reduceMotion;
        _hue.snap(accentHue);
        _neutralHue.snap(neutralHue);
        _neutralTint.snap(neutralTint);
    }

    function resolve(token: var): color {
        return Oklch.themed(token, darkness, hue, contrast);
    }

    // Neutral roles take the theme's tint.
    function neutral(token: var): color {
        return Oklch.themed(token, darkness, hue, contrast, ((neutralHueValue % 360) + 360) % 360, neutralTintValue);
    }

    function elevation(level: int): var {
        return Tokens.elevation[Math.max(0, Math.min(Tokens.elevation.length - 1, level))];
    }

    // ── Color roles ───────────────────────────────────────────────────────
    readonly property color background: neutral(Tokens.color.background)
    readonly property color surface: neutral(Tokens.color.surface)
    readonly property color surfaceRaised: neutral(Tokens.color.surfaceRaised)
    readonly property color surfaceSunken: neutral(Tokens.color.surfaceSunken)
    readonly property color glassTint: solidMaterials ? Oklch.alpha(neutral(Tokens.color.glassTint), 0.97)
                                                      : neutral(Tokens.color.glassTint)
    readonly property color glassStroke: neutral(Tokens.color.glassStroke)
    readonly property color frostTint: solidMaterials ? Oklch.alpha(neutral(Tokens.color.frostTint), 0.98)
                                                      : neutral(Tokens.color.frostTint)
    readonly property color windowStroke: neutral(Tokens.color.windowStroke)
    readonly property color textPrimary: neutral(Tokens.color.textPrimary)
    readonly property color textSecondary: neutral(Tokens.color.textSecondary)
    readonly property color textTertiary: neutral(Tokens.color.textTertiary)
    readonly property color textDisabled: neutral(Tokens.color.textDisabled)
    readonly property color separator: neutral(Tokens.color.separator)
    readonly property color fillControl: neutral(Tokens.color.fillControl)
    readonly property color fillControlHover: neutral(Tokens.color.fillControlHover)
    readonly property color fillControlPressed: neutral(Tokens.color.fillControlPressed)
    readonly property color fillSelected: resolve(Tokens.color.fillSelected)
    readonly property color accent: resolve(Tokens.color.accent)
    readonly property color accentHover: resolve(Tokens.color.accentHover)
    readonly property color accentText: resolve(Tokens.color.accentText)
    readonly property color onAccent: resolve(Tokens.color.onAccent)
    readonly property color focusRing: resolve(Tokens.color.focusRing)
    readonly property color shadow: neutral(Tokens.color.shadow)
    readonly property color scrim: neutral(Tokens.color.scrim)
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
