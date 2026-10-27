// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Apps

// The Shelf: a floating glass dock with Beam, pinned and running apps, and
// Overview. Icons magnify around the pointer and hop when launching.
Item {
    id: shelf

    property Backdrop backdrop
    property var runningApps: []
    property var attentionApps: []
    // appId => {name, icon} for apps outside the registry (Wayland clients).
    property var identityProvider: appId => ({ name: appId, icon: "" })
    property bool overview: false

    signal launchRequested(string appId)
    signal beamRequested()
    signal overviewRequested()

    readonly property var apps: {
        const ids = AppRegistry.pinned.slice();
        for (const id of runningApps)
            if (ids.indexOf(id) < 0)
                ids.push(id);
        return ids;
    }

    // Where a window for `appId` should fly from / to, in desktop coordinates.
    function originFor(appId: string): rect {
        for (let i = 0; i < iconRepeater.count; ++i) {
            const button = iconRepeater.itemAt(i) as ShelfButton;
            if (button && button.appId === appId) {
                const p = button.mapToItem(shelf.parent, 0, 0);
                return Qt.rect(p.x, p.y, button.width, button.height);
            }
        }
        const c = mapToItem(shelf.parent, width / 2, 0);
        return Qt.rect(c.x - 24, c.y, 48, 48);
    }

    width: row.implicitWidth + 24
    height: 70

    Glass {
        anchors.fill: parent
        backdrop: shelf.backdrop
        radius: 24
        elevation: 4
        highlight: 1
    }

    HoverHandler { id: pointer }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 10

        ShelfButton {
            glyph: "search"
            label: qsTr("Beam")
            neutral: true
            pointerX: pointer.hovered ? pointer.point.position.x - row.x : -1000
            onActivated: shelf.beamRequested()
        }

        Divider {
            vertical: true
            height: 34
            anchors.verticalCenter: parent.verticalCenter
        }

        Repeater {
            id: iconRepeater
            model: shelf.apps
            delegate: ShelfButton {
                required property string modelData
                readonly property var app: AppRegistry.app(modelData)
                readonly property var client: app ? null : shelf.identityProvider(modelData)
                appId: modelData
                glyph: app ? app.glyph : "app-window"
                iconSource: client ? client.icon : ""
                hue: app ? app.hue : 274
                chroma: app ? app.chroma : 0.1
                label: app ? app.name : client.name
                running: shelf.runningApps.indexOf(modelData) >= 0
                attention: shelf.attentionApps.indexOf(modelData) >= 0
                pointerX: pointer.hovered ? pointer.point.position.x - row.x : -1000
                onActivated: shelf.launchRequested(modelData)
            }
        }

        Divider {
            vertical: true
            height: 34
            anchors.verticalCenter: parent.verticalCenter
        }

        ShelfButton {
            glyph: "layout-grid"
            label: qsTr("Overview")
            neutral: true
            checked: shelf.overview
            pointerX: pointer.hovered ? pointer.point.position.x - row.x : -1000
            onActivated: shelf.overviewRequested()
        }
    }
}
