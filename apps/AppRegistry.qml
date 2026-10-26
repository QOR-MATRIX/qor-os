// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma Singleton

import QtQuick
import QTOS.Kit
import "FileIndex.js" as FileIndex

// Catalogue of first-party apps: identity, icon, default window size, search
// keywords, and the component the shell instantiates inside a window.
// On device this metadata comes from Qt Application Manager manifests.
QtObject {
    id: registry

    readonly property var apps: [
        { id: "files", name: qsTr("Files"), glyph: "folder", hue: 248, chroma: 0.15, width: 1000, height: 640,
          keywords: "finder explorer documents folders browse photos downloads",
          description: qsTr("Browse documents, photos, music and network locations.") },
        { id: "music", name: qsTr("Music"), glyph: "music", hue: 355, chroma: 0.17, width: 940, height: 620,
          keywords: "audio songs player playlist album artist",
          description: qsTr("Your library with gapless playback and a live queue.") },
        { id: "notes", name: qsTr("Notes"), glyph: "notebook-pen", hue: 84, chroma: 0.15, width: 880, height: 600,
          keywords: "write text memo journal ideas",
          description: qsTr("Capture ideas in a calm, distraction-free editor.") },
        { id: "terminal", name: qsTr("Terminal"), glyph: "terminal", hue: 274, chroma: 0.035, width: 840, height: 540,
          keywords: "shell console command line cli bash",
          description: qsTr("A GPU-rendered command line wired into the QTOS services.") },
        { id: "monitor", name: qsTr("Monitor"), glyph: "activity", hue: 158, chroma: 0.15, width: 960, height: 620,
          keywords: "system cpu memory gpu performance activity frame time graphs",
          description: qsTr("Live CPU, GPU, memory and frame-time telemetry, drawn with Qt Graphs.") },
        { id: "settings", name: qsTr("Settings"), glyph: "settings", hue: 262, chroma: 0.03, width: 960, height: 660,
          keywords: "preferences control panel configure options system",
          description: qsTr("Appearance, display, sound, network and accessibility.") },
        { id: "store", name: qsTr("Store"), glyph: "shopping-bag", hue: 300, chroma: 0.17, width: 1100, height: 720,
          keywords: "shop market download install themes widgets apps games tools get",
          description: qsTr("Themes and widgets for QTOS, signed and verified.") }
    ]

    readonly property var pinned: ["files", "music", "notes", "terminal", "monitor", "store", "settings"]

    readonly property var settingsPages: [
        { id: "appearance", title: qsTr("Appearance"), icon: "palette",
          keywords: "dark light theme themes accent color wallpaper aurora living",
          description: qsTr("Themes, light and dark, accent color and the living wallpaper.") },
        { id: "display", title: qsTr("Display"), icon: "monitor",
          keywords: "brightness night light screen temperature",
          description: qsTr("Brightness and Night Light.") },
        { id: "sound", title: qsTr("Sound"), icon: "volume-2",
          keywords: "volume audio output speakers headphones mute equalizer eq bass treble sounds reactive visualizer spectrum",
          description: qsTr("Volume, output, equalizer, interface sounds and the audio-reactive interface.") },
        { id: "controllers", title: qsTr("Controllers"), icon: "gamepad-2",
          keywords: "controller gamepad game pad joystick xbox playstation ps4 ps5 dualshock dualsense stratus steelseries console vibration",
          description: qsTr("Game controllers and the controller interface.") },
        { id: "network", title: qsTr("Wi-Fi"), icon: "wifi",
          keywords: "wireless internet network connect ssid",
          description: qsTr("Wireless networks and connectivity.") },
        { id: "bluetooth", title: qsTr("Bluetooth"), icon: "bluetooth",
          keywords: "devices pair headphones keyboard mouse",
          description: qsTr("Paired devices and their batteries.") },
        { id: "notifications", title: qsTr("Notifications"), icon: "bell",
          keywords: "alerts banners focus do not disturb",
          description: qsTr("Banners and Focus.") },
        { id: "accessibility", title: qsTr("Accessibility"), icon: "accessibility",
          keywords: "reduce motion transparency contrast vision",
          description: qsTr("Motion, transparency and legibility.") },
        { id: "about", title: qsTr("About"), icon: "info",
          keywords: "version qt system information modules",
          description: qsTr("QTOS, Qt and graphics information.") }
    ]

    // Identities for system senders that are not apps (notifications).
    readonly property var senders: ({
        "system": { name: "QTOS", glyph: "sparkles", hue: Theme.hue, chroma: 0.16 },
        "messages": { name: qsTr("Messages"), glyph: "mail", hue: 205, chroma: 0.15 },
        "calendar": { name: qsTr("Calendar"), glyph: "calendar", hue: 25, chroma: 0.17 }
    })

    function app(id: string): var {
        for (const a of apps)
            if (a.id === id)
                return a;
        return null;
    }

    function identity(appId: string): var {
        const a = app(appId);
        if (a)
            return { name: a.name, glyph: a.glyph, hue: a.hue, chroma: a.chroma };
        return senders[appId] ?? { name: appId, glyph: "bell", hue: Theme.hue, chroma: 0.1 };
    }

    function files(): var {
        return FileIndex.allFiles();
    }

    function component(id: string): Component {
        switch (id) {
        case "files": return filesComponent;
        case "music": return musicComponent;
        case "notes": return notesComponent;
        case "terminal": return terminalComponent;
        case "monitor": return monitorComponent;
        case "settings": return settingsComponent;
        case "store": return storeComponent;
        }
        return null;
    }

    readonly property Component filesComponent: Component { FilesApp { } }
    readonly property Component musicComponent: Component { MusicApp { } }
    readonly property Component notesComponent: Component { NotesApp { } }
    readonly property Component terminalComponent: Component { TerminalApp { } }
    readonly property Component monitorComponent: Component { MonitorApp { } }
    readonly property Component settingsComponent: Component { SettingsApp { } }
    readonly property Component storeComponent: Component { StoreApp { } }
}
