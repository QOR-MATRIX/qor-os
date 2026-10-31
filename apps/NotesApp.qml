// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QOR.Kit

// Notes: a calm editor with a note list. Notes persist locally with
// QtCore.Settings (qor-settingsd-backed storage on device).
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})
    property int current: 0

    ListModel { id: notes }

    Settings {
        id: store
        category: "notes"
        property string json: ""
    }

    function save() {
        const out = [];
        for (let i = 0; i < notes.count; ++i) {
            const n = notes.get(i);
            out.push({ title: n.title, body: n.body, edited: n.edited });
        }
        store.json = JSON.stringify(out);
    }

    function addNote() {
        notes.insert(0, { title: "", body: "", edited: new Date().toLocaleString(Qt.locale(), Locale.ShortFormat) });
        current = 0;
        titleField.forceActiveFocus();
        save();
    }

    Component.onCompleted: {
        let loaded = [];
        try {
            loaded = store.json.length > 0 ? JSON.parse(store.json) : [];
        } catch (e) {
            loaded = [];
        }
        if (loaded.length === 0) {
            loaded = [
                { title: qsTr("Welcome to Notes"), edited: qsTr("Today"),
                  body: qsTr("Everything here is a Qt Quick TextArea styled by Lumen.\n\nTry switching to light appearance from Control Center: the editor, the sidebar frost and the wallpaper all morph together.") },
                { title: qsTr("Prism ideas"), edited: qsTr("Yesterday"),
                  body: qsTr("• Hero transitions between apps\n• Stacks: tabs for any window\n• Spatial mode with Qt Quick 3D Xr") },
                { title: qsTr("Groceries"), edited: qsTr("Mon"), body: qsTr("Oat milk\nLemons\nSourdough\nCoffee beans") }
            ];
        }
        for (const n of loaded)
            notes.append(n);
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    FrostSidebar {
        id: sidebar
        width: 250
        height: parent.height
        topInset: app.topInset
        spacing: 4

        Item {
            width: sidebar.width - 20
            height: 34
            Text {
                anchors.verticalCenter: parent.verticalCenter
                x: 6
                text: qsTr("Notes")
                font: Theme.title3
                color: Theme.textPrimary
            }
            IconButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                icon: "plus"
                onClicked: app.addNote()
            }
        }

        Repeater {
            model: notes
            delegate: Item {
                id: entry
                required property int index
                required property string title
                required property string body
                required property string edited
                width: sidebar.width - 20
                height: 60
                Surface {
                    anchors.fill: parent
                    radius: Tokens.radius.l
                    color: app.current === entry.index ? Theme.fillSelected : entryHover.hovered ? Theme.fillControl : "transparent"
                }
                Column {
                    x: 12
                    width: parent.width - 24
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: entry.title.length > 0 ? entry.title : qsTr("New Note")
                        font: Theme.bodyStrong
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: entry.edited + "  " + entry.body.split("\n")[0]
                        textFormat: Text.PlainText
                        font: Theme.footnote
                        color: Theme.textTertiary
                        elide: Text.ElideRight
                    }
                }
                HoverHandler { id: entryHover }
                TapHandler { onTapped: app.current = entry.index }
            }
        }
    }

    // ── Editor ──────────────────────────────────────────────────────────────
    Item {
        id: editor
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: app.topInset
        anchors.bottom: parent.bottom
        readonly property bool hasNote: app.current >= 0 && app.current < notes.count

        TextField {
            id: titleField
            x: 36
            y: 16
            width: parent.width - 72
            enabled: editor.hasNote
            text: editor.hasNote ? notes.get(app.current).title : ""
            placeholderText: qsTr("Title")
            font: Theme.title1
            background: Item { }
            leftPadding: 0
            onTextEdited: {
                notes.setProperty(app.current, "title", text);
                app.save();
            }
        }

        ScrollView {
            x: 30
            anchors.top: titleField.bottom
            anchors.topMargin: 4
            width: parent.width - 60
            anchors.bottom: footer.top
            TextArea {
                id: body
                enabled: editor.hasNote
                text: editor.hasNote ? notes.get(app.current).body : ""
                placeholderText: qsTr("Start writing…")
                font: Theme.callout
                leftPadding: 6
                onTextChanged: {
                    if (!activeFocus || !editor.hasNote)
                        return;
                    notes.setProperty(app.current, "body", text);
                    app.save();
                }
            }
        }

        Text {
            id: footer
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
            readonly property int words: body.text.trim().length === 0 ? 0 : body.text.trim().split(/\s+/).length
            text: qsTr("%n word(s)", "", words)
            font: Theme.caption
            color: Theme.textTertiary
        }
    }
}
