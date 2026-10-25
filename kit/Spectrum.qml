// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QTOS.Kit

// A spectrum visualiser. `values` are bands from low to high frequencies
// (0..1, any number of them, e.g. Reactive.spectrum); they are drawn as `bars`
// rounded bars, each showing the loudest band it covers. Bars whose centre
// lies inside [highlightFrom, highlightTo) (fractions of the band range) use
// `color`; the rest use `dimColor`.
Item {
    id: root

    property list<real> values: []
    property int bars: 24
    property real spacing: 2
    // Resting height, as a fraction of the height.
    property real minimum: 0.06
    property color color: Theme.accent
    property color dimColor: Oklch.alpha(color, 0.32)
    property real highlightFrom: 0
    property real highlightTo: 1
    // Grow from the centre line instead of the bottom.
    property bool mirrored: false

    implicitWidth: 160
    implicitHeight: 40

    Repeater {
        model: root.bars
        delegate: Rectangle {
            id: bar
            required property int index
            readonly property real centre: (index + 0.5) / root.bars
            readonly property real value: {
                const count = root.values.length;
                if (count === 0)
                    return 0;
                const first = Math.floor(index * count / root.bars);
                const last = Math.max(first + 1, Math.floor((index + 1) * count / root.bars));
                let loudest = 0;
                for (let i = first; i < last; ++i)
                    loudest = Math.max(loudest, root.values[i]);
                return loudest;
            }
            readonly property real barWidth: (root.width - root.spacing * (root.bars - 1)) / root.bars

            x: index * (barWidth + root.spacing)
            width: barWidth
            height: Math.max(width, root.height * Math.max(root.minimum, Math.min(1, value)))
            y: root.mirrored ? (root.height - height) / 2 : root.height - height
            radius: width / 2
            antialiasing: true
            color: bar.centre >= root.highlightFrom && bar.centre < root.highlightTo ? root.color : root.dimColor
        }
    }
}
