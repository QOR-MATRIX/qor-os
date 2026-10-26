// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QtQuick.Shapes
import QTOS.Kit

// Focus: a 25-minute timer. Tap to start or pause; press and hold to reset.
Item {
    id: widget

    property int length: 25 * 60
    property int remaining: length
    property bool running: false
    readonly property bool done: remaining === 0
    readonly property real ring: Math.min(width, height) - 40

    Timer {
        interval: 1000
        repeat: true
        running: widget.running
        onTriggered: {
            widget.remaining = Math.max(0, widget.remaining - 1);
            if (widget.remaining === 0)
                widget.running = false;
        }
    }

    Shape {
        id: dial
        anchors.centerIn: parent
        width: widget.ring
        height: widget.ring
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.fillControl
            strokeWidth: 5
            fillColor: "transparent"
            PathAngleArc {
                centerX: dial.width / 2; centerY: dial.height / 2
                radiusX: dial.width / 2 - 3; radiusY: radiusX
                startAngle: -90; sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: widget.done ? Theme.positive : Theme.accent
            strokeWidth: 5
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: dial.width / 2; centerY: dial.height / 2
                radiusX: dial.width / 2 - 3; radiusY: radiusX
                startAngle: -90
                sweepAngle: 360 * widget.remaining / widget.length
            }
        }
    }

    Column {
        anchors.centerIn: dial
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Math.floor(widget.remaining / 60) + ":" + String(widget.remaining % 60).padStart(2, "0")
            color: Theme.textPrimary
            font: Qt.font({ family: Theme.fontFamily, pixelSize: 24, weight: 520,
                            variableAxes: { "wght": 520, "opsz": 24 }, features: { "tnum": 1 } })
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: widget.done ? qsTr("Done") : widget.running ? qsTr("Focus") : qsTr("Tap to start")
            color: Theme.textTertiary
            font: Theme.caption
        }
    }

    TapHandler {
        onTapped: {
            if (widget.done)
                widget.remaining = widget.length;
            widget.running = !widget.running;
        }
        onLongPressed: {
            widget.running = false;
            widget.remaining = widget.length;
        }
    }
}
