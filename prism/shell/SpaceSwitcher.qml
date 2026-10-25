// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit

// Space chips shown at the top of Overview.
Item {
    id: switcher

    property real progress: 0
    property int current: 0
    property int count: 3
    property Backdrop backdrop
    // The chip a dragged window would move to (Overview).
    property int dropHint: -1

    signal selected(int index)

    // The chip under a scene point (with a little slack), or -1.
    function indexAt(scenePoint: point): int {
        if (!visible)
            return -1;
        for (let i = 0; i < chips.count; ++i) {
            const chip = chips.itemAt(i);
            const local = chip.mapFromItem(null, scenePoint.x, scenePoint.y);
            if (local.x >= -8 && local.y >= -12 && local.x < chip.width + 8 && local.y < chip.height + 12)
                return i;
        }
        return -1;
    }

    width: row.implicitWidth + 12
    height: 40
    visible: progress > 0.01
    opacity: progress
    y: -12 * (1 - progress)

    Glass {
        anchors.fill: parent
        backdrop: switcher.backdrop
        radius: 20
        elevation: 3
    }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 4
        Repeater {
            id: chips
            model: switcher.count
            delegate: Surface {
                id: chip
                required property int index
                readonly property bool selectedChip: index === switcher.current
                readonly property bool dropTarget: index === switcher.dropHint && index !== switcher.current
                objectName: "spaceChip" + index
                width: label.implicitWidth + 28
                height: 30
                radius: 15
                exponent: 2
                color: dropTarget ? Oklch.alpha(Theme.accent, 0.3)
                                  : selectedChip ? Theme.fillSelected
                                  : chipHover.hovered ? Theme.fillControlHover : "transparent"
                strokeColor: dropTarget ? Theme.accent : "transparent"
                strokeWidth: 1.5
                scale: dropTarget ? 1.08 : 1
                Behavior on scale { NumberAnimation { duration: 120 } }
                Behavior on color { ColorAnimation { duration: 140 } }
                Text {
                    id: label
                    anchors.centerIn: parent
                    text: qsTr("Space %1").arg(chip.index + 1)
                    font: chip.selectedChip ? Theme.bodyStrong : Theme.body
                    color: chip.selectedChip ? Theme.accentText : Theme.textSecondary
                }
                HoverHandler { id: chipHover }
                TapHandler { onTapped: switcher.selected(chip.index) }
            }
        }
    }
}
