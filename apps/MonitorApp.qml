// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtGraphs
import QTOS.Kit
import QTOS.Services

// Monitor: live system telemetry rendered with Qt Graphs. CPU and GPU load
// are simulated; the frame-time panel measures this very window's render loop.
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})

    readonly property int samples: 60
    property real cpu: 18
    property real gpu: 12
    property real memory: 41
    property var cpuHistory: []
    property var gpuHistory: []

    function step() {
        const drift = (v, lo, hi, amp) => Math.max(lo, Math.min(hi, v + (Math.random() - 0.5) * amp));
        cpu = drift(cpu, 4, 96, 14);
        gpu = drift(gpu, 2, 92, 10 + (Services.settings.livingWallpaper ? 6 : 0));
        memory = drift(memory, 30, 74, 1.5);
        cpuHistory = cpuHistory.concat([cpu]).slice(-samples);
        gpuHistory = gpuHistory.concat([gpu]).slice(-samples);
        cpuLine.replace(cpuHistory.map((v, i) => Qt.point(samples - cpuHistory.length + i, v)));
        gpuLine.replace(gpuHistory.map((v, i) => Qt.point(samples - gpuHistory.length + i, v)));
    }

    Timer {
        interval: 500
        repeat: true
        running: app.visible
        onTriggered: app.step()
    }

    Component.onCompleted: {
        // Seed thirty seconds of history so the chart opens full.
        for (let i = 0; i < samples; ++i) {
            cpu = Math.max(4, Math.min(96, cpu + (Math.random() - 0.5) * 14));
            gpu = Math.max(2, Math.min(92, gpu + (Math.random() - 0.5) * 10));
            cpuHistory.push(cpu);
            gpuHistory.push(gpu);
        }
        step();
    }

    // Real frame pacing of this window, measured passively: Prism renders on
    // demand, so an idle desktop reports "idle" instead of a frame rate.
    FrameStats {
        id: frames
        item: app
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    Item {
        anchors.fill: parent
        anchors.topMargin: app.topInset
        anchors.margins: 18

        Text {
            id: heading
            text: qsTr("System")
            font: Theme.title2
            color: Theme.textPrimary
        }

        Row {
            id: tiles
            anchors.top: heading.bottom
            anchors.topMargin: 14
            width: parent.width
            spacing: 12
            StatTile { title: qsTr("CPU"); value: Math.round(app.cpu) + "%"; hue: Theme.hue; fraction: app.cpu / 100 }
            StatTile { title: qsTr("GPU"); value: Math.round(app.gpu) + "%"; hue: Theme.hue + 120; fraction: app.gpu / 100 }
            StatTile { title: qsTr("Memory"); value: (app.memory * 0.32).toFixed(1) + " GB"; hue: 60; fraction: app.memory / 100 }
            StatTile {
                title: qsTr("Frame time")
                value: frames.idle ? qsTr("Idle") : frames.frameMs.toFixed(1) + " ms"
                hue: 150
                fraction: frames.idle ? 0 : Math.min(1, frames.frameMs / 33.3)
                caption: frames.idle ? qsTr("Rendering on demand") : Math.round(frames.fps) + " fps"
            }
        }

        Card {
            anchors.top: tiles.bottom
            anchors.topMargin: 14
            anchors.bottom: parent.bottom
            width: parent.width
            padding: 14

            Row {
                spacing: 18
                Text { text: qsTr("Load, last 30 seconds"); font: Theme.headline; color: Theme.textPrimary }
                Legend { label: qsTr("CPU"); swatch: Theme.accent }
                Legend { label: qsTr("GPU"); swatch: Oklch.color(0.72, 0.15, Theme.hue + 120) }
            }

            GraphsView {
                width: parent.width
                height: parent.parent.height - 70
                marginLeft: 0
                marginRight: 8
                theme: GraphsTheme {
                    colorScheme: Theme.darkness > 0.5 ? GraphsTheme.ColorScheme.Dark : GraphsTheme.ColorScheme.Light
                    backgroundVisible: false
                    plotAreaBackgroundVisible: false
                    gridVisible: true
                    grid.mainColor: Theme.separator
                    grid.subColor: "transparent"
                    labelTextColor: Theme.textTertiary
                    labelFont: Theme.caption
                    axisX.mainColor: Theme.separator
                    axisY.mainColor: "transparent"
                }
                axisX: ValueAxis {
                    min: 0
                    max: app.samples - 1
                    labelsVisible: false
                    gridVisible: false
                    subGridVisible: false
                }
                axisY: ValueAxis {
                    min: 0
                    max: 100
                    tickInterval: 25
                    subGridVisible: false
                    labelFormat: "%.0f%%"
                }

                AreaSeries {
                    color: Oklch.alpha(Theme.accent, 0.16)
                    borderColor: Theme.accent
                    borderWidth: 2
                    upperSeries: LineSeries { id: cpuLine }
                }
                SplineSeries {
                    id: gpuLine
                    color: Oklch.color(0.72, 0.15, Theme.hue + 120)
                    width: 2
                }
            }
        }
    }

    component StatTile: Card {
        id: tile
        property string title
        property string value
        property string caption: ""
        property real hue: 274
        property real fraction: 0
        width: (parent.width - 36) / 4
        padding: 14
        spacing: 6

        Text { text: tile.title; font: Theme.footnote; color: Theme.textSecondary }
        Text { text: tile.value; font: Theme.title1; color: Theme.textPrimary }
        Item {
            width: parent.width
            height: 6
            Surface {
                anchors.fill: parent
                radius: 3
                exponent: 2
                color: Theme.fillControlPressed
            }
            Surface {
                width: Math.max(6, parent.width * fill.value)
                height: parent.height
                radius: 3
                exponent: 2
                color: Oklch.color(0.70, 0.16, tile.hue)
                Spring { id: fill; target: tile.fraction; preset: Motion.smooth }
            }
        }
        Text {
            visible: tile.caption.length > 0
            text: tile.caption
            font: Theme.caption
            color: Theme.textTertiary
        }
    }

    component Legend: Row {
        id: legend
        property string label
        property color swatch
        spacing: 6
        anchors.verticalCenter: parent ? parent.verticalCenter : undefined
        Surface { anchors.verticalCenter: parent.verticalCenter; width: 10; height: 10; radius: 5; exponent: 2; color: legend.swatch }
        Text { anchors.verticalCenter: parent.verticalCenter; text: legend.label; font: Theme.footnote; color: Theme.textSecondary }
    }
}
