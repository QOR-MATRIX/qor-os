// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit
import QTOS.Apps

// A single notification banner. Slides in on a spring, waits (pausing while
// hovered), then slides out and asks to be removed.
Item {
    id: card

    property Backdrop backdrop
    property int notificationId
    property string app
    property string heading
    property string message
    property string glyph
    property real postedAt
    property int timeoutMs: 6500
    property bool leaving: false

    signal activated()
    signal expired()

    readonly property var identity: AppRegistry.identity(app)

    height: 78
    opacity: Math.min(1, presence.value * 1.4)

    Spring {
        id: presence
        value: 0
        target: card.leaving ? 0 : 1
        preset: Motion.smooth
        onSettled: if (card.leaving) card.expired()
    }

    Timer {
        interval: card.timeoutMs
        running: !hover.hovered && !card.leaving
        onTriggered: card.leaving = true
    }

    Item {
        width: parent.width
        height: parent.height
        x: (1 - presence.value) * (card.width + 24)

        Glass {
            anchors.fill: parent
            backdrop: card.backdrop
            radius: 18
            elevation: 4
        }

        AppIcon {
            id: appIcon
            x: 12
            y: 14
            size: 34
            elevation: 1
            glyph: card.glyph.length > 0 ? card.glyph : card.identity.glyph
            hue: card.identity.hue
            chroma: card.identity.chroma
        }

        Column {
            anchors.left: appIcon.right
            anchors.leftMargin: 11
            anchors.right: parent.right
            anchors.rightMargin: 14
            y: 11
            spacing: 1
            Row {
                width: parent.width
                Text {
                    width: parent.width - when.width
                    text: card.identity.name
                    font: Theme.caption
                    color: Theme.textTertiary
                    elide: Text.ElideRight
                }
                Text {
                    id: when
                    text: qsTr("now")
                    font: Theme.caption
                    color: Theme.textTertiary
                }
            }
            Text {
                width: parent.width
                text: card.heading
                font: Theme.bodyStrong
                color: Theme.textPrimary
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: card.message
                font: Theme.footnote
                color: Theme.textSecondary
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }

        IconButton {
            x: -6
            y: -6
            width: 22
            height: 22
            radius: 11
            icon: "x"
            iconSize: 11
            opacity: hover.hovered ? 1 : 0
            visible: opacity > 0.01
            Behavior on opacity { NumberAnimation { duration: 120 } }
            onClicked: card.leaving = true
        }

        HoverHandler { id: hover }
        TapHandler { onTapped: card.activated() }
    }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: heading + ". " + message
}
