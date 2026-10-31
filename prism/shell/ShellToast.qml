// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QOR.Kit

// A brief, quiet confirmation from the shell itself ("Controller interface
// on"), shown under the Rail for a couple of seconds.
Item {
    id: toast

    property Backdrop backdrop
    property string icon
    property string text

    function show(iconName: string, message: string) {
        icon = iconName;
        text = message;
        presence.target = 1;
        hide.restart();
    }

    width: row.implicitWidth + 32
    height: 40
    visible: presence.value > 0.002
    opacity: presence.value
    scale: 0.94 + 0.06 * presence.value

    Spring { id: presence; target: 0; preset: Motion.snappy }
    Timer { id: hide; interval: 2600; onTriggered: presence.target = 0 }

    Glass {
        anchors.fill: parent
        backdrop: toast.backdrop
        radius: height / 2
        elevation: 4
    }
    Row {
        id: row
        anchors.centerIn: parent
        spacing: 10
        Icon { anchors.verticalCenter: parent.verticalCenter; name: toast.icon; size: 17; color: Theme.accentText }
        Text { anchors.verticalCenter: parent.verticalCenter; text: toast.text; textFormat: Text.PlainText; font: Theme.callout; color: Theme.textPrimary }
    }
}
