// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes
import QTOS.Kit
import QTOS.Input

// A controller button as it is printed on the controller in use: Xbox
// letters in their colours, PlayStation symbols, Nintendo letters, or the
// button's name (LB, L1, Menu, Options, ...).
Item {
    id: glyph

    property int button: Gamepads.South
    property string family: Gamepads.active.family ?? "xbox"
    property real size: 20
    // Overrides the printed name (for example "D-pad").
    property string text: ""

    readonly property bool face: button >= Gamepads.South && button <= Gamepads.North
    readonly property bool symbol: face && family === "playstation" && text.length === 0
    readonly property string label: text.length > 0 ? text : Gamepads.buttonName(button, family)
    readonly property color tint: {
        if (!face || family === "nintendo" || family === "generic")
            return Theme.textPrimary;
        const xbox = [145, 25, 250, 85];        // A green, B red, X blue, Y amber
        const playstation = [255, 20, 340, 175]; // cross, circle, square, triangle
        const hue = (family === "playstation" ? playstation : xbox)[button - Gamepads.South];
        return Oklch.color(Theme.darkness > 0.5 ? 0.78 : 0.55, 0.16, hue);
    }

    implicitWidth: face ? size : Math.max(size, name.implicitWidth + size * 0.7)
    implicitHeight: size

    Accessible.role: Accessible.StaticText
    Accessible.name: label

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: glyph.face ? Oklch.alpha(glyph.tint, 0.16) : Theme.fillControlPressed
        border.color: glyph.face ? Oklch.alpha(glyph.tint, 0.65) : Theme.windowStroke
        border.width: 1
    }

    Shape {
        id: shape
        visible: glyph.symbol
        anchors.centerIn: parent
        readonly property real s: glyph.size * 0.46
        width: s
        height: s
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: glyph.tint
            strokeWidth: Math.max(1.4, glyph.size / 13)
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg {
                path: {
                    const s = shape.s;
                    switch (glyph.button) {
                    case Gamepads.South: return `M 0 0 L ${s} ${s} M ${s} 0 L 0 ${s}`;
                    case Gamepads.East: return `M ${s / 2} 0 A ${s / 2} ${s / 2} 0 1 1 ${s / 2} ${s} A ${s / 2} ${s / 2} 0 1 1 ${s / 2} 0`;
                    case Gamepads.West: return `M 0 0 L ${s} 0 L ${s} ${s} L 0 ${s} Z`;
                    default: return `M ${s / 2} ${s * 0.04} L ${s} ${s * 0.9} L 0 ${s * 0.9} Z`;
                    }
                }
            }
        }
    }

    Text {
        id: name
        visible: !glyph.symbol
        anchors.centerIn: parent
        text: glyph.label
        font.family: Theme.body.family
        font.pixelSize: Math.round(glyph.size * (glyph.face ? 0.56 : 0.48))
        font.weight: Font.DemiBold
        color: glyph.tint
    }
}
