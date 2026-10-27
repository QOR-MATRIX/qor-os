// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Services
import QOR.Store

// Desktop widgets: Store widgets on a grid over the wallpaper, below the
// windows, the same on every Space. Each sits on a glass card that blurs the
// wallpaper. The layout lives in qor-settingsd (Settings.widgets) as
// [{ "id", "key", "col", "row" }]; while `editing`, widgets can be dragged to
// a free cell or removed.
//
// Widgets are QML from signed first-party packages. Third-party widgets will
// run out of process, in the Phase 3 sandbox (docs/STORE.md).
Item {
    id: widgetGrid

    property rect area
    property Backdrop backdrop: null
    property bool editing: false

    readonly property real cell: 152
    readonly property real gap: 16
    readonly property real margin: 22
    readonly property int columns: Math.max(1, Math.floor((area.width - 2 * margin + gap) / (cell + gap)))
    readonly property int rows: Math.max(1, Math.floor((area.height - 2 * margin + gap) / (cell + gap)))
    // The grid starts at the work area's top left, clear of notifications.
    readonly property real originX: area.x + margin
    readonly property real originY: area.y + margin

    // The widgets on the desktop, from Settings.
    readonly property var placed: {
        if (!Services.connected)
            return [];
        try {
            const list = JSON.parse(Services.settings.widgets || "[]");
            return Array.isArray(list) ? list : [];
        } catch (e) {
            return [];
        }
    }

    signal menuRequested(point scenePosition)

    function span(size: string): size {
        return size === "small" ? Qt.size(1, 1) : size === "medium" ? Qt.size(2, 1) : Qt.size(2, 2);
    }

    function sizeOf(id: string): size {
        const widget = Store.widget(id);
        return span(widget.size ?? "small");
    }

    // Whether a widget of `extent` fits at (col, row) without overlapping any
    // other (but the one with `ignoreKey`).
    function fits(col: int, row: int, extent: size, ignoreKey: string): bool {
        if (col < 0 || row < 0 || col + extent.width > columns || row + extent.height > rows)
            return false;
        for (const other of placed) {
            if (other.key === ignoreKey)
                continue;
            const o = sizeOf(other.id);
            if (col < other.col + o.width && other.col < col + extent.width
                    && row < other.row + o.height && other.row < row + extent.height)
                return false;
        }
        return true;
    }

    // Adds an installed widget at the first free cell, down the columns from
    // the left.
    function add(id: string): bool {
        const widget = Store.widget(id);
        if (!widget.id)
            return false;
        const extent = span(widget.size);
        for (let col = 0; col + extent.width <= columns; ++col) {
            for (let row = 0; row + extent.height <= rows; ++row) {
                if (fits(col, row, extent, "")) {
                    const key = Date.now().toString(36) + Math.floor(Math.random() * 1e6).toString(36);
                    Services.settings.widgets = JSON.stringify(placed.concat([{ id: id, key: key, col: col, row: row }]));
                    return true;
                }
            }
        }
        return false;
    }

    function remove(key: string) {
        Services.settings.widgets = JSON.stringify(placed.filter(w => w.key !== key));
    }

    function moveTo(key: string, col: int, row: int): bool {
        const entry = placed.find(w => w.key === key);
        if (!entry || !fits(col, row, sizeOf(entry.id), key))
            return false;
        Services.settings.widgets = JSON.stringify(placed.map(w => w.key === key ? Object.assign({}, w, { col: col, row: row }) : w));
        return true;
    }

    // The live widgets, kept in step with `placed` by key, so moving or adding
    // one never restarts the others.
    ListModel { id: live }
    onPlacedChanged: sync()
    Component.onCompleted: sync()
    function sync() {
        for (let i = live.count - 1; i >= 0; --i) {
            const key = live.get(i).key;
            if (!placed.some(w => w.key === key))
                live.remove(i);
        }
        for (const w of placed) {
            let index = -1;
            for (let i = 0; i < live.count && index < 0; ++i)
                if (live.get(i).key === w.key)
                    index = i;
            if (index < 0) {
                live.append({ key: w.key, widgetId: w.id, col: w.col, row: w.row });
            } else {
                live.setProperty(index, "col", w.col);
                live.setProperty(index, "row", w.row);
            }
        }
    }

    // A right click (or press and hold) on the desktop.
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => widgetGrid.menuRequested(eventPoint.scenePosition)
    }
    TapHandler {
        acceptedDevices: PointerDevice.TouchScreen
        onLongPressed: widgetGrid.menuRequested(point.scenePosition)
    }

    Repeater {
        model: live

        delegate: Item {
            id: frame

            required property string key
            required property string widgetId
            required property int col
            required property int row
            // Re-read when widgets are installed or removed.
            readonly property var widget: Store.widgets.length >= 0 ? Store.widget(widgetId) : ({})
            readonly property size extent: widgetGrid.span(widget.size ?? "small")
            readonly property real homeX: widgetGrid.originX + col * (widgetGrid.cell + widgetGrid.gap)
            readonly property real homeY: widgetGrid.originY + row * (widgetGrid.cell + widgetGrid.gap)

            objectName: "widget:" + widgetId
            visible: widget.id !== undefined
            x: homeX
            y: homeY
            width: extent.width * widgetGrid.cell + (extent.width - 1) * widgetGrid.gap
            height: extent.height * widgetGrid.cell + (extent.height - 1) * widgetGrid.gap
            z: drag.active ? 2 : 1
            scale: drag.active ? 1.03 : widgetGrid.editing ? 0.97 : 1
            Behavior on scale {
                enabled: !Theme.reduceMotion
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }
            Behavior on x {
                enabled: !drag.active && !Theme.reduceMotion
                SpringAnimation { spring: 4; damping: 0.4 }
            }
            Behavior on y {
                enabled: !drag.active && !Theme.reduceMotion
                SpringAnimation { spring: 4; damping: 0.4 }
            }

            Glass {
                anchors.fill: parent
                backdrop: widgetGrid.backdrop
                radius: Tokens.radius.xl
                elevation: drag.active ? 4 : 2
                strokeColor: widgetGrid.editing ? Oklch.alpha(Theme.accent, 0.6) : Theme.glassStroke
            }

            Loader {
                id: content
                anchors.fill: parent
                source: frame.widget.source ?? ""
                enabled: !widgetGrid.editing
            }
            Text {
                anchors.centerIn: parent
                visible: content.status === Loader.Error
                text: qsTr("This widget could not start.")
                color: Theme.textTertiary
                font: Theme.footnote
            }

            // Editing: drag to another cell; the cross removes it.
            DragHandler {
                id: drag
                enabled: widgetGrid.editing
                onActiveChanged: {
                    if (active)
                        return;
                    const col = Math.round((frame.x - widgetGrid.originX) / (widgetGrid.cell + widgetGrid.gap));
                    const row = Math.round((frame.y - widgetGrid.originY) / (widgetGrid.cell + widgetGrid.gap));
                    widgetGrid.moveTo(frame.key, col, row);
                    // Back to its (possibly new) cell.
                    frame.x = Qt.binding(() => frame.homeX);
                    frame.y = Qt.binding(() => frame.homeY);
                }
            }
            IconButton {
                objectName: "removeWidget:" + frame.widgetId
                visible: widgetGrid.editing
                x: -10
                y: -10
                width: 26
                height: 26
                radius: 13
                icon: "x"
                iconSize: 13
                color: Theme.textPrimary
                Rectangle {
                    z: -1
                    anchors.fill: parent
                    radius: width / 2
                    color: Theme.surfaceRaised
                    border.color: Theme.windowStroke
                }
                onClicked: widgetGrid.remove(frame.key)
            }
        }
    }
}
