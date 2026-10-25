// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit
import QTOS.Services

// Feedback for the volume keys: a glass pill with the output level, shown
// briefly while the volume changes from the keyboard.
Item {
    id: hud

    property Backdrop backdrop

    function show() {
        presence.target = 1;
        hideTimer.restart();
    }

    readonly property real level: Services.audio.muted ? 0 : Services.audio.volume

    width: 264
    height: 50
    visible: presence.value > 0.002
    opacity: presence.value
    scale: 0.92 + 0.08 * presence.value

    Spring { id: presence; target: 0; preset: Motion.snappy }
    Timer { id: hideTimer; interval: 1400; onTriggered: presence.target = 0 }

    Glass {
        anchors.fill: parent
        backdrop: hud.backdrop
        radius: height / 2
        elevation: 4
    }

    Row {
        anchors.centerIn: parent
        spacing: 12

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: Services.audio.muted ? "volume-x" : hud.level < 0.01 ? "volume" : hud.level < 0.5 ? "volume-1" : "volume-2"
            size: 18
            color: Theme.textPrimary
        }
        Surface {
            anchors.verticalCenter: parent.verticalCenter
            width: 160
            height: 6
            radius: 3
            exponent: 2
            color: Theme.fillControlPressed
            Surface {
                objectName: "volumeHudLevel"
                width: Math.max(height, parent.width * fill.value)
                height: parent.height
                radius: 3
                exponent: 2
                color: Theme.accent
                opacity: fill.value > 0.001 ? 1 : 0
                Spring { id: fill; target: hud.level; preset: Motion.snappy }
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: 36
            horizontalAlignment: Text.AlignRight
            text: Services.audio.muted ? qsTr("Off") : Math.round(hud.level * 100) + "%"
            font: Theme.footnote
            color: Theme.textSecondary
        }
    }
}
