// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Layouts
import QOR.Kit

// Golden scene: QOR.Kit building blocks on an opaque background.
Rectangle {
    property bool rtl: false

    width: 560
    height: 380
    color: Theme.background
    LayoutMirroring.enabled: rtl
    LayoutMirroring.childrenInherit: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // A sidebar column.
        Surface {
            Layout.preferredWidth: 180
            Layout.fillHeight: true
            color: Theme.surfaceSunken
            radius: Tokens.radius.l

            Column {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 2
                SectionHeader { width: parent.width; text: "Locations" }
                SidebarItem { width: parent.width; icon: "clock"; text: "Recents"; selected: true }
                SidebarItem { width: parent.width; icon: "file-text"; text: "Documents"; badge: "6" }
                SidebarItem { width: parent.width; icon: "music"; text: "Music"; badge: "4" }
                Divider { width: parent.width }
                SectionHeader { width: parent.width; text: "Tags" }
                SidebarItem { width: parent.width; icon: "tag"; text: "Lumen"; iconColor: Theme.accentText }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            RowLayout {
                spacing: 12
                QorMark { size: 28 }
                AppIcon { glyph: "folder"; hue: 230; size: 40 }
                AppIcon { glyph: "music"; hue: 0; size: 40 }
                AppIcon { glyph: "notebook-pen"; hue: 85; size: 40 }
                AppIcon { glyph: "terminal"; hue: 262; chroma: 0.03; size: 40 }
                Item { Layout.fillWidth: true }
                IconButton { icon: "search" }
                IconButton { icon: "bell"; checked: true }
            }

            SearchField {
                Layout.fillWidth: true
                placeholderText: "Search apps, files and actions"
            }

            SegmentedControl {
                model: ["Day", "Week", "Month"]
                currentIndex: 1
            }

            RowLayout {
                spacing: 12
                Repeater {
                    model: [1, 3, 5]
                    Surface {
                        required property int modelData
                        Layout.preferredWidth: 96
                        Layout.preferredHeight: 64
                        color: Theme.surfaceRaised
                        elevation: modelData
                        Text {
                            anchors.centerIn: parent
                            text: "Elevation " + parent.modelData
                            font: Theme.footnote
                            color: Theme.textSecondary
                        }
                    }
                }
            }

            Row {
                spacing: 6
                Kbd { text: "Ctrl" }
                Kbd { text: "Space" }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "opens Beam"
                    font: Theme.body
                    color: Theme.textTertiary
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
