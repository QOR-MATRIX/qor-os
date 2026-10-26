// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Shapes
import QTOS.Kit
import QTOS.Services

// System: the battery as a ring, the network beneath.
Item {
    id: widget

    readonly property int level: Services.connected ? Services.power.batteryLevel : 0
    readonly property bool charging: Services.connected && Services.power.charging
    readonly property color ringColor: charging ? Theme.positive
                                       : level <= 15 ? Theme.critical
                                       : level <= 30 ? Theme.warning : Theme.accent
    readonly property real ring: Math.min(width, height) - 64

    Shape {
        id: gauge
        anchors.horizontalCenter: parent.horizontalCenter
        y: 16
        width: widget.ring
        height: widget.ring
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.fillControl
            strokeWidth: 6
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: gauge.width / 2; centerY: gauge.height / 2
                radiusX: gauge.width / 2 - 4; radiusY: radiusX
                startAngle: -90; sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: widget.ringColor
            strokeWidth: 6
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: gauge.width / 2; centerY: gauge.height / 2
                radiusX: gauge.width / 2 - 4; radiusY: radiusX
                startAngle: -90; sweepAngle: 360 * widget.level / 100
            }
        }
    }

    Column {
        anchors.centerIn: gauge
        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: widget.charging ? "battery-charging" : "battery-full"
            size: 14
            color: Theme.textSecondary
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: widget.level + "%"
            color: Theme.textPrimary
            font: Theme.headline
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        spacing: 6
        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: !Services.connected || !Services.network.online ? "wifi-off"
                  : Services.network.signalBars >= 3 ? "wifi" : Services.network.signalBars === 2 ? "wifi-high"
                  : "wifi-low"
            size: 14
            color: Theme.textSecondary
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, widget.width - 52)
            text: Services.connected && Services.network.online ? Services.network.activeSsid : qsTr("Offline")
            color: Theme.textSecondary
            font: Theme.footnote
            elide: Text.ElideRight
        }
    }
}
