// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Services
import QOR.Apps

// The lock screen. Owned by Prism, drawn above every client, never coverable.
// Unlocking runs the signature "focus pull": the blurred wallpaper sharpens
// while windows and chrome rise into place.
Item {
    id: lock

    property real progress: 1      // 1 = locked, 0 = unlocked
    property bool locked: true
    property Backdrop backdrop
    property ListModel history

    signal unlockRequested()

    visible: progress > 0.002
    opacity: progress
    enabled: locked

    property date now: new Date()
    Timer {
        running: lock.visible
        repeat: true
        interval: 1000
        onTriggered: lock.now = new Date()
    }

    onLockedChanged: {
        if (locked) {
            password.text = "";
            password.forceActiveFocus();
        }
    }
    Component.onCompleted: if (locked) password.forceActiveFocus()

    readonly property string userName: Services.settings.userName
    readonly property string initials: userName.split(" ").map(p => p.charAt(0)).join("").slice(0, 2).toUpperCase()

    TapHandler { onTapped: password.forceActiveFocus() }

    // ── Clock ───────────────────────────────────────────────────────────────
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: lock.height * 0.11 - 40 * (1 - lock.progress)
        spacing: -6

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: lock.now.toLocaleDateString(Qt.locale(), "dddd, d MMMM")
            font: Theme.title3
            color: Theme.textPrimary
            opacity: 0.85
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: lock.now.toLocaleTimeString(Qt.locale(), "h:mm")
            font: Theme.hero
            color: Theme.textPrimary
            renderType: Text.CurveRendering
        }
    }

    // ── Recent notifications ────────────────────────────────────────────────
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        y: lock.height * 0.42
        width: 380
        spacing: 8
        Repeater {
            model: lock.history
            delegate: Item {
                id: note
                required property int index
                required property string appId
                required property string title
                required property string body
                required property string icon
                readonly property var identity: AppRegistry.identity(appId)
                visible: index < 3
                width: parent.width
                height: visible ? 62 : 0

                Glass {
                    anchors.fill: parent
                    backdrop: lock.backdrop
                    radius: 16
                    elevation: 3
                }
                AppIcon {
                    id: noteIcon
                    x: 12
                    anchors.verticalCenter: parent.verticalCenter
                    size: 30
                    elevation: 1
                    glyph: note.icon.length > 0 ? note.icon : note.identity.glyph
                    hue: note.identity.hue
                    chroma: note.identity.chroma
                }
                Column {
                    anchors.left: noteIcon.right
                    anchors.leftMargin: 10
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: note.title
                        font: Theme.bodyStrong
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: note.body
                        font: Theme.footnote
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // ── User & password ─────────────────────────────────────────────────────
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 64 - 30 * (1 - lock.progress)
        spacing: 12

        Surface {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 72
            height: 72
            radius: 36
            exponent: 2
            color: Oklch.color(0.74, 0.14, Theme.hue + 40)
            gradientTo: Oklch.color(0.50, 0.17, Theme.hue - 20)
            strokeColor: Qt.rgba(1, 1, 1, 0.35)
            strokeWidth: 1.5
            highlight: 1
            elevation: 3
            Text {
                anchors.centerIn: parent
                text: lock.initials
                font: Theme.title2
                color: "white"
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: lock.userName
            font: Theme.headline
            color: Theme.textPrimary
        }

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 260
            height: 40

            Glass {
                anchors.fill: parent
                backdrop: lock.backdrop
                radius: 20
                elevation: 2
                strokeColor: password.activeFocus ? Oklch.alpha(Theme.accent, 0.7) : Theme.glassStroke
            }
            TextInput {
                id: password
                x: 18
                width: parent.width - 58
                anchors.verticalCenter: parent.verticalCenter
                echoMode: TextInput.Password
                passwordCharacter: "●"
                font: Theme.callout
                color: Theme.textPrimary
                clip: true
                focus: true
                Keys.onReturnPressed: lock.unlockRequested()
                Keys.onEnterPressed: lock.unlockRequested()
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: password.text.length === 0
                    text: qsTr("Password")
                    font: Theme.callout
                    color: Theme.textTertiary
                }
            }
            Surface {
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 30
                radius: 15
                exponent: 2
                color: Theme.accent
                highlight: 0.8
                Icon {
                    anchors.centerIn: parent
                    name: "arrow-right"
                    size: 16
                    color: Theme.onAccent
                }
                TapHandler { onTapped: lock.unlockRequested() }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Simulation: any password unlocks")
            font: Theme.caption
            color: Theme.textTertiary
        }
    }
}
