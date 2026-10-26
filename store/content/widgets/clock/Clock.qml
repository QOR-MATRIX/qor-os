// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit

// Clock: the time, large and light, with the day beneath it and the minute
// passing as a hairline.
Item {
    id: clock

    property date now: new Date()

    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: clock.now = new Date()
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: 22
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -4
        spacing: 0

        Text {
            objectName: "clockTime"
            width: parent.width
            text: clock.now.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
            color: Theme.textPrimary
            font: Qt.font({ family: Theme.fontFamily, pixelSize: 58, weight: 280, letterSpacing: -2,
                            variableAxes: { "wght": 280, "opsz": 32 } })
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 30
        }
        Text {
            width: parent.width
            text: clock.now.toLocaleDateString(Qt.locale(), "dddd, d MMMM")
            color: Theme.textSecondary
            font: Theme.callout
            elide: Text.ElideRight
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 22
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 18
        width: (parent.width - 44) * clock.now.getSeconds() / 59
        height: 2
        radius: 1
        color: Oklch.alpha(Theme.accent, 0.7)
        Behavior on width {
            enabled: !Theme.reduceMotion
            NumberAnimation { duration: 400; easing.type: Easing.OutCubic }
        }
    }
}
