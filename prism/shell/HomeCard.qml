// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit

// One game or piece of media on Home and in the Library: art drawn from its
// hue until it has its own, with its glyph; its title and what it is; and,
// for a film or a song, how far it got. The one the keyboard or the d-pad is
// on (`current`) is lifted and ringed.
Item {
    id: card

    required property var item
    property bool current: false

    signal chosen()

    implicitWidth: 232
    implicitHeight: 194
    z: current ? 1 : 0
    scale: current ? 1.05 : 1
    Behavior on scale {
        NumberAnimation { duration: Theme.reduceMotion ? 0 : 160; easing.type: Easing.OutCubic }
    }

    Accessible.role: Accessible.Button
    Accessible.name: card.item.title
    Accessible.description: card.item.subtitle

    Artwork {
        id: art
        width: parent.width
        height: 136
        hue: card.item.hue
        glyphVisible: false

        Icon {
            anchors.centerIn: parent
            name: card.item.glyph
            size: 40
            color: Qt.rgba(1, 1, 1, 0.82)
        }
        // How far a film or a song got.
        Rectangle {
            readonly property real done: card.item.duration > 0 ? Math.min(1, card.item.position / card.item.duration) : 0
            visible: done > 0
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 10
            width: (parent.width - 20) * done
            height: 4
            radius: 2
            color: Theme.accent
        }
    }
    Rectangle {
        objectName: "homeCardRing"
        anchors.fill: art
        anchors.margins: -4
        radius: art.radius + 4
        color: "transparent"
        border.width: 3
        border.color: Theme.focusRing
        visible: card.current
    }

    Text {
        id: title
        anchors.top: art.bottom
        anchors.topMargin: 10
        width: parent.width
        text: card.item.title
        color: Theme.textPrimary
        font: Theme.bodyStrong
        elide: Text.ElideRight
    }
    Text {
        anchors.top: title.bottom
        anchors.topMargin: 2
        width: parent.width
        text: card.item.subtitle
        color: Theme.textSecondary
        font: Theme.footnote
        elide: Text.ElideRight
    }

    TapHandler {
        onTapped: card.chosen()
    }
}
