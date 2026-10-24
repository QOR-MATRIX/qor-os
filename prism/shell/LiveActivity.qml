// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit
import QTOS.Services

// The Live Activity capsule in the centre of the Rail. It shows what is
// happening right now (media playback today; timers, calls, recordings and
// downloads publish here through qtos_activity on device) and opens the
// matching popover.
Item {
    id: capsule

    property bool expanded: false
    signal toggled()

    readonly property bool active: Services.media.available
    implicitHeight: 26
    width: widthSpring.value
    height: implicitHeight
    visible: width > 2
    opacity: active ? 1 : 0

    Spring {
        id: widthSpring
        target: capsule.active ? row.implicitWidth + 22 : 0
        preset: Motion.snappy
    }
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Surface {
        anchors.fill: parent
        radius: height / 2
        exponent: 2
        color: capsule.expanded ? Theme.fillControlPressed : hover.hovered ? Theme.fillControlHover : Theme.fillControl
        strokeColor: Oklch.alpha(Theme.glassStroke, Theme.glassStroke.a * 0.6)
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Item {
        anchors.fill: parent
        clip: true

        Row {
            id: row
            x: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Artwork {
                anchors.verticalCenter: parent.verticalCenter
                width: 18
                height: 18
                radius: 9
                exponent: 2
                hue: Services.media.artworkHue
                glyphVisible: false
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: Services.media.title
                font: Theme.footnote
                color: Theme.textPrimary
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: Services.media.artist
                font: Theme.footnote
                color: Theme.textTertiary
            }
            Equalizer {
                anchors.verticalCenter: parent.verticalCenter
                playing: Services.media.playing
                barColor: Oklch.color(0.78, 0.15, Services.media.artworkHue)
            }
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: capsule.toggled() }

    component Equalizer: Row {
        id: eq
        property bool playing: false
        property color barColor: Theme.accent
        spacing: 2
        height: 12
        Repeater {
            model: 4
            delegate: Rectangle {
                id: bar
                required property int index
                anchors.bottom: parent.bottom
                width: 2.5
                radius: 1.25
                color: eq.barColor
                height: eq.playing ? level : 3
                property real level: 6
                Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                Timer {
                    running: eq.playing && eq.visible
                    repeat: true
                    interval: 170 + bar.index * 37
                    onTriggered: bar.level = 3 + Math.random() * 9
                }
            }
        }
    }
}
