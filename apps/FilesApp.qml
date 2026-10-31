// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QOR.Kit
import "FileIndex.js" as FileIndex

// Files: sidebar locations and tags, grid and list views, live filtering and
// an inspector for the selection.
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})

    property string location: "Recents"
    property string selectedName: ""
    property bool gridView: true
    readonly property var entries: {
        const q = search.text.trim().toLowerCase();
        const list = FileIndex.filesIn(location);
        return q.length === 0 ? list : list.filter(f => f.name.toLowerCase().indexOf(q) >= 0);
    }
    readonly property var selected: {
        for (const f of entries)
            if (f.name === selectedName)
                return f;
        return null;
    }

    onLaunchArgsChanged: {
        if (launchArgs && launchArgs.folder)
            location = launchArgs.folder;
        if (launchArgs && launchArgs.select)
            selectedName = launchArgs.select;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    FrostSidebar {
        id: sidebar
        width: 214
        height: parent.height
        topInset: app.topInset

        SectionHeader { text: qsTr("Locations") }
        Repeater {
            model: FileIndex.folders()
            delegate: SidebarItem {
                required property var modelData
                width: sidebar.width - 20
                icon: modelData.icon
                text: modelData.id
                badge: modelData.id === "Recents" ? "" : String(FileIndex.filesIn(modelData.id).length)
                selected: app.location === modelData.id
                onClicked: {
                    app.location = modelData.id;
                    app.selectedName = "";
                }
            }
        }
        SectionHeader { text: qsTr("Tags") }
        Repeater {
            model: FileIndex.tags()
            delegate: SidebarItem {
                required property var modelData
                width: sidebar.width - 20
                icon: "tag"
                iconColor: Oklch.color(0.7, 0.16, modelData.hue)
                text: modelData.id
                selected: app.location === modelData.id
                onClicked: {
                    app.location = modelData.id;
                    app.selectedName = "";
                }
            }
        }
    }

    // ── Toolbar ─────────────────────────────────────────────────────────────
    Item {
        id: toolbar
        anchors.left: sidebar.right
        anchors.right: parent.right
        height: app.topInset + 44

        Text {
            x: 20
            y: app.topInset + 6
            text: app.location
            font: Theme.title2
            color: Theme.textPrimary
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 16
            y: app.topInset + 6
            spacing: 6
            SegmentedControl {
                width: 84
                model: ["", ""]
                currentIndex: app.gridView ? 0 : 1
                onActivated: index => app.gridView = index === 0
                Row {
                    anchors.fill: parent
                    Icon { width: parent.width / 2; height: parent.height; size: 15; name: "layout-grid"; color: Theme.textSecondary }
                    Icon { width: parent.width / 2; height: parent.height; size: 15; name: "list"; color: Theme.textSecondary }
                }
            }
            SearchField {
                id: search
                width: 200
                placeholderText: qsTr("Search %1").arg(app.location)
            }
        }
    }

    // ── Content ─────────────────────────────────────────────────────────────
    Item {
        id: contentArea
        anchors.left: sidebar.right
        anchors.right: inspector.left
        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom

        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: 12
            visible: app.gridView
            clip: true
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 128)))
            cellHeight: 124
            model: app.entries
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }
            delegate: Item {
                id: cell
                required property var modelData
                required property int index
                readonly property bool isSelected: app.selectedName === modelData.name
                width: grid.cellWidth
                height: grid.cellHeight
                opacity: enter.value
                Spring { id: enter; value: 0; target: 1; preset: Motion.smooth }

                Surface {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: Tokens.radius.l
                    color: cell.isSelected ? Theme.fillSelected : cellHover.hovered ? Theme.fillControl : "transparent"
                }
                AppIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 14
                    size: 54
                    elevation: 2
                    glyph: cell.modelData.glyph
                    hue: cell.modelData.hue
                    chroma: cell.modelData.kind === "folder" ? 0.13 : 0.11
                }
                Text {
                    x: 8
                    width: parent.width - 16
                    y: 76
                    horizontalAlignment: Text.AlignHCenter
                    text: cell.modelData.name
                    textFormat: Text.PlainText
                    font: Theme.footnote
                    color: Theme.textPrimary
                    elide: Text.ElideMiddle
                    maximumLineCount: 2
                    wrapMode: Text.Wrap
                }
                HoverHandler { id: cellHover }
                TapHandler {
                    onTapped: app.selectedName = cell.modelData.name
                    onDoubleTapped: if (cell.modelData.kind === "folder") app.selectedName = cell.modelData.name
                }
            }
        }

        ListView {
            id: listView
            anchors.fill: parent
            anchors.margins: 10
            visible: !app.gridView
            clip: true
            model: app.entries
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }
            header: Item {
                width: listView.width
                height: 28
                Row {
                    x: 44
                    anchors.verticalCenter: parent.verticalCenter
                    Text { width: listView.width * 0.5; text: qsTr("Name"); font: Theme.caption; color: Theme.textTertiary }
                    Text { width: listView.width * 0.22; text: qsTr("Modified"); font: Theme.caption; color: Theme.textTertiary }
                    Text { text: qsTr("Size"); font: Theme.caption; color: Theme.textTertiary }
                }
            }
            delegate: Item {
                id: rowItem
                required property var modelData
                required property int index
                width: listView.width
                height: 34
                Surface {
                    anchors.fill: parent
                    radius: Tokens.radius.m
                    color: app.selectedName === rowItem.modelData.name ? Theme.fillSelected
                           : rowItem.index % 2 === 1 ? Theme.fillControl : "transparent"
                }
                AppIcon {
                    x: 10
                    anchors.verticalCenter: parent.verticalCenter
                    size: 22
                    elevation: 0
                    glyph: rowItem.modelData.glyph
                    hue: rowItem.modelData.hue
                    chroma: 0.11
                }
                Row {
                    x: 44
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: listView.width * 0.5
                        text: rowItem.modelData.name
                        textFormat: Text.PlainText
                        font: Theme.body
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        width: listView.width * 0.22
                        text: rowItem.modelData.modified
                        font: Theme.footnote
                        color: Theme.textSecondary
                    }
                    Text {
                        text: rowItem.modelData.size
                        font: Theme.footnote
                        color: Theme.textSecondary
                    }
                }
                TapHandler { onTapped: app.selectedName = rowItem.modelData.name }
            }
        }

        Column {
            anchors.centerIn: parent
            visible: app.entries.length === 0
            spacing: 8
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "search"; size: 32; color: Theme.textTertiary }
            Text { text: qsTr("No matching items"); font: Theme.headline; color: Theme.textSecondary }
        }
    }

    // ── Inspector ───────────────────────────────────────────────────────────
    Item {
        id: inspector
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        width: widthSpring.value
        clip: true

        Spring { id: widthSpring; target: app.selected ? 250 : 0; preset: Motion.smooth }

        Divider { vertical: true; height: parent.height }

        Column {
            x: 20
            y: 18
            width: 210
            spacing: 10
            visible: app.selected !== null

            Surface {
                width: 210
                height: 150
                radius: Tokens.radius.l
                color: Theme.surfaceSunken
                strokeColor: Theme.windowStroke
                AppIcon {
                    anchors.centerIn: parent
                    size: 84
                    elevation: 3
                    glyph: app.selected ? app.selected.glyph : "file"
                    hue: app.selected ? app.selected.hue : 248
                    chroma: 0.13
                }
            }
            Text {
                width: parent.width
                text: app.selected ? app.selected.name : ""
                textFormat: Text.PlainText
                font: Theme.headline
                color: Theme.textPrimary
                wrapMode: Text.Wrap
            }
            Text {
                text: app.selected ? app.selected.kindLabel + " · " + app.selected.size : ""
                font: Theme.footnote
                color: Theme.textSecondary
            }
            Divider { width: parent.width }
            Repeater {
                model: app.selected ? [[qsTr("Modified"), app.selected.modified], [qsTr("Where"), app.selected.path],
                                       [qsTr("Tag"), app.selected.tag.length ? app.selected.tag : "—"]] : []
                delegate: Column {
                    id: fact
                    required property var modelData
                    width: 210
                    Text { text: fact.modelData[0]; font: Theme.caption; color: Theme.textTertiary }
                    Text {
                        width: parent.width
                        text: fact.modelData[1]
                        font: Theme.footnote
                        color: Theme.textPrimary
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
            Button {
                text: qsTr("Open")
                highlighted: true
                icon.name: "arrow-up-right"
            }
        }
    }
}
