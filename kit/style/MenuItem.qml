// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
import QtQuick
import QtQuick.Templates as T
import QOR.Kit

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 5
    horizontalPadding: 9
    spacing: 8
    font: Theme.body
    hoverEnabled: true

    contentItem: Item {
        readonly property real indicatorPadding: control.checkable && control.indicator
                                                 ? control.indicator.width + control.spacing : 0
        implicitWidth: row.implicitWidth + indicatorPadding
        implicitHeight: Math.max(row.implicitHeight, 18)
        Row {
            id: row
            x: parent.indicatorPadding
            anchors.verticalCenter: parent.verticalCenter
            spacing: control.spacing
            Icon {
                anchors.verticalCenter: parent.verticalCenter
                visible: control.icon.name.length > 0
                name: control.icon.name
                size: 15
                color: control.highlighted ? Theme.onAccent : Theme.textSecondary
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: control.text
                font: control.font
                color: !control.enabled ? Theme.textDisabled : control.highlighted ? Theme.onAccent : Theme.textPrimary
            }
        }
    }

    indicator: Icon {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checked
        name: "check"
        size: 14
        color: control.highlighted ? Theme.onAccent : Theme.accentText
    }

    arrow: Icon {
        x: control.mirrored ? control.leftPadding : control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.subMenu
        name: control.mirrored ? "chevron-left" : "chevron-right"
        size: 14
        color: control.highlighted ? Theme.onAccent : Theme.textTertiary
    }

    background: Surface {
        implicitWidth: 200
        implicitHeight: 28
        radius: Tokens.radius.m - 3
        color: control.highlighted ? Theme.accent : "transparent"
    }
}
