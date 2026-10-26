// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QTOS.Kit
import QTOS.Store

// The widget gallery, shown while editing the desktop's widgets: live
// previews of every installed widget (click to add), and the way to the
// Store for more.
Item {
    id: gallery

    property Backdrop backdrop: null
    property bool open: false

    signal addRequested(string id)
    signal storeRequested()
    signal doneRequested()

    implicitHeight: 236

    Glass {
        id: panel
        anchors.fill: parent
        backdrop: gallery.backdrop
        radius: Tokens.radius.xxl
        elevation: 5
    }

    Text {
        id: title
        x: 24
        y: 18
        text: qsTr("Widgets")
        color: Theme.textPrimary
        font: Theme.title3
    }
    Text {
        anchors.left: title.right
        anchors.leftMargin: 12
        anchors.baseline: title.baseline
        text: Store.widgets.length > 0 ? qsTr("Click one to add it. Drag widgets on the desktop to arrange them.")
                                       : qsTr("Get widgets from the Store to put them on your desktop.")
        color: Theme.textTertiary
        font: Theme.footnote
    }
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: title.verticalCenter
        spacing: 8
        Button {
            objectName: "widgetsGetMore"
            text: qsTr("Get More Widgets")
            flat: true
            onClicked: gallery.storeRequested()
        }
        Button {
            objectName: "widgetsDone"
            text: qsTr("Done")
            highlighted: true
            onClicked: gallery.doneRequested()
        }
    }

    ListView {
        id: list
        x: 20
        y: title.y + title.height + 16
        width: parent.width - 40
        height: parent.height - y - 18
        orientation: ListView.Horizontal
        spacing: 14
        clip: true
        model: Store.widgets

        delegate: Item {
            id: card

            required property var modelData
            readonly property size extent: modelData.size === "small" ? Qt.size(152, 152)
                                           : modelData.size === "medium" ? Qt.size(320, 152) : Qt.size(320, 320)
            readonly property real fit: Math.min((list.height - 26) / extent.height, 1)

            objectName: "galleryWidget:" + modelData.id
            width: extent.width * fit
            height: list.height

            Item {
                width: card.extent.width
                height: card.extent.height
                scale: card.fit * (hover.hovered ? 1.03 : 1)
                transformOrigin: Item.TopLeft
                Behavior on scale {
                    enabled: !Theme.reduceMotion
                    NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                }
                Rectangle {
                    anchors.fill: parent
                    radius: Tokens.radius.xl
                    color: Theme.surfaceSunken
                    border.color: hover.hovered ? Theme.accent : Theme.windowStroke
                    border.width: hover.hovered ? 2 : 1
                }
                Loader {
                    anchors.fill: parent
                    active: gallery.open
                    enabled: false
                    source: card.modelData.source
                }
            }
            Text {
                anchors.bottom: parent.bottom
                width: parent.width
                text: card.modelData.name
                color: Theme.textSecondary
                font: Theme.footnote
                elide: Text.ElideRight
            }
            HoverHandler { id: hover }
            TapHandler { onTapped: gallery.addRequested(card.modelData.id) }
        }
    }
}
