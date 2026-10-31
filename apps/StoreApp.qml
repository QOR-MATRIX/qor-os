// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Services
import QOR.Store

// Store: themes and widgets for QOR OS today; apps, games, tools and music
// with the developer program. Every item is signed by its developer and
// verified against the QOR OS Store's signed catalogue before it installs
// (docs/STORE.md).
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})
    // discover | themes | widgets | apps | games | tools | music | library
    property string section: "discover"
    property string selectedId: ""
    property string query: ""
    // The last failed install, shown as a banner.
    property string notice: ""

    readonly property var themeItems: Store.items.filter(item => item.kind === "theme")
    readonly property var widgetItems: Store.items.filter(item => item.kind === "widget")
    readonly property var featuredTheme: themeItems.find(item => item.featured) ?? themeItems[0] ?? null
    readonly property var selected: selectedId.length > 0 && Store.items.length >= 0 ? Store.item(selectedId) : null

    function open(id: string) {
        selectedId = id;
        scroller.contentY = 0;
    }

    function show(name: string) {
        section = name;
        selectedId = "";
        query = "";
        search.text = "";
        scroller.contentY = 0;
    }

    onLaunchArgsChanged: {
        if (!launchArgs)
            return;
        if (launchArgs.page)
            show(launchArgs.page);
        if (launchArgs.item)
            open(launchArgs.item);
    }

    Connections {
        target: Store
        function onFinished(id: string, ok: bool, message: string) {
            if (ok)
                return;
            const item = Store.item(id);
            app.notice = qsTr("%1 could not be installed. %2").arg(item.name ?? id).arg(message);
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    FrostSidebar {
        id: sidebar
        width: 220
        height: parent.height
        topInset: app.topInset

        SearchField {
            id: search
            objectName: "storeSearch"
            width: 200
            placeholderText: qsTr("Search the Store")
            onTextChanged: {
                app.query = text;
                if (text.length > 0)
                    app.selectedId = "";
            }
        }
        Item { width: 1; height: 8 }
        SidebarItem {
            objectName: "storeDiscover"
            width: 200; icon: "sparkles"; text: qsTr("Discover")
            selected: app.section === "discover" && app.query.length === 0
            onClicked: app.show("discover")
        }
        SidebarItem {
            objectName: "storeThemes"
            width: 200; icon: "palette"; text: qsTr("Themes"); badge: String(app.themeItems.length)
            selected: app.section === "themes" && app.query.length === 0
            onClicked: app.show("themes")
        }
        SidebarItem {
            objectName: "storeWidgets"
            width: 200; icon: "layout-grid"; text: qsTr("Widgets"); badge: String(app.widgetItems.length)
            selected: app.section === "widgets" && app.query.length === 0
            onClicked: app.show("widgets")
        }
        SidebarItem {
            objectName: "storeApps"
            width: 200; icon: "app-window"; text: qsTr("Apps")
            selected: app.section === "apps" && app.query.length === 0
            onClicked: app.show("apps")
        }
        SidebarItem {
            objectName: "storeGames"
            width: 200; icon: "gamepad-2"; text: qsTr("Games")
            selected: app.section === "games" && app.query.length === 0
            onClicked: app.show("games")
        }
        SidebarItem {
            objectName: "storeTools"
            width: 200; icon: "wrench"; text: qsTr("Tools")
            selected: app.section === "tools" && app.query.length === 0
            onClicked: app.show("tools")
        }
        SidebarItem {
            objectName: "storeMusic"
            width: 200; icon: "music"; text: qsTr("Music")
            selected: app.section === "music" && app.query.length === 0
            onClicked: app.show("music")
        }
        SectionHeader { text: qsTr("You") }
        SidebarItem {
            objectName: "storeLibrary"
            width: 200; icon: "download"; text: qsTr("Library")
            badge: Store.updateCount > 0 ? qsTr("%n update(s)", "", Store.updateCount) : String(Store.library.length)
            selected: app.section === "library" && app.query.length === 0
            onClicked: app.show("library")
        }
    }

    Flickable {
        id: scroller
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: app.topInset
        contentHeight: page.height + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        Column {
            id: page
            x: 32
            y: 12
            width: scroller.width - 64
            spacing: 22

            // A failed install.
            Surface {
                width: parent.width
                height: noticeText.implicitHeight + 24
                visible: app.notice.length > 0
                radius: Tokens.radius.l
                color: Oklch.alpha(Theme.critical, 0.12)
                strokeColor: Oklch.alpha(Theme.critical, 0.4)
                Text {
                    id: noticeText
                    objectName: "storeNotice"
                    x: 16
                    width: parent.width - 60
                    anchors.verticalCenter: parent.verticalCenter
                    text: app.notice
                    color: Theme.textPrimary
                    font: Theme.body
                    wrapMode: Text.Wrap
                }
                IconButton {
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "x"
                    onClicked: app.notice = ""
                }
            }

            // The catalogue could not be loaded.
            Card {
                width: parent.width
                visible: !Store.ready && !Store.loading
                Text {
                    width: parent.width
                    text: qsTr("The Store is unavailable")
                    color: Theme.textPrimary
                    font: Theme.headline
                }
                Text {
                    width: parent.width
                    text: Store.error
                    color: Theme.textSecondary
                    font: Theme.body
                    wrapMode: Text.Wrap
                }
                Button {
                    text: qsTr("Try Again")
                    onClicked: Store.refresh()
                }
            }

            Loader {
                width: parent.width
                sourceComponent: app.selected ? itemPage
                               : app.query.length > 0 ? searchPage
                               : app.section === "discover" ? discoverPage
                               : app.section === "themes" ? themesPage
                               : app.section === "widgets" ? widgetsPage
                               : app.section === "apps" ? appsPage
                               : app.section === "library" ? libraryPage
                               : comingPage
            }
        }
    }

    // ── Pages ───────────────────────────────────────────────────────────────

    Component {
        id: discoverPage
        Column {
            spacing: 26

            PageTitle {
                text: qsTr("Discover")
                subtitle: qsTr("Themes and widgets for QOR OS, signed by their developers and verified by the Store.")
            }

            // The featured theme.
            Surface {
                id: hero
                objectName: "storeHero"
                width: parent.width
                height: 260
                visible: app.featuredTheme !== null
                radius: Tokens.radius.xl
                color: Theme.surfaceRaised
                strokeColor: Theme.windowStroke
                elevation: 2

                ThemePreview {
                    x: 16
                    y: 16
                    width: (parent.height - 32) * 1.6
                    height: parent.height - 32
                    theme: app.featuredTheme ? app.featuredTheme.preview : ({})
                    radius: Tokens.radius.l
                }
                Column {
                    x: (parent.height - 32) * 1.6 + 44
                    width: parent.width - x - 24
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    Text {
                        text: qsTr("FEATURED THEME")
                        color: Theme.accentText
                        font: Theme.caption
                    }
                    Text {
                        width: parent.width
                        text: app.featuredTheme ? app.featuredTheme.name : ""
                        textFormat: Text.PlainText
                        color: Theme.textPrimary
                        font: Theme.display
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: app.featuredTheme ? app.featuredTheme.summary : ""
                        color: Theme.textSecondary
                        font: Theme.callout
                        wrapMode: Text.Wrap
                    }
                    Item { width: 1; height: 6 }
                    Row {
                        spacing: 10
                        ActionButton { item: app.featuredTheme ?? ({}) }
                        Button {
                            text: qsTr("Details")
                            flat: true
                            onClicked: app.open(app.featuredTheme.id)
                        }
                    }
                }
            }

            Shelf {
                title: qsTr("Themes")
                items: app.themeItems
                onSeeAll: app.show("themes")
            }
            Shelf {
                title: qsTr("Widgets for your desktop")
                items: app.widgetItems
                onSeeAll: app.show("widgets")
            }

            // What comes next, plainly.
            Surface {
                width: parent.width
                height: program.implicitHeight + 40
                radius: Tokens.radius.xl
                color: Oklch.alpha(Theme.accent, 0.08)
                strokeColor: Oklch.alpha(Theme.accent, 0.3)
                Row {
                    id: program
                    x: 20
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 16
                    AppIcon {
                        glyph: "code-xml"
                        hue: Theme.hue
                        size: 52
                    }
                    Column {
                        width: page.width - 140
                        spacing: 4
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            text: qsTr("Build for QOR OS")
                            color: Theme.textPrimary
                            font: Theme.headline
                        }
                        Text {
                            width: parent.width
                            text: qsTr("The developer program opens with the QOR OS SDK: themes, widgets, apps and games from developers everywhere, each signed and reviewed before it reaches the Store.")
                            color: Theme.textSecondary
                            font: Theme.body
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }

    Component {
        id: themesPage
        Column {
            spacing: 20
            PageTitle {
                text: qsTr("Themes")
                subtitle: qsTr("A theme sets the accent, the tint of every surface and the character of the wallpaper. You can still change the accent and appearance afterwards.")
            }
            ItemGrid { items: app.themeItems }
        }
    }

    Component {
        id: widgetsPage
        Column {
            spacing: 20
            PageTitle {
                text: qsTr("Widgets")
                subtitle: qsTr("Glanceable information on your desktop. Add them from here, or right-click the desktop and choose Edit Widgets.")
            }
            ItemGrid { items: app.widgetItems }
        }
    }

    Component {
        id: searchPage
        Column {
            id: searchColumn
            readonly property var results: Store.items.length >= 0 ? Store.search(app.query) : []
            spacing: 20
            PageTitle {
                text: qsTr("Results for “%1”").arg(app.query)
                subtitle: searchColumn.results.length === 0
                          ? qsTr("Nothing matches. Try a colour, a mood or a kind of widget.")
                          : qsTr("%n item(s)", "", searchColumn.results.length)
            }
            ItemGrid { items: searchColumn.results }
        }
    }

    Component {
        id: appsPage
        Column {
            spacing: 20
            PageTitle {
                text: qsTr("Apps")
                subtitle: qsTr("Apps from other developers arrive with the developer program, each running in its own sandbox. These come with QOR OS.")
            }
            Card {
                width: parent.width
                padding: 0
                spacing: 0
                Repeater {
                    model: AppRegistry.apps.filter(a => a.id !== "store")
                    delegate: Item {
                        id: row
                        required property var modelData
                        required property int index
                        width: parent.width
                        height: 68
                        AppIcon {
                            x: 16
                            anchors.verticalCenter: parent.verticalCenter
                            glyph: row.modelData.glyph
                            hue: row.modelData.hue
                            chroma: row.modelData.chroma
                            size: 40
                        }
                        Column {
                            x: 70
                            width: parent.width - 180
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                text: row.modelData.name
                                textFormat: Text.PlainText
                                color: Theme.textPrimary
                                font: Theme.bodyStrong
                            }
                            Text {
                                width: parent.width
                                text: row.modelData.description
                                color: Theme.textSecondary
                                font: Theme.footnote
                                elide: Text.ElideRight
                            }
                        }
                        Button {
                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Open")
                            onClicked: Intents.openApp(row.modelData.id)
                        }
                        Divider {
                            visible: row.index > 0
                            x: 70
                            width: parent.width - 70
                        }
                    }
                }
            }
        }
    }

    Component {
        id: comingPage
        Column {
            id: coming
            readonly property var what: ({
                games: { icon: "gamepad-2", title: qsTr("Games are on their way"),
                         text: qsTr("Games arrive with the developer program and the app sandbox, controller support included.") },
                tools: { icon: "wrench", title: qsTr("Tools are on their way"),
                         text: qsTr("Utilities and developer tools arrive with the developer program and the app sandbox.") },
                music: { icon: "music", title: qsTr("Music is on its way"),
                         text: qsTr("Sound packs and music from the artists who own them arrive with the developer program.") }
            })[app.section] ?? { icon: "sparkles", title: "", text: "" }
            spacing: 14
            topPadding: 80
            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: coming.what.icon
                size: 44
                color: Theme.textTertiary
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: coming.what.title
                color: Theme.textPrimary
                font: Theme.title2
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(460, page.width)
                horizontalAlignment: Text.AlignHCenter
                text: coming.what.text
                color: Theme.textSecondary
                font: Theme.body
                wrapMode: Text.Wrap
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Browse Themes")
                onClicked: app.show("themes")
            }
        }
    }

    Component {
        id: libraryPage
        Column {
            spacing: 20
            Item {
                width: parent.width
                height: libraryTitle.height
                PageTitle {
                    id: libraryTitle
                    width: parent.width - 200
                    text: qsTr("Library")
                    subtitle: Store.library.length === 0 ? qsTr("What you get from the Store appears here.")
                              : Store.updateCount > 0 ? qsTr("%n update(s) available", "", Store.updateCount)
                              : qsTr("Everything is up to date.")
                }
                Button {
                    objectName: "storeRefresh"
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    text: Store.loading ? qsTr("Checking…") : qsTr("Check for Updates")
                    enabled: !Store.loading
                    onClicked: Store.refresh()
                }
            }
            Card {
                width: parent.width
                visible: Store.library.length > 0
                padding: 0
                spacing: 0
                Repeater {
                    model: Store.library
                    delegate: Item {
                        id: entry
                        required property var modelData
                        required property int index
                        objectName: "libraryItem:" + modelData.id
                        width: parent.width
                        height: 72
                        Component {
                            id: smallTheme
                            ThemePreview {
                                theme: entry.modelData.preview ?? ({})
                                radius: 8
                            }
                        }
                        Component {
                            id: smallWidget
                            WidgetTile {
                                item: entry.modelData
                                compact: true
                            }
                        }
                        Loader {
                            x: 14
                            width: 72
                            height: 46
                            anchors.verticalCenter: parent.verticalCenter
                            sourceComponent: entry.modelData.kind === "theme" ? smallTheme : smallWidget
                        }
                        Column {
                            x: 102
                            width: parent.width - 102 - actions.width - 24
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                width: parent.width
                                text: entry.modelData.name
                                textFormat: Text.PlainText
                                color: Theme.textPrimary
                                font: Theme.bodyStrong
                                elide: Text.ElideRight
                            }
                            Text {
                                width: parent.width
                                text: (entry.modelData.kind === "theme" ? qsTr("Theme") : qsTr("Widget")) + " · "
                                      + (entry.modelData.updateAvailable
                                         ? qsTr("%1 → %2").arg(entry.modelData.installedVersion).arg(entry.modelData.version)
                                         : entry.modelData.installedVersion)
                                color: Theme.textSecondary
                                font: Theme.footnote
                            }
                        }
                        Row {
                            id: actions
                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8
                            ActionButton {
                                objectName: "libraryAction:" + entry.modelData.id
                                item: entry.modelData
                            }
                            Button {
                                objectName: "libraryRemove:" + entry.modelData.id
                                text: qsTr("Remove")
                                flat: true
                                enabled: !entry.modelData.busy
                                onClicked: Store.remove(entry.modelData.id)
                            }
                        }
                        Divider {
                            visible: entry.index > 0
                            x: 102
                            width: parent.width - 102
                        }
                    }
                }
            }
        }
    }

    Component {
        id: itemPage
        Column {
            id: details
            readonly property bool isTheme: app.selected.kind === "theme"
            readonly property real artWidth: Math.min(420, width * 0.5)
            spacing: 24

            Component {
                id: bigTheme
                ThemePreview {
                    theme: app.selected.preview ?? ({})
                    radius: Tokens.radius.xl
                }
            }
            Component {
                id: bigWidget
                Item {
                    id: widgetArt
                    // Installed widgets show themselves; others their tile.
                    readonly property var live: Store.widgets.length >= 0 ? Store.widget(app.selectedId) : ({})
                    readonly property bool isLive: live.source !== undefined
                    WidgetTile {
                        anchors.fill: parent
                        visible: !widgetArt.isLive
                        item: app.selected
                    }
                    Glass {
                        visible: widgetArt.isLive
                        anchors.centerIn: parent
                        width: widgetArt.live.size === "small" ? 152 : 320
                        height: widgetArt.live.size === "large" ? 320 : 152
                        scale: Math.min(1, widgetArt.height / height)
                        backdrop: Materials.desktopFor(Window.window)
                        radius: Tokens.radius.xl
                        Loader {
                            anchors.fill: parent
                            source: widgetArt.isLive ? widgetArt.live.source : ""
                        }
                    }
                }
            }

            Button {
                objectName: "storeBack"
                flat: true
                text: "‹  " + (app.query.length > 0 ? qsTr("Results")
                               : app.section === "discover" ? qsTr("Discover")
                               : app.section === "library" ? qsTr("Library")
                               : details.isTheme ? qsTr("Themes") : qsTr("Widgets"))
                onClicked: app.selectedId = ""
            }

            Row {
                width: parent.width
                spacing: 28

                Loader {
                    width: details.artWidth
                    height: width * 0.62
                    sourceComponent: details.isTheme ? bigTheme : bigWidget
                }

                Column {
                    width: parent.width - details.artWidth - 28
                    spacing: 8
                    Text {
                        objectName: "storeItemName"
                        width: parent.width
                        text: app.selected.name
                        textFormat: Text.PlainText
                        color: Theme.textPrimary
                        font: Theme.title1
                        wrapMode: Text.Wrap
                    }
                    Row {
                        spacing: 6
                        Text {
                            text: app.selected.developerName
                            textFormat: Text.PlainText
                            color: Theme.accentText
                            font: Theme.body
                        }
                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: app.selected.verified === true
                            name: "badge-check"
                            size: 14
                            color: Theme.accentText
                        }
                    }
                    Text {
                        width: parent.width
                        text: app.selected.summary
                        color: Theme.textSecondary
                        font: Theme.callout
                        wrapMode: Text.Wrap
                    }
                    Item { width: 1; height: 10 }
                    Row {
                        spacing: 10
                        ActionButton {
                            objectName: "storeAction"
                            item: app.selected
                        }
                        Button {
                            objectName: "storeRemove"
                            visible: app.selected.installed === true
                            text: qsTr("Remove")
                            flat: true
                            enabled: !app.selected.busy
                            onClicked: Store.remove(app.selectedId)
                        }
                    }
                    Text {
                        visible: app.selected.price === 0
                        text: qsTr("Free")
                        color: Theme.textTertiary
                        font: Theme.footnote
                    }
                }
            }

            SectionTitle { text: qsTr("About") }
            Text {
                width: Math.min(parent.width, 680)
                text: app.selected.description ?? ""
                color: Theme.textPrimary
                font: Theme.body
                wrapMode: Text.Wrap
                lineHeight: 1.25
            }

            SectionTitle { text: qsTr("What's New") }
            Text {
                width: Math.min(parent.width, 680)
                text: qsTr("Version %1").arg(app.selected.version) + "  ·  " + (app.selected.whatsNew ?? "")
                color: Theme.textSecondary
                font: Theme.body
                wrapMode: Text.Wrap
            }

            SectionTitle { text: qsTr("Information") }
            Grid {
                columns: 2
                columnSpacing: 40
                rowSpacing: 10
                Repeater {
                    model: [].concat(
                        [qsTr("Developer"), app.selected.developerName],
                        [qsTr("Kind"), app.selected.kind === "theme" ? qsTr("Theme")
                            : qsTr("Widget · %1").arg(app.selected.preview && app.selected.preview.size === "small" ? qsTr("small")
                                                     : app.selected.preview && app.selected.preview.size === "large" ? qsTr("large") : qsTr("medium"))],
                        [qsTr("Version"), app.selected.version],
                        [qsTr("Size"), qsTr("%1 KB").arg(Math.max(1, Math.round((app.selected.size ?? 0) / 1024)))],
                        [qsTr("Released"), app.selected.released ?? ""],
                        [qsTr("Price"), app.selected.price === 0 ? qsTr("Free") : "$" + app.selected.price])
                    delegate: Text {
                        required property var modelData
                        required property int index
                        text: modelData ?? ""
                        textFormat: Text.PlainText
                        color: index % 2 === 0 ? Theme.textTertiary : Theme.textPrimary
                        font: Theme.body
                    }
                }
            }

            Row {
                spacing: 10
                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "shield-check"
                    size: 18
                    color: Theme.positive
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: Math.min(page.width - 40, 640)
                    text: qsTr("Signed by %1 and verified against the QOR OS Store's signed catalogue before it installs.")
                          .arg(app.selected.developerName)
                    color: Theme.textSecondary
                    font: Theme.footnote
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    // ── Pieces ──────────────────────────────────────────────────────────────

    component PageTitle: Column {
        property alias text: heading.text
        property alias subtitle: sub.text
        width: parent ? parent.width : 400
        spacing: 4
        Text {
            id: heading
            color: Theme.textPrimary
            font: Theme.title1
        }
        Text {
            id: sub
            width: parent.width
            visible: text.length > 0
            color: Theme.textSecondary
            font: Theme.body
            wrapMode: Text.Wrap
        }
    }

    component SectionTitle: Text {
        color: Theme.textPrimary
        font: Theme.title3
    }

    // A horizontal row of items with "See All".
    component Shelf: Column {
        id: shelf
        property string title
        property var items: []
        signal seeAll()
        width: parent ? parent.width : 400
        spacing: 12
        Item {
            width: parent.width
            height: shelfTitle.height
            SectionTitle {
                id: shelfTitle
                text: shelf.title
            }
            Button {
                anchors.right: parent.right
                anchors.verticalCenter: shelfTitle.verticalCenter
                flat: true
                text: qsTr("See All")
                onClicked: shelf.seeAll()
            }
        }
        ListView {
            width: parent.width
            height: 238
            orientation: ListView.Horizontal
            spacing: 18
            clip: true
            model: shelf.items
            delegate: ItemCard {
                required property var modelData
                item: modelData
            }
        }
    }

    component ItemGrid: Flow {
        id: grid
        property var items: []
        width: parent ? parent.width : 400
        spacing: 18
        Repeater {
            model: grid.items
            delegate: ItemCard {
                required property var modelData
                item: modelData
            }
        }
    }

    component ItemCard: Item {
        id: card
        property var item: ({})
        objectName: "storeItem:" + (item.id ?? "")
        width: 240
        height: 238

        Component {
            id: cardTheme
            ThemePreview { theme: card.item.preview ?? ({}) }
        }
        Component {
            id: cardWidget
            WidgetTile { item: card.item }
        }
        Loader {
            id: art
            width: parent.width
            height: 150
            sourceComponent: card.item.kind === "theme" ? cardTheme : cardWidget
            scale: hover.hovered ? 1.02 : 1
            Behavior on scale {
                enabled: !Theme.reduceMotion
                NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
            }
        }
        Column {
            y: art.height + 10
            width: parent.width - actionButton.width - 8
            Text {
                width: parent.width
                text: card.item.name ?? ""
                textFormat: Text.PlainText
                color: Theme.textPrimary
                font: Theme.bodyStrong
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                text: card.item.summary ?? ""
                color: Theme.textSecondary
                font: Theme.footnote
                elide: Text.ElideRight
            }
            Text {
                text: card.item.developerName ?? ""
                textFormat: Text.PlainText
                color: Theme.textTertiary
                font: Theme.caption
            }
        }
        ActionButton {
            id: actionButton
            objectName: "cardAction:" + (card.item.id ?? "")
            anchors.right: parent.right
            y: art.height + 12
            item: card.item
            compact: true
        }
        HoverHandler { id: hover }
        TapHandler {
            // Opening the page; the button handles its own taps.
            onTapped: eventPoint => {
                if (!actionButton.contains(actionButton.mapFromItem(card, eventPoint.position)))
                    app.open(card.item.id);
            }
        }
    }

    // A widget's tile until it is installed: its glyph on its hue.
    component WidgetTile: Surface {
        id: tile
        property var item: ({})
        property bool compact: false
        readonly property real hue: item.preview ? item.preview.hue : 274
        radius: compact ? 8 : Tokens.radius.l
        color: Oklch.color(Theme.darkness > 0.5 ? 0.34 : 0.9, 0.06, hue)
        gradientTo: Oklch.color(Theme.darkness > 0.5 ? 0.24 : 0.82, 0.08, hue + 30)
        strokeColor: Theme.windowStroke
        Icon {
            anchors.centerIn: parent
            name: tile.item.preview ? tile.item.preview.glyph : "layout-grid"
            size: tile.compact ? 18 : 46
            color: Oklch.color(Theme.darkness > 0.5 ? 0.86 : 0.42, 0.12, tile.hue)
        }
        Text {
            visible: !tile.compact
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 10
            text: !tile.item.preview ? "" : tile.item.preview.size === "small" ? qsTr("Small · 1×1")
                  : tile.item.preview.size === "large" ? qsTr("Large · 2×2") : qsTr("Medium · 2×1")
            color: Theme.textSecondary
            font: Theme.caption
        }
    }

    // Get, Installing…, Update, Apply / In Use (themes) or Add to Desktop
    // (widgets).
    component ActionButton: Button {
        id: action
        property var item: ({})
        property bool compact: false
        readonly property bool activeTheme: item.kind === "theme" && item.installed === true && Themes.activeId === item.id
        text: item.busy ? qsTr("Installing…")
              : !item.installed ? qsTr("Get")
              : item.updateAvailable ? qsTr("Update")
              : item.kind === "theme" ? (activeTheme ? qsTr("In Use") : qsTr("Apply"))
              : compact ? qsTr("Add") : qsTr("Add to Desktop")
        highlighted: !item.installed || item.updateAvailable === true
        enabled: item.id !== undefined && !item.busy && !activeTheme
        onClicked: {
            if (!item.installed || item.updateAvailable)
                Store.install(item.id);
            else if (item.kind === "theme")
                Themes.apply(item.id);
            else
                Intents.request("widgets.add", { id: item.id });
        }
    }
}
