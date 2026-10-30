// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QOR.Kit

// Home and the Library (VISION §6, Phase 1): where a controller lands.
//
// Home shows what was played or opened last (Continue), every game, and the
// newest media. The Library shows everything, chosen by what it is or who
// made it. Both come from the Library (library.h).
//
// The d-pad or the arrow keys go from card to card, and on Home a row at a
// time; Up from the top reaches the tabs. South, Return or Space plays a game
// or opens a file. LB and RB, or Left and Right on the tabs, go between Home
// and the Library. East or Esc closes it.
FocusScope {
    id: home

    property Backdrop backdrop: null
    property bool open: false
    // The Rail's height: what is drawn starts below it.
    property real topMargin: 0
    property string page: "home"
    property string filter: "all"

    signal chosen(var item)
    signal dismissed()

    readonly property var filters: [
        { id: "all", label: qsTr("Everything") },
        { id: "games", label: qsTr("Games") },
        { id: "arqade", label: "ARQADE" },
        { id: "mine", label: qsTr("Made by you") },
        { id: "media", label: qsTr("Media") }
    ]
    // What the Library page shows, by the filter chosen.
    readonly property var shown: {
        switch (filter) {
        case "games": return Library.games;
        case "arqade": return Library.games.filter(item => item.source === "arqade");
        case "mine": return Library.games.filter(item => item.source === "mine");
        case "media": return Library.media;
        }
        return Library.games.concat(Library.media);
    }
    readonly property var shelves: [continueShelf, gamesShelf, mediaShelf]
    readonly property var firstShelf: shelves.find(shelf => shelf.visible) ?? null
    // Home opened before the Library had found anything: the first card to
    // come takes the keyboard, unless the person has moved since.
    property bool awaitingCards: false

    function showPage(name: string) {
        page = name;
        focusFirst();
    }

    // The first card there is, on the page showing.
    function focusFirst() {
        awaitingCards = false;
        if (page === "library") {
            grid.forceActiveFocus();
            return;
        }
        if (firstShelf) {
            firstShelf.forceActiveFocus();
        } else {
            tabBar.forceActiveFocus();
            awaitingCards = open;
        }
    }
    onFirstShelfChanged: {
        if (awaitingCards && firstShelf && page === "home" && tabBar.activeFocus)
            focusFirst();
    }

    // From a row, the next one shown that way, at the same place along it;
    // above the first, the tabs.
    function leaveShelf(from: HomeShelf, direction: int) {
        const shown = shelves.filter(shelf => shelf.visible);
        const at = shown.indexOf(from) + direction;
        if (at < 0) {
            tabBar.forceActiveFocus();
            return;
        }
        if (at >= shown.length)
            return;
        shown[at].currentIndex = Math.min(from.currentIndex, shown[at].items.length - 1);
        shown[at].forceActiveFocus();
        homePage.reveal(shown[at]);
    }

    visible: opacity > 0
    opacity: open ? 1 : 0
    Behavior on opacity {
        NumberAnimation { duration: Theme.reduceMotion ? 0 : 180; easing.type: Easing.OutCubic }
    }
    onOpenChanged: {
        if (!open)
            return;
        Library.refresh();
        Qt.callLater(focusFirst);
    }

    Keys.onEscapePressed: home.dismissed()

    Glass {
        anchors.fill: parent
        backdrop: home.backdrop
        radius: 0
        elevation: 0
    }
    // Clicks stop here, short of the desktop.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: wheel => wheel.accepted = false
    }

    // ── Home · Library ─────────────────────────────────────────────────────
    FocusScope {
        id: tabBar
        objectName: "homeTabs"
        x: 64
        y: home.topMargin + 28
        width: tabs.width
        height: tabs.height

        Keys.onLeftPressed: {
            home.awaitingCards = false;
            home.page = "home";
        }
        Keys.onRightPressed: {
            home.awaitingCards = false;
            home.page = "library";
        }
        Keys.onDownPressed: home.focusFirst()
        // The top: taken, so the controller interface does not go on as Tab.
        Keys.onUpPressed: event => event.accepted = true
        Keys.onReturnPressed: home.focusFirst()
        Keys.onSpacePressed: home.focusFirst()

        Row {
            id: tabs
            spacing: 32
            Repeater {
                model: [{ id: "home", label: qsTr("Home") }, { id: "library", label: qsTr("Library") }]
                delegate: Column {
                    id: tab
                    required property var modelData
                    readonly property bool chosen: home.page === tab.modelData.id
                    spacing: 6
                    Text {
                        text: tab.modelData.label
                        color: tab.chosen ? Theme.textPrimary : Theme.textTertiary
                        font: Theme.title1
                    }
                    Rectangle {
                        width: parent.width
                        height: 3
                        radius: 1.5
                        color: tab.chosen ? (tabBar.activeFocus ? Theme.focusRing : Theme.accent) : "transparent"
                    }
                    TapHandler {
                        onTapped: home.showPage(tab.modelData.id)
                    }
                }
            }
        }
    }

    // ── Home ──────────────────────────────────────────────────────────────
    Flickable {
        id: homePage
        objectName: "homePage"
        visible: home.page === "home"
        anchors.fill: parent
        anchors.topMargin: tabBar.y + tabBar.height + 28
        contentHeight: shelfColumn.height + 64
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        // Scrolls so a row is in view.
        function reveal(shelf: Item) {
            const top = shelf.y;
            const bottom = shelf.y + shelf.height + 24;
            if (top < contentY)
                contentY = top;
            else if (bottom > contentY + height)
                contentY = Math.min(contentHeight - height, bottom - height);
        }

        Column {
            id: shelfColumn
            x: 64
            width: home.width - 128
            spacing: 36

            HomeShelf {
                id: continueShelf
                objectName: "continueShelf"
                width: parent.width
                title: qsTr("Continue")
                items: Library.recent
                onChosen: item => home.chosen(item)
                onLeave: direction => home.leaveShelf(continueShelf, direction)
            }
            HomeShelf {
                id: gamesShelf
                objectName: "gamesShelf"
                width: parent.width
                title: qsTr("Games")
                items: Library.games
                onChosen: item => home.chosen(item)
                onLeave: direction => home.leaveShelf(gamesShelf, direction)
            }
            HomeShelf {
                id: mediaShelf
                objectName: "mediaShelf"
                width: parent.width
                title: qsTr("Media")
                items: Library.media.slice(0, 16)
                onChosen: item => home.chosen(item)
                onLeave: direction => home.leaveShelf(mediaShelf, direction)
            }
            Text {
                visible: !home.shelves.some(shelf => shelf.visible)
                width: parent.width
                text: Library.scanning ? qsTr("Looking for games and media…")
                                       : qsTr("Nothing here yet. The games QOR OS carries, ARQADE's, your own QQ games and your videos, music and pictures appear here.")
                color: Theme.textSecondary
                font: Theme.body
                wrapMode: Text.WordWrap
            }
        }
    }

    // ── Library ───────────────────────────────────────────────────────────
    Item {
        id: libraryPage
        visible: home.page === "library"
        anchors.fill: parent
        anchors.topMargin: tabBar.y + tabBar.height + 28

        FocusScope {
            id: filterBar
            objectName: "libraryFilters"
            x: 64
            width: filterRow.width
            height: filterRow.height
            property int at: Math.max(0, home.filters.findIndex(f => f.id === home.filter))

            Keys.onLeftPressed: home.filter = home.filters[Math.max(0, at - 1)].id
            Keys.onRightPressed: home.filter = home.filters[Math.min(home.filters.length - 1, at + 1)].id
            Keys.onUpPressed: tabBar.forceActiveFocus()
            Keys.onDownPressed: grid.forceActiveFocus()
            Keys.onReturnPressed: grid.forceActiveFocus()
            Keys.onSpacePressed: grid.forceActiveFocus()

            Row {
                id: filterRow
                spacing: 10
                Repeater {
                    model: home.filters
                    delegate: Rectangle {
                        id: chip
                        required property var modelData
                        required property int index
                        readonly property bool chosen: home.filter === chip.modelData.id
                        width: chipLabel.implicitWidth + 32
                        height: 36
                        radius: height / 2
                        color: chip.chosen ? Theme.accent : Theme.fillControl
                        border.width: filterBar.activeFocus && chip.chosen ? 3 : 0
                        border.color: Theme.focusRing
                        Text {
                            id: chipLabel
                            anchors.centerIn: parent
                            text: chip.modelData.label
                            color: chip.chosen ? Theme.onAccent : Theme.textPrimary
                            font: Theme.callout
                        }
                        TapHandler {
                            onTapped: home.filter = chip.modelData.id
                        }
                    }
                }
            }
        }

        GridView {
            id: grid
            objectName: "libraryGrid"
            // Room inside the clip for the card the keyboard is on, which
            // grows a little and is ringed.
            x: 64 - leftMargin
            anchors.top: filterBar.bottom
            anchors.topMargin: 28 - topMargin
            anchors.bottom: parent.bottom
            width: home.width - 128 + leftMargin + rightMargin
            leftMargin: 16
            rightMargin: 16
            topMargin: 16
            cellWidth: 256
            cellHeight: 240
            clip: true
            model: home.shown
            keyNavigationEnabled: false
            boundsBehavior: Flickable.StopAtBounds
            readonly property int columns: Math.max(1, Math.floor((width - leftMargin - rightMargin) / cellWidth))
            onCountChanged: if (currentIndex >= count) currentIndex = Math.max(0, count - 1)

            function choose() {
                if (currentIndex >= 0 && currentIndex < home.shown.length)
                    home.chosen(home.shown[currentIndex]);
            }

            delegate: Item {
                id: cell
                required property var modelData
                required property int index
                width: grid.cellWidth
                height: grid.cellHeight
                HomeCard {
                    item: cell.modelData
                    current: grid.activeFocus && cell.index === grid.currentIndex
                    onChosen: {
                        grid.currentIndex = cell.index;
                        grid.choose();
                    }
                }
            }

            Keys.onLeftPressed: currentIndex = Math.max(0, currentIndex - 1)
            Keys.onRightPressed: currentIndex = Math.min(count - 1, currentIndex + 1)
            Keys.onUpPressed: {
                if (currentIndex < columns)
                    filterBar.forceActiveFocus();
                else
                    currentIndex -= columns;
            }
            Keys.onDownPressed: currentIndex = Math.min(count - 1, currentIndex + columns)
            Keys.onReturnPressed: choose()
            Keys.onEnterPressed: choose()
            Keys.onSpacePressed: choose()
        }
        Text {
            visible: grid.count === 0
            x: 64
            anchors.top: filterBar.bottom
            anchors.topMargin: 28
            text: Library.scanning ? qsTr("Looking…") : qsTr("Nothing here.")
            color: Theme.textSecondary
            font: Theme.body
        }
    }
}
