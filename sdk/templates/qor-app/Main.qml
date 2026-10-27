import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Services

// A minimal QOR OS app: Lumen controls, Theme colors, a spring, and an intent.
ApplicationWindow {
    width: 720
    height: 480
    visible: true
    title: qsTr("Hello, Lumen")
    color: Theme.surface

    Column {
        anchors.centerIn: parent
        spacing: Tokens.spacing.l

        AppIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            glyph: "sparkles"
            size: 72
            scale: bounce.value
            Spring { id: bounce; target: 1; preset: Motion.bouncy }
            TapHandler { onTapped: bounce.velocity = -6 }
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Hello, Lumen")
            font: Theme.title1
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Open Settings")
            highlighted: true
            icon.name: "settings"
            onClicked: Intents.openApp("settings", { page: "appearance" })
        }
    }
}
