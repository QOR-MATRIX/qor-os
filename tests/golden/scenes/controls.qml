// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QOR.Kit

// Golden scene: the Lumen Qt Quick Controls style in its resting states.
Rectangle {
    property bool rtl: false

    width: 560
    height: 380
    color: Theme.background
    LayoutMirroring.enabled: rtl
    LayoutMirroring.childrenInherit: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            spacing: 8
            Button { text: "Continue"; highlighted: true }
            Button { text: "Cancel" }
            Button { text: "Flat"; flat: true }
            Button { text: "Disabled"; enabled: false }
            ToolButton { icon.name: "settings" }
        }

        RowLayout {
            spacing: 18
            CheckBox { text: "Checked"; checked: true }
            CheckBox { text: "Unchecked" }
            RadioButton { text: "Selected"; checked: true }
            Switch { text: "Wi-Fi"; checked: true }
            Switch { }
        }

        RowLayout {
            spacing: 16
            Slider { from: 0; to: 1; value: 0.65; Layout.preferredWidth: 220 }
            ProgressBar { from: 0; to: 1; value: 0.4; Layout.fillWidth: true }
        }

        RowLayout {
            spacing: 12
            TextField { text: "Lumen Studio"; Layout.preferredWidth: 220 }
            TextField { placeholderText: "Search files"; Layout.fillWidth: true }
        }

        TabBar {
            Layout.fillWidth: true
            currentIndex: 1
            TabButton { text: "General" }
            TabButton { text: "Appearance" }
            TabButton { text: "Privacy" }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label {
                anchors.fill: parent
                text: "Lumen controls follow the live theme: appearance, accent hue and contrast."
                wrapMode: Text.Wrap
            }
        }
    }
}
