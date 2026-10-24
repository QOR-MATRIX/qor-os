// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// Borderless icon button with Lumen hover, press and checked feedback.
Item {
    id: root

    property string icon
    property real iconSize: 16
    property color color: Theme.textSecondary
    property color hoverColor: Theme.textPrimary
    property bool checked: false
    property real radius: Tokens.radius.m
    readonly property bool hovered: hover.hovered
    readonly property bool pressed: tap.pressed

    signal clicked()

    implicitWidth: 28
    implicitHeight: 28
    activeFocusOnTab: true
    scale: press.value

    Accessible.role: Accessible.Button
    Accessible.name: icon
    Accessible.onPressAction: root.clicked()
    Keys.onSpacePressed: root.clicked()
    Keys.onReturnPressed: root.clicked()

    Spring {
        id: press
        target: tap.pressed ? 0.9 : 1
        preset: Motion.snappy
    }

    Surface {
        anchors.fill: parent
        radius: root.radius
        color: root.checked ? Theme.fillSelected
                            : tap.pressed ? Theme.fillControlPressed
                                          : hover.hovered ? Theme.fillControlHover : "transparent"
        strokeColor: root.activeFocus ? Theme.focusRing : "transparent"
        strokeWidth: 2
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    Icon {
        anchors.centerIn: parent
        name: root.icon
        size: root.iconSize
        color: root.checked ? Theme.accentText : hover.hovered ? root.hoverColor : root.color
    }

    HoverHandler { id: hover }
    TapHandler {
        id: tap
        onTapped: root.clicked()
    }
}
