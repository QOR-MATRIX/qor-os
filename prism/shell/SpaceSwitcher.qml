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

    signal selected(int index)

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
            model: switcher.count
            delegate: Surface {
                id: chip
                required property int index
                readonly property bool selectedChip: index === switcher.current
                width: label.implicitWidth + 28
                height: 30
                radius: 15
                exponent: 2
                color: selectedChip ? Theme.fillSelected : chipHover.hovered ? Theme.fillControlHover : "transparent"
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
