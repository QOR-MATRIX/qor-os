// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit

// A theme in miniature: its Aurora wallpaper with a small window in its
// tinted neutrals and accent. `theme` is a theme definition or a catalogue
// preview (accentHue, appearance, neutralHue, neutralTint, wallpaperSeed,
// wallpaperIntensity). Themes that suit either appearance follow the current
// one.
Item {
    id: preview

    property var theme: ({})
    property real radius: Tokens.radius.l

    readonly property real hue: theme.accentHue ?? Tokens.accentDefaultHue
    readonly property real darkness: theme.appearance === "dark" ? 1 : theme.appearance === "light" ? 0 : Theme.darkness
    readonly property real neutralHue: theme.neutralHue ?? 262
    readonly property real neutralTint: theme.neutralTint ?? 1

    function neutral(token: var): color {
        return Oklch.themed(token, darkness, hue, 0, neutralHue, neutralTint);
    }

    implicitWidth: 240
    implicitHeight: 150

    Item {
        anchors.fill: parent
        layer.enabled: true
        layer.effect: RoundMask { radius: preview.radius }

        Wallpaper {
            anchors.fill: parent
            hue: preview.hue
            darkness: preview.darkness
            seed: preview.theme.wallpaperSeed ?? 42
            intensity: preview.theme.wallpaperIntensity ?? 1
            living: false
        }

        // A window in the theme's colours.
        Surface {
            x: parent.width * 0.12
            y: parent.height * 0.2
            width: parent.width * 0.62
            height: parent.height * 0.62
            radius: Math.max(4, preview.radius * 0.55)
            color: preview.neutral(Tokens.color.surface)
            strokeColor: preview.neutral(Tokens.color.windowStroke)
            elevation: 2

            Rectangle {
                x: 0
                y: 0
                width: parent.width * 0.34
                height: parent.height
                radius: parent.radius
                color: Oklch.alpha(preview.neutral(Tokens.color.frostTint), 0.9)
            }
            Column {
                x: parent.width * 0.42
                y: parent.height * 0.18
                spacing: parent.height * 0.08
                Rectangle {
                    width: preview.width * 0.26
                    height: Math.max(3, preview.height * 0.04)
                    radius: height / 2
                    color: preview.neutral(Tokens.color.textPrimary)
                }
                Rectangle {
                    width: preview.width * 0.18
                    height: Math.max(3, preview.height * 0.03)
                    radius: height / 2
                    color: preview.neutral(Tokens.color.textSecondary)
                }
                Rectangle {
                    width: preview.width * 0.12
                    height: Math.max(8, preview.height * 0.09)
                    radius: height / 2
                    color: Oklch.themed(Tokens.color.accent, preview.darkness, preview.hue)
                }
            }
        }
    }

    Surface {
        anchors.fill: parent
        radius: preview.radius
        color: "transparent"
        strokeColor: Theme.windowStroke
    }
}
