// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit

// Another display, when Prism drives several outputs: the wallpaper and the
// windows on it. The main display's Desktop keeps the Rail, the Shelf,
// notifications and every other surface. Spaces, Overview and the lock
// screen span all displays, so this one follows the main one, and windows
// move between displays (WindowLayer.moveToDisplay).
FocusScope {
    id: display

    required property Desktop primary
    readonly property alias windowLayer: windows
    readonly property rect workArea: Qt.rect(10, 10, width - 20, height - 20)

    Item {
        id: content
        anchors.fill: parent

        Wallpaper {
            id: wallpaper
            anchors.fill: parent
            parallax: Qt.vector2d(display.primary.spaceOffset * 0.03, 0)
            zoom: 1 + 0.06 * display.primary.lockProgress + 0.03 * display.primary.overviewProgress
        }

        // Focus pull for Overview and the lock screen, as on the main display.
        ShaderEffect {
            anchors.fill: parent
            property variant source: wallpaperBackdrop.texture
            opacity: Math.max(display.primary.lockProgress, display.primary.overviewProgress * 0.9)
            visible: opacity > 0.002
        }
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.16 * display.primary.lockProgress + 0.10 * display.primary.overviewProgress
            visible: opacity > 0.002
        }

        WindowLayer {
            id: windows
            objectName: "windowLayer"
            anchors.fill: parent
            workArea: display.workArea
            displays: display.primary.displays
            currentSpace: display.primary.currentSpace
            spaceOffset: display.primary.spaceOffset
            overview: display.primary.overview
            surfaceDelegate: display.primary.windowLayer.surfaceDelegate
            enabled: !display.primary.locked
            locked: display.primary.locked
            surfaceOpen: display.primary.surface !== ""
            opacity: 1 - display.primary.lockProgress
            scale: 1 - 0.035 * display.primary.lockProgress
            visible: opacity > 0.002
            onOverviewPicked: display.primary.setSurface("")
            onSpaceRequested: index => display.primary.switchSpace(index)
        }
    }

    Backdrop { id: wallpaperBackdrop; sourceItem: wallpaper; live: true }

    // In Overview, which display this is.
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: 22
        opacity: display.primary.overviewProgress
        visible: opacity > 0.01
        text: qsTr("Display %1").arg(windows.displayIndex + 1)
        color: Theme.textSecondary
        font: Theme.headline
    }

    Component.onCompleted: {
        display.primary.registerDisplay(windows);
        Materials.displayDesktops = Materials.displayDesktops.concat([{ window: Window.window, backdrop: wallpaperBackdrop }]);
    }
    Component.onDestruction: {
        display.primary.unregisterDisplay(windows);
        Materials.displayDesktops = Materials.displayDesktops.filter(entry => entry.backdrop !== wallpaperBackdrop);
    }
}
