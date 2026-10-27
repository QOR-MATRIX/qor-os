// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Services

// Expanded Now Playing, opened from the Live Activity capsule.
Item {
    id: popover

    property bool open: false
    property Backdrop backdrop
    signal dismissed()

    width: 380
    height: 156
    visible: presence.value > 0.002
    opacity: presence.value
    transformOrigin: Item.Top
    scale: 0.94 + 0.06 * presence.value

    Spring { id: presence; target: popover.open ? 1 : 0; preset: Motion.snappy }

    function formatTime(seconds: real): string {
        const s = Math.max(0, Math.floor(seconds));
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0");
    }

    Glass {
        anchors.fill: parent
        backdrop: popover.backdrop
        radius: 22
        elevation: 4

        Artwork {
            id: art
            x: 16
            y: 16
            width: 84
            height: 84
            hue: Services.media.artworkHue
        }

        Column {
            anchors.left: art.right
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 16
            y: 20
            spacing: 2
            Text {
                width: parent.width
                text: Services.media.title
                font: Theme.headline
                color: Theme.textPrimary
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: Services.media.artist + " — " + Services.media.album
                font: Theme.footnote
                color: Theme.textSecondary
                elide: Text.ElideRight
            }
            Text {
                text: qsTr("Playing in %1").arg(Services.media.player)
                font: Theme.caption
                color: Theme.textTertiary
            }
        }

        Row {
            anchors.left: art.right
            anchors.leftMargin: 8
            y: 74
            spacing: 4
            IconButton { icon: "skip-back"; iconSize: 17; width: 34; height: 30; onClicked: Services.media.previous() }
            IconButton {
                icon: Services.media.playing ? "pause" : "play"
                iconSize: 19
                width: 38
                height: 30
                color: Theme.textPrimary
                onClicked: Services.media.togglePlayback()
            }
            IconButton { icon: "skip-forward"; iconSize: 17; width: 34; height: 30; onClicked: Services.media.next() }
        }

        Row {
            x: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            width: parent.width - 32
            spacing: 10
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 34
                text: popover.formatTime(Services.media.position)
                font: Theme.caption
                color: Theme.textTertiary
            }
            Slider {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 88
                from: 0
                to: Math.max(1, Services.media.duration)
                value: Services.media.position
                onMoved: Services.media.seek(value)
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 34
                horizontalAlignment: Text.AlignRight
                text: popover.formatTime(Services.media.duration)
                font: Theme.caption
                color: Theme.textTertiary
            }
        }
    }
}
