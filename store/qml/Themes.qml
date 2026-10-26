// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma Singleton

import QtQuick
import QTOS.Kit
import QTOS.Services

// The themes the user can choose from: Aurora, which QTOS ships with, and
// those installed from the Store. A theme is data only: an accent, a
// preferred appearance, a tint for the neutral surfaces and the wallpaper's
// character. Applying one sets the accent and appearance in Settings, so the
// user can still change either afterwards.
QtObject {
    id: themes

    readonly property var aurora: ({
        id: "",
        name: qsTr("Aurora"),
        developerName: "QTOS",
        builtIn: true,
        accentHue: Tokens.accentDefaultHue,
        appearance: "any",
        neutralHue: 262,
        neutralTint: 1,
        wallpaperSeed: 42,
        wallpaperIntensity: 1
    })
    readonly property var all: [aurora].concat(Store.themes)
    readonly property string activeId: Services.connected ? Services.settings.theme : ""
    readonly property var active: definition(activeId)

    function definition(id: string): var {
        for (const theme of Store.themes)
            if (theme.id === id)
                return theme;
        return aurora;
    }

    function apply(id: string) {
        const theme = definition(id);
        const settings = Services.settings;
        settings.theme = theme.id;
        settings.accentHue = theme.accentHue;
        if (theme.appearance === "dark" || theme.appearance === "light")
            settings.darkMode = theme.appearance === "dark";
    }
}
