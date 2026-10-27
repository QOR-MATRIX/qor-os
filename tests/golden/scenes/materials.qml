// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// Golden scene: the Aurora wallpaper, the shared Backdrop blur and Glass.
// With Increase Contrast the glass falls back to solid material.
Item {
    property bool rtl: false

    width: 560
    height: 380
    LayoutMirroring.enabled: rtl
    LayoutMirroring.childrenInherit: true

    Wallpaper {
        id: wallpaper
        anchors.fill: parent
        living: false
    }

    Backdrop {
        id: backdrop
        sourceItem: wallpaper
    }

    Glass {
        id: panel
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 28
        width: 300
        height: 190
        backdrop: backdrop
        radius: 22
        elevation: 4

        Column {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 6
            Text { text: "Glass Horizon"; font: Theme.title3; color: Theme.textPrimary }
            Text { text: "Nova Arc"; font: Theme.body; color: Theme.textSecondary }
            Text {
                width: parent.width
                text: "Blurred once per frame, sampled by every panel."
                font: Theme.footnote
                color: Theme.textTertiary
                wrapMode: Text.Wrap
            }
        }
    }

    Glass {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 28
        width: 172
        height: 190
        backdrop: backdrop
        radius: 18
        elevation: 3

        Artwork {
            anchors.centerIn: parent
            width: 112
            height: 112
            hue: 300
            layer.enabled: true
            layer.effect: RoundMask { radius: Tokens.radius.l }
        }
    }

    Surface {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 28
        height: 96
        color: Theme.surface
        radius: Tokens.radius.xl
        elevation: 2
        highlight: 0.6

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 20
            spacing: 14
            AppIcon { glyph: "music"; hue: 0; size: 52 }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Text { text: "Opaque surface"; font: Theme.headline; color: Theme.textPrimary }
                Text { text: "Continuous-curvature corners"; font: Theme.footnote; color: Theme.accentText }
            }
        }
    }
}
