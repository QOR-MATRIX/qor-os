// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
import QtQuick
import QTOS.Kit
import QTOS.Shell

// Top-level window used when the shell runs on a desktop OS (qtos-shell).
// On a device, Prism's compositor hosts the same Desktop on each output.
Window {
    id: window

    property var options: ({})

    width: options.width ?? 1600
    height: options.height ?? 1000
    minimumWidth: 1100
    minimumHeight: 720
    visible: true
    color: "black"
    title: qsTr("QTOS · Prism Shell (simulation)")
    visibility: options.fullscreen ? Window.FullScreen : Window.AutomaticVisibility

    Desktop {
        id: desktop
        objectName: "desktop"
        anchors.fill: parent
        options: window.options
        focus: true
    }

    Shortcut {
        sequence: "F11"
        onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed
                                                                                 : Window.FullScreen
    }
}
