// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Services

// The Live Activity capsule in the centre of the Rail. It shows what is
// happening right now (media playback today; timers, calls, recordings and
// downloads publish here through qor_activity on device) and opens the
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
        // The rim catches the music's energy.
        strokeColor: Reactive.energy + Reactive.pulse > 0.01
                     ? Oklch.color(0.78, 0.15, Services.media.artworkHue, 0.25 + 0.5 * Math.min(1, Reactive.energy + Reactive.pulse))
                     : Oklch.alpha(Theme.glassStroke, Theme.glassStroke.a * 0.6)
        strokeWidth: 1 + Reactive.pulse
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
            // Four bars of the live spectrum.
            Spectrum {
                objectName: "liveActivitySpectrum"
                anchors.verticalCenter: parent.verticalCenter
                width: 17
                height: 12
                bars: 4
                spacing: 2
                minimum: 0.25
                values: Services.media.playing ? Reactive.spectrum.slice(2, 34) : []
                color: Oklch.color(0.78, 0.15, Services.media.artworkHue)
            }
        }
    }

    HoverHandler { id: hover }
    TapHandler { onTapped: capsule.toggled() }
}
