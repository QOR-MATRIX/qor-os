// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// One Shelf slot: an app (or system) icon that magnifies with pointer
// proximity, shows a label while hovered and hops when activated.
Item {
    id: button

    property string appId
    property string glyph
    // An app's own icon (Wayland clients); when set it replaces the glyph tile.
    property url iconSource
    property string label
    property real hue: 274
    property real chroma: 0.15
    property bool neutral: false
    property bool running: false
    // The app wants attention: the icon hops every 1.5 s and its dot turns to the accent.
    property bool attention: false
    property bool checked: false
    property real pointerX: -1000   // pointer x in the Shelf row's coordinates

    signal activated()

    width: 46
    height: 46
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    readonly property real centerInRow: x + width / 2
    readonly property real proximity: Math.max(0, 1 - Math.abs(centerInRow - pointerX) / 120)

    Spring {
        id: magnify
        target: 1 + 0.24 * button.proximity * button.proximity
        preset: Motion.snappy
    }
    Spring {
        id: hop
        target: 0
        preset: Motion.bouncy
    }

    Item {
        id: visual
        width: parent.width
        height: parent.height
        y: hop.value
        scale: magnify.value * (tap.pressed ? 0.92 : 1)
        transformOrigin: Item.Bottom

        Image {
            anchors.fill: parent
            anchors.margins: 2
            visible: !button.neutral && button.iconSource.toString().length > 0
            source: button.iconSource
            sourceSize: Qt.size(width * Screen.devicePixelRatio * 1.3, height * Screen.devicePixelRatio * 1.3)
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
            asynchronous: true
        }

        AppIcon {
            anchors.fill: parent
            visible: !button.neutral && button.iconSource.toString().length === 0
            size: button.width
            glyph: button.glyph
            hue: button.hue
            chroma: button.chroma
        }

        Surface {
            anchors.fill: parent
            visible: button.neutral
            radius: button.width * 0.27
            exponent: 4.6
            color: button.checked ? Theme.fillSelected : Theme.fillControlHover
            strokeColor: Theme.glassStroke
            highlight: 0.6
            Icon {
                anchors.centerIn: parent
                name: button.glyph
                size: 21
                color: button.checked ? Theme.accentText : Theme.textPrimary
            }
        }
    }

    Timer {
        running: button.attention && !Motion.reduced
        interval: 1500
        repeat: true
        triggeredOnStart: true
        onTriggered: hop.velocity = -460
    }

    // Running indicator.
    Surface {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height + 4
        width: 4
        height: 4
        radius: 2
        exponent: 2
        color: button.attention ? Theme.accent : Theme.textPrimary
        opacity: button.attention ? 1 : button.running ? 0.75 : 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    // Hover label.
    Surface {
        id: tip
        anchors.horizontalCenter: parent.horizontalCenter
        y: -height - 18 - (magnify.value - 1) * button.height
        width: tipText.implicitWidth + 18
        height: 24
        radius: 8
        color: Theme.surfaceRaised
        strokeColor: Theme.windowStroke
        elevation: 2
        opacity: hover.hovered ? 1 : 0
        visible: opacity > 0.01
        Behavior on opacity { NumberAnimation { duration: 130 } }
        Text {
            id: tipText
            anchors.centerIn: parent
            text: button.label
            font: Theme.footnote
            color: Theme.textPrimary
        }
    }

    HoverHandler { id: hover }
    TapHandler {
        id: tap
        onTapped: {
            if (!button.neutral && !button.running)
                hop.velocity = -460;
            button.activated();
        }
    }

    Accessible.role: Accessible.Button
    Accessible.name: label
}
