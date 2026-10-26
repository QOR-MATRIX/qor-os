// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit
import QTOS.Services

// Now Playing: the artwork, the track and the transport, with the position
// beneath. Shows a play button to start the queue when nothing plays.
Item {
    id: widget

    readonly property var media: Services.media
    readonly property bool hasTrack: Services.connected && media.title.length > 0
    readonly property real fraction: hasTrack && media.duration > 0 ? media.position / media.duration : 0

    Artwork {
        id: art
        x: 16
        anchors.verticalCenter: parent.verticalCenter
        width: parent.height - 32
        height: width
        hue: widget.hasTrack ? widget.media.artworkHue : Theme.hue
    }

    Column {
        anchors.left: art.right
        anchors.leftMargin: 14
        anchors.right: parent.right
        anchors.rightMargin: 16
        y: art.y + 2
        spacing: 1

        Text {
            width: parent.width
            text: widget.hasTrack ? widget.media.title : qsTr("Nothing playing")
            color: Theme.textPrimary
            font: Theme.headline
            elide: Text.ElideRight
        }
        Text {
            width: parent.width
            text: widget.hasTrack ? widget.media.artist : qsTr("Play to start your queue")
            color: Theme.textSecondary
            font: Theme.footnote
            elide: Text.ElideRight
        }
    }

    Row {
        anchors.left: art.right
        anchors.leftMargin: 8
        anchors.bottom: progress.top
        anchors.bottomMargin: 8
        spacing: 2

        IconButton {
            width: 32; height: 32
            icon: "skip-back"
            enabled: widget.hasTrack
            onClicked: widget.media.previous()
        }
        IconButton {
            objectName: "nowPlayingToggle"
            width: 32; height: 32
            icon: Services.connected && widget.media.playing ? "pause" : "play"
            iconSize: 18
            color: Theme.textPrimary
            onClicked: widget.media.togglePlayback()
        }
        IconButton {
            width: 32; height: 32
            icon: "skip-forward"
            enabled: widget.hasTrack
            onClicked: widget.media.next()
        }
    }

    Rectangle {
        id: progress
        anchors.left: art.right
        anchors.leftMargin: 14
        anchors.right: parent.right
        anchors.rightMargin: 16
        y: art.y + art.height - height
        height: 3
        radius: 1.5
        color: Theme.fillControl
        Rectangle {
            width: parent.width * widget.fraction
            height: parent.height
            radius: parent.radius
            color: Theme.accent
        }
    }
}
