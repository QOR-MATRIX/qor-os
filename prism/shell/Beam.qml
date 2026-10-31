// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit
import QOR.Services
import QOR.Store
import QOR.Apps
import "search.js" as Search

// Beam: the universal command bar. One field searches apps, actions, settings
// and files, evaluates arithmetic, and previews the selected result.
Item {
    id: beam

    property bool open: false
    property Backdrop backdrop
    property alias query: field.text

    signal dismissed()
    signal actionRequested(string id, var args)

    visible: presence.value > 0.002

    Spring { id: presence; target: beam.open ? 1 : 0; preset: Motion.snappy }

    onOpenChanged: {
        if (open) {
            field.text = "";
            refresh();
            field.forceActiveFocus();
        }
    }

    // ── Catalogue ───────────────────────────────────────────────────────────
    function catalog(): var {
        const apps = AppRegistry.apps.map(a => ({
            kind: "app", id: a.id, title: a.name, subtitle: qsTr("Application"), keywords: a.keywords,
            icon: a.glyph, hue: a.hue, chroma: a.chroma, description: a.description,
            action: "app.open", args: JSON.stringify({ id: a.id })
        }));
        // The products Prism hosts that open by themselves (ARQADE), where
        // they are installed and Prism can show them.
        for (const p of ProductHost.products)
            if (p.launchable && p.available)
                apps.push({
                    kind: "app", id: p.id, title: p.name, subtitle: qsTr("Application"), keywords: p.keywords,
                    icon: p.glyph, hue: p.hue, chroma: p.chroma, description: p.description,
                    action: "app.open", args: JSON.stringify({ id: p.id })
                });
        // Every game in the Library (library.h), played as from Home.
        for (const g of Library.games)
            apps.push({
                kind: "app", id: g.id, title: g.title, subtitle: qsTr("Game"), keywords: "game play " + g.subtitle,
                icon: g.glyph, hue: g.hue, chroma: 0.15, description: g.subtitle,
                action: "library.play", args: JSON.stringify({ id: g.id })
            });

        const s = Services.settings;
        const playing = Services.media.playing;
        const action = (id, title, icon, keywords, description, args) => ({
            kind: "action", id: id, title: title, subtitle: qsTr("Action"), keywords: keywords, icon: icon,
            hue: Theme.hue, chroma: 0.0, description: description, action: id,
            args: JSON.stringify(args === undefined ? {} : args)
        });
        const actions = [
            action("home.open", qsTr("Home"), "house", "home games play continue controller",
                   qsTr("What you played last, every game, and your newest media.")),
            action("library.open", qsTr("Library"), "library", "library games media videos music pictures",
                   qsTr("Every game and every film, song and picture, by what it is or who made it.")),
            action("appearance.toggle", s.darkMode ? qsTr("Switch to Light Appearance") : qsTr("Switch to Dark Appearance"),
                   s.darkMode ? "sun" : "moon", "theme dark light mode appearance",
                   qsTr("Every color morphs through OKLab as the appearance spring settles.")),
            action("shell.overview", qsTr("Show Overview"), "layout-grid", "expose windows spaces mission",
                   qsTr("See every window on this Space at once.")),
            action("media.toggle", playing ? qsTr("Pause Music") : qsTr("Play Music"), playing ? "pause" : "play",
                   "music media playback", Services.media.title + " — " + Services.media.artist),
            action("session.lock", qsTr("Lock Screen"), "lock", "lock sleep away security",
                   qsTr("The lock screen is drawn by Prism; apps can never cover it.")),
            action("shell.control-center", qsTr("Open Control Center"), "sliders-horizontal", "quick settings toggles",
                   qsTr("Connectivity, display, sound and media at a glance.")),
            action("media.next", qsTr("Next Track"), "skip-forward", "music skip song", qsTr("Skip to the next song.")),
            action("wifi.toggle", Services.network.wifiEnabled ? qsTr("Turn Wi-Fi Off") : qsTr("Turn Wi-Fi On"),
                   Services.network.wifiEnabled ? "wifi-off" : "wifi", "wireless network internet",
                   qsTr("Handled by qor-netd.")),
            action("bluetooth.toggle", Services.bluetooth.enabled ? qsTr("Turn Bluetooth Off") : qsTr("Turn Bluetooth On"),
                   "bluetooth", "wireless devices headphones", qsTr("Handled by qor-btd.")),
            action("dnd.toggle", Services.notifications.doNotDisturb ? qsTr("Turn Off Do Not Disturb")
                                                                     : qsTr("Turn On Do Not Disturb"),
                   "moon-star", "focus notifications silence quiet", qsTr("Silence banners; nothing is lost.")),
            action("motion.toggle", s.reduceMotion ? qsTr("Turn Off Reduce Motion") : qsTr("Turn On Reduce Motion"),
                   "accessibility", "animation accessibility vestibular",
                   qsTr("Every spring becomes an instant, calm transition.")),
            action("transparency.toggle", s.reduceTransparency ? qsTr("Turn Off Reduce Transparency")
                                                               : qsTr("Turn On Reduce Transparency"),
                   "layers", "glass blur accessibility opaque", qsTr("Glass becomes solid for legibility.")),
            action("contrast.toggle", s.increaseContrast ? qsTr("Turn Off Increase Contrast")
                                                         : qsTr("Turn On Increase Contrast"),
                   "contrast", "accessibility legibility vision text outline",
                   qsTr("Stronger text, outlines and separators; glass becomes solid.")),
            action("wallpaper.living", s.livingWallpaper ? qsTr("Stop Living Wallpaper") : qsTr("Start Living Wallpaper"),
                   "sparkles", "wallpaper animate aurora background", qsTr("Let the Aurora wallpaper drift slowly.")),
            action("reactive.toggle", s.audioReactive ? qsTr("Stop Reacting to Sound") : qsTr("React to Sound"),
                   "activity", "audio reactive visualizer music beat wallpaper spectrum",
                   qsTr("The wallpaper, Now Playing and the Rail move with what's playing.")),
            action("notify.test", qsTr("Send Test Notification"), "bell-ring", "notification banner",
                   qsTr("Posts through qor-notifyd over Qt Remote Objects.")),
            action("widgets.edit", qsTr("Edit Widgets"), "layout-grid", "widgets desktop add arrange clock calendar",
                   qsTr("Add, arrange and remove the widgets on your desktop.")),
            action("store.open", qsTr("Get Themes and Widgets"), "shopping-bag", "store shop download themes widgets",
                   qsTr("Signed themes and widgets from the QOR OS Store."), { page: "discover" })
        ];
        for (const theme of Themes.all)
            actions.push(action("theme.apply", qsTr("Theme: %1").arg(theme.name), "palette",
                                "theme look style " + theme.name.toLowerCase(),
                                qsTr("Sets the accent, the surfaces' tint and the wallpaper."), { id: theme.id }));
        for (let i = 0; i < 3; ++i)
            actions.push(action("space.go", qsTr("Go to Space %1").arg(i + 1), "layers", "space desktop workspace switch",
                                qsTr("Switch the window strip to Space %1.").arg(i + 1), { index: i }));
        for (let i = 0; i < 3; ++i)
            actions.push(action("window.space", qsTr("Move Window to Space %1").arg(i + 1), "layers",
                                "move window space desktop workspace send",
                                qsTr("Send the active window to Space %1, and go there.").arg(i + 1), { index: i }));
        const accents = [[qsTr("Indigo"), 274], [qsTr("Blue"), 248], [qsTr("Teal"), 205], [qsTr("Green"), 162],
                         [qsTr("Amber"), 95], [qsTr("Orange"), 58], [qsTr("Red"), 25], [qsTr("Pink"), 338]];
        for (const a of accents)
            actions.push(Object.assign(action("accent.set", qsTr("Accent Color: %1").arg(a[0]), "palette",
                                              "accent color theme tint " + a[0].toLowerCase(),
                                              qsTr("Re-tints the whole system and the wallpaper."), { hue: a[1] }),
                                       { hue: a[1], chroma: 0.17 }));

        const settings = AppRegistry.settingsPages.map(p => ({
            kind: "setting", id: "settings-" + p.id, title: qsTr("Settings › %1").arg(p.title), subtitle: qsTr("Setting"),
            keywords: p.keywords, icon: p.icon, hue: 262, chroma: 0.03, description: p.description,
            action: "app.open", args: JSON.stringify({ id: "settings", page: p.id })
        }));
        settings.push({
            kind: "setting", id: "settings-sound-reactive", title: qsTr("Settings › Sound › Audio-reactive interface"),
            subtitle: qsTr("Setting"), keywords: "audio reactive intensity frequency bass visualizer beat",
            icon: "activity", hue: 262, chroma: 0.03,
            description: qsTr("Intensity and the frequencies the interface reacts to."),
            action: "app.open", args: JSON.stringify({ id: "settings", page: "sound", section: "reactive" })
        });

        const files = AppRegistry.files().map(f => ({
            kind: "file", id: f.path, title: f.name, subtitle: f.folder, keywords: f.kind + " " + f.folder,
            icon: f.glyph, hue: f.hue, chroma: 0.12, description: f.kindLabel + " · " + f.size + " · " + f.modified,
            action: "app.open", args: JSON.stringify({ id: "files", folder: f.folder, select: f.name })
        }));

        return { apps: apps, actions: actions, settings: settings, files: files };
    }

    function refresh() {
        const found = Search.search(field.text, catalog(), 12);
        results.clear();
        for (const r of found)
            results.append(r);
        list.currentIndex = results.count > 0 ? 0 : -1;
    }

    function activate(index: int) {
        if (index < 0 || index >= results.count)
            return;
        const r = results.get(index);
        if (r.kind === "calc") {
            clipboard.text = r.title;
            clipboard.selectAll();
            clipboard.copy();
            beam.dismissed();
            return;
        }
        beam.actionRequested(r.action, JSON.parse(r.args));
    }

    ListModel { id: results }
    TextEdit { id: clipboard; visible: false }

    // ── Scrim ───────────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        opacity: presence.value
        TapHandler { onTapped: beam.dismissed() }
    }

    // ── Panel ───────────────────────────────────────────────────────────────
    Item {
        id: panel
        width: Math.min(860, beam.width - 80)
        height: 74 + Math.min(440, Math.max(list.contentHeight, 120)) + 40
        x: (beam.width - width) / 2
        y: beam.height * 0.16 - 16 * (1 - presence.value)
        opacity: presence.value
        scale: 0.97 + 0.03 * presence.value

        Glass {
            anchors.fill: parent
            backdrop: beam.backdrop
            radius: 24
            elevation: 5
            highlight: 1
        }

        // Search field.
        Item {
            id: searchHeader
            width: parent.width
            height: 72

            Icon {
                x: 24
                anchors.verticalCenter: parent.verticalCenter
                name: "search"
                size: 22
                color: Theme.textSecondary
            }
            TextInput {
                id: field
                x: 60
                width: parent.width - 84
                anchors.verticalCenter: parent.verticalCenter
                font: Theme.title2
                color: Theme.textPrimary
                selectionColor: Theme.fillSelected
                selectedTextColor: Theme.textPrimary
                clip: true
                onTextChanged: beam.refresh()
                Keys.onDownPressed: list.incrementCurrentIndex()
                Keys.onUpPressed: list.decrementCurrentIndex()
                Keys.onReturnPressed: beam.activate(list.currentIndex)
                Keys.onEnterPressed: beam.activate(list.currentIndex)
                Keys.onEscapePressed: beam.dismissed()

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: field.text.length === 0
                    text: qsTr("Search apps, actions, settings and files")
                    font: Theme.title2
                    color: Theme.textTertiary
                }
            }
        }

        Divider { y: searchHeader.height; width: parent.width }

        // Results.
        ListView {
            id: list
            x: 10
            y: searchHeader.height + 8
            width: parent.width * 0.56 - 10
            height: Math.min(440, Math.max(contentHeight, 120))
            clip: true
            model: results
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: 0
            currentIndex: -1

            section.property: "section"
            section.delegate: Text {
                required property string section
                width: ListView.view ? ListView.view.width : 0
                leftPadding: 12
                topPadding: 8
                bottomPadding: 4
                text: section
                font: Theme.caption
                color: Theme.textTertiary
            }

            highlight: Surface {
                radius: Tokens.radius.l
                color: Theme.fillSelected
            }

            delegate: Item {
                id: row
                required property int index
                required property string title
                required property string subtitle
                required property string icon
                required property string kind
                required property real hue
                required property real chroma

                width: ListView.view.width
                height: 44
                opacity: appear.value
                transform: Translate { y: (1 - appear.value) * 8 }

                Spring { id: appear; target: 0; preset: Motion.snappy }
                Timer {
                    interval: row.index * Motion.staggerMs
                    running: true
                    onTriggered: appear.target = 1
                }

                Item {
                    id: glyph
                    x: 10
                    width: 28
                    height: 28
                    anchors.verticalCenter: parent.verticalCenter
                    AppIcon {
                        anchors.fill: parent
                        visible: row.kind === "app" || row.kind === "file" || row.kind === "calc"
                        size: 28
                        elevation: 1
                        glyph: row.icon
                        hue: row.hue
                        chroma: row.chroma
                    }
                    Surface {
                        anchors.fill: parent
                        visible: !(row.kind === "app" || row.kind === "file" || row.kind === "calc")
                        radius: 8
                        color: Theme.fillControlHover
                        Icon {
                            anchors.centerIn: parent
                            name: row.icon
                            size: 16
                            color: row.chroma > 0 ? Oklch.color(0.7, 0.16, row.hue) : Theme.textPrimary
                        }
                    }
                }

                Column {
                    anchors.left: glyph.right
                    anchors.leftMargin: 12
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: row.title
                        textFormat: Text.PlainText
                        font: row.kind === "calc" ? Theme.title3 : Theme.bodyStrong
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: row.subtitle
                        font: Theme.footnote
                        color: Theme.textTertiary
                        elide: Text.ElideRight
                    }
                }

                HoverHandler {
                    onHoveredChanged: if (hovered) list.currentIndex = row.index
                }
                TapHandler { onTapped: beam.activate(row.index) }
            }
        }

        Divider {
            vertical: true
            x: parent.width * 0.56
            y: searchHeader.height + 12
            height: list.height - 8
        }

        // Preview.
        Item {
            id: preview
            x: parent.width * 0.56 + 1
            y: searchHeader.height + 1
            width: parent.width * 0.44 - 1
            height: list.height + 14
            readonly property var current: list.currentIndex >= 0 && list.currentIndex < results.count
                                           ? results.get(list.currentIndex) : null

            Column {
                anchors.centerIn: parent
                width: parent.width - 48
                spacing: 10
                visible: preview.current !== null

                AppIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    size: 72
                    elevation: 3
                    glyph: preview.current ? preview.current.icon : ""
                    hue: preview.current ? preview.current.hue : 274
                    chroma: preview.current ? Math.max(preview.current.chroma, 0.03) : 0.1
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: preview.current ? preview.current.title : ""
                    textFormat: Text.PlainText
                    font: Theme.title3
                    color: Theme.textPrimary
                    wrapMode: Text.Wrap
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: preview.current ? preview.current.description : ""
                    font: Theme.footnote
                    color: Theme.textSecondary
                    wrapMode: Text.Wrap
                }
            }
        }

        // Footer hints.
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            spacing: 14
            Repeater {
                model: [["↑↓", qsTr("Navigate")], ["↵", qsTr("Open")], ["esc", qsTr("Close")]]
                delegate: Row {
                    id: hint
                    required property var modelData
                    spacing: 6
                    Kbd { text: hint.modelData[0] }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: hint.modelData[1]
                        font: Theme.caption
                        color: Theme.textTertiary
                    }
                }
            }
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            text: qsTr("%n result(s)", "", results.count)
            font: Theme.caption
            color: Theme.textTertiary
        }
    }
}
