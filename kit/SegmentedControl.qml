// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit

// A row of mutually exclusive segments with a sliding, spring-driven thumb.
Item {
    id: root

    property list<string> model
    property int currentIndex: 0
    signal activated(int index)

    implicitWidth: Math.max(160, row.implicitWidth + 4)
    implicitHeight: 28
    activeFocusOnTab: true

    Accessible.role: Accessible.PageTabList
    Keys.onLeftPressed: if (currentIndex > 0) { currentIndex--; activated(currentIndex) }
    Keys.onRightPressed: if (currentIndex < model.length - 1) { currentIndex++; activated(currentIndex) }

    readonly property real _segmentWidth: model.length > 0 ? (width - 4) / model.length : 0

    Surface {
        anchors.fill: parent
        radius: Tokens.radius.m
        color: Theme.fillControl
        strokeColor: root.activeFocus ? Theme.focusRing : "transparent"
        strokeWidth: 2
    }

    Spring {
        id: thumbX
        target: 2 + root.currentIndex * root._segmentWidth
        preset: Motion.snappy
    }

    Surface {
        x: thumbX.value
        y: 2
        width: root._segmentWidth
        height: root.height - 4
        radius: Tokens.radius.m - 2
        color: Theme.darkness > 0.5 ? Qt.lighter(Theme.surfaceRaised, 1.35) : Theme.surfaceRaised
        strokeColor: Theme.windowStroke
        elevation: 1
        shadowStrength: 0.6
    }

    Row {
        id: row
        x: 2
        anchors.verticalCenter: parent.verticalCenter
        Repeater {
            model: root.model
            delegate: Item {
                id: segment
                required property string modelData
                required property int index
                width: root._segmentWidth
                height: root.height - 4
                implicitWidth: label.implicitWidth + 24
                Text {
                    id: label
                    anchors.centerIn: parent
                    text: segment.modelData
                    font: segment.index === root.currentIndex ? Theme.bodyStrong : Theme.body
                    color: segment.index === root.currentIndex ? Theme.textPrimary : Theme.textSecondary
                }
                TapHandler {
                    onTapped: {
                        root.currentIndex = segment.index;
                        root.activated(segment.index);
                    }
                }
            }
        }
    }
}
