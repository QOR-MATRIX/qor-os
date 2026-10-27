// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QOR.Kit
import QOR.Services

// Music: a now-playing hero bound to the media service, with transport,
// seeking, shuffle and repeat, volume, a live visualiser and the queue, and a
// library of songs, albums and artists. QOR Sessions (music the audio engine
// generates live) come first; files from the Music folder follow.
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})
    // "now", "songs", "albums" or "artists".
    property string section: "now"
    // Songs from one collection only: "", "session" or "file".
    property string collection: ""

    onLaunchArgsChanged: if (launchArgs && launchArgs.section) section = launchArgs.section

    readonly property var media: Services.media
    readonly property var track: media.queue[media.queueIndex] ?? ({})
    readonly property color tint: Oklch.color(Theme.darkness > 0.5 ? 0.76 : 0.6, 0.15, media.artworkHue)

    // The library is the queue; every entry remembers its queue index.
    readonly property var songs: media.queue.map((entry, index) => Object.assign({ index: index }, entry))
    readonly property var shownSongs: collection.length === 0 ? songs
                                                               : songs.filter(s => (s.kind ?? "session") === collection)
    readonly property int sessionCount: songs.filter(s => (s.kind ?? "session") === "session").length
    readonly property int fileCount: songs.length - sessionCount
    readonly property var albums: group(s => s.album || qsTr("Singles"), s => s.artist)
    readonly property var artists: group(s => s.artist || qsTr("Unknown Artist"), s => "")

    // [{ name, detail, hue, first, count }] in library order.
    function group(key: var, detail: var): var {
        const groups = [];
        const byName = {};
        for (const s of songs) {
            const name = key(s);
            if (byName[name] === undefined) {
                byName[name] = groups.length;
                groups.push({ name: name, detail: detail(s), hue: s.hue ?? 280, first: s.index, count: 0 });
            }
            groups[byName[name]].count++;
        }
        return groups;
    }

    function formatTime(seconds: real): string {
        if (!(seconds > 0) && seconds !== 0)
            return "--:--";
        const s = Math.max(0, Math.floor(seconds));
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0");
    }

    function totalMinutes(list: var): int {
        let total = 0;
        for (const s of list)
            total += s.duration ?? 0;
        return Math.round(total / 60);
    }

    function play(index: int) {
        media.playIndex(index);
    }

    function musicFolder(): string {
        const url = String(StandardPaths.writableLocation(StandardPaths.MusicLocation));
        const path = decodeURIComponent(url.replace(/^file:\/\//, ""));
        return /^\/[A-Za-z]:/.test(path) ? path.slice(1) : path;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }

    // Ambient wash in the artwork's hue.
    Rectangle {
        anchors.left: sidebar.right
        anchors.right: parent.right
        height: parent.height
        visible: app.section === "now"
        gradient: Gradient {
            GradientStop { position: 0.0; color: Oklch.color(Theme.darkness > 0.5 ? 0.30 : 0.92, 0.07, app.media.artworkHue, 1) }
            GradientStop { position: 0.75; color: Theme.surface }
        }
    }

    FrostSidebar {
        id: sidebar
        width: 200
        height: parent.height
        topInset: app.topInset

        SectionHeader { text: qsTr("Library") }
        SidebarItem {
            objectName: "musicNow"
            width: 180; icon: "play"; text: qsTr("Now Playing")
            selected: app.section === "now"
            onClicked: app.section = "now"
        }
        SidebarItem {
            objectName: "musicSongs"
            width: 180; icon: "list-music"; text: qsTr("Songs"); badge: String(app.songs.length)
            selected: app.section === "songs" && app.collection.length === 0
            onClicked: { app.section = "songs"; app.collection = ""; }
        }
        SidebarItem {
            objectName: "musicAlbums"
            width: 180; icon: "disc-3"; text: qsTr("Albums")
            selected: app.section === "albums"
            onClicked: app.section = "albums"
        }
        SidebarItem {
            objectName: "musicArtists"
            width: 180; icon: "mic-vocal"; text: qsTr("Artists")
            selected: app.section === "artists"
            onClicked: app.section = "artists"
        }
        SectionHeader { text: qsTr("Collections") }
        SidebarItem {
            objectName: "musicSessions"
            width: 180; icon: "sparkles"; text: qsTr("QOR Sessions"); badge: String(app.sessionCount)
            selected: app.section === "songs" && app.collection === "session"
            onClicked: { app.section = "songs"; app.collection = "session"; }
        }
        SidebarItem {
            objectName: "musicFolder"
            width: 180; icon: "folder"; text: qsTr("Music Folder"); badge: String(app.fileCount)
            selected: app.section === "songs" && app.collection === "file"
            onClicked: { app.section = "songs"; app.collection = "file"; }
        }
    }

    // ── Now playing ─────────────────────────────────────────────────────────
    Item {
        id: hero
        anchors.left: sidebar.right
        anchors.right: queue.left
        anchors.top: parent.top
        anchors.topMargin: app.topInset
        anchors.bottom: parent.bottom
        visible: app.section === "now"

        Artwork {
            id: art
            anchors.horizontalCenter: parent.horizontalCenter
            y: 26
            width: Math.max(120, Math.min(280, hero.width - 80, hero.height - 300))
            height: width
            hue: app.media.artworkHue
            elevation: 5
            // The glow beneath brightens with the music.
            shadowColor: Oklch.color(0.45 + 0.22 * Reactive.energy, 0.16 + 0.05 * Reactive.energy,
                                     app.media.artworkHue, 0.85)
            scale: app.media.playing ? 1 : 0.92
            Behavior on scale { SpringAnimation { spring: 3; damping: 0.28 } }
            // Breathes on the beat (on top of the play/pause spring above).
            transform: Scale {
                origin.x: art.width / 2
                origin.y: art.height / 2
                xScale: 1 + 0.035 * Reactive.pulse + 0.015 * Reactive.energy
                yScale: xScale
            }
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: art.bottom
            anchors.topMargin: 24
            width: Math.min(420, hero.width - 60)
            spacing: 4

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: app.media.title
                font: Theme.title2
                color: Theme.textPrimary
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: app.media.album.length > 0 ? app.media.artist + " · " + app.media.album : app.media.artist
                font: Theme.callout
                color: Theme.textSecondary
                elide: Text.ElideRight
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 5
                visible: app.track.kind === "session"
                Icon { anchors.verticalCenter: parent.verticalCenter; name: "sparkles"; size: 12; color: Theme.textTertiary }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("QOR Session · generated live, %1 BPM").arg(app.track.session?.bpm ?? "")
                    font: Theme.caption
                    color: Theme.textTertiary
                }
            }

            Item { width: 1; height: 8 }

            Slider {
                width: parent.width
                from: 0
                to: Math.max(1, app.media.duration)
                value: app.media.position
                onMoved: app.media.seek(value)
            }
            Item {
                width: parent.width
                height: 16
                Text {
                    text: app.media.buffering ? qsTr("Loading…") : app.formatTime(app.media.position)
                    font: Theme.caption
                    color: Theme.textTertiary
                }
                Text {
                    anchors.right: parent.right
                    text: app.media.duration > 0 ? "-" + app.formatTime(app.media.duration - app.media.position) : "--:--"
                    font: Theme.caption
                    color: Theme.textTertiary
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 18
                IconButton {
                    objectName: "shuffleButton"
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "shuffle"
                    iconSize: 16
                    checked: app.media.shuffle
                    Accessible.name: qsTr("Shuffle")
                    onClicked: app.media.shuffle = !app.media.shuffle
                }
                IconButton { anchors.verticalCenter: parent.verticalCenter; icon: "skip-back"; iconSize: 22; width: 40; height: 40; color: Theme.textPrimary; onClicked: app.media.previous() }
                Surface {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 56
                    height: 56
                    radius: 28
                    exponent: 2
                    color: Theme.accent
                    highlight: 1
                    elevation: 2
                    scale: playTap.pressed ? 0.92 : 1
                    Behavior on scale { NumberAnimation { duration: 110 } }
                    Icon {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: app.media.playing ? 0 : 2
                        name: app.media.playing ? "pause" : "play"
                        size: 24
                        color: Theme.onAccent
                    }
                    TapHandler { id: playTap; onTapped: app.media.togglePlayback() }
                }
                IconButton { anchors.verticalCenter: parent.verticalCenter; icon: "skip-forward"; iconSize: 22; width: 40; height: 40; color: Theme.textPrimary; onClicked: app.media.next() }
                // Cycles: repeat all → repeat one → off.
                IconButton {
                    objectName: "repeatButton"
                    anchors.verticalCenter: parent.verticalCenter
                    icon: app.media.repeat === 2 ? "repeat-1" : "repeat"
                    iconSize: 16
                    checked: app.media.repeat !== 0
                    Accessible.name: app.media.repeat === 2 ? qsTr("Repeat one") : app.media.repeat === 1 ? qsTr("Repeat all")
                                                                                                         : qsTr("Repeat off")
                    onClicked: app.media.repeat = (app.media.repeat + 1) % 3
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                Icon { anchors.verticalCenter: parent.verticalCenter; name: "volume"; size: 14; color: Theme.textTertiary }
                Slider {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 200
                    value: Services.audio.volume
                    onMoved: Services.audio.volume = value
                    onPressedChanged: if (!pressed) Sounds.play("volume")
                }
                Icon { anchors.verticalCenter: parent.verticalCenter; name: "volume-2"; size: 14; color: Theme.textTertiary }
            }
        }

        // Live visualiser of what is playing.
        Spectrum {
            objectName: "musicVisualizer"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            width: Math.min(460, hero.width - 60)
            height: 44
            bars: 40
            spacing: 3
            mirrored: true
            minimum: 0.08
            values: Reactive.spectrum
            color: app.tint
            dimColor: app.tint
            opacity: Reactive.playing ? 0.9 : 0.35
            Behavior on opacity { NumberAnimation { duration: 300 } }
        }
    }

    // ── Queue ───────────────────────────────────────────────────────────────
    Item {
        id: queue
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: app.topInset
        anchors.bottom: parent.bottom
        width: app.section === "now" ? 280 : 0
        visible: app.section === "now"

        Divider { vertical: true; height: parent.height }

        Text {
            x: 18
            y: 14
            text: app.media.shuffle ? qsTr("Up Next · Shuffled") : qsTr("Up Next")
            font: Theme.headline
            color: Theme.textPrimary
        }

        ListView {
            x: 8
            y: 44
            width: parent.width - 16
            height: parent.height - 52
            clip: true
            model: app.media.queue
            spacing: 2
            currentIndex: app.media.queueIndex
            highlightFollowsCurrentItem: false
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
            delegate: Item {
                id: entry
                required property var modelData
                required property int index
                readonly property bool current: index === app.media.queueIndex
                width: ListView.view.width
                height: 50
                Surface {
                    anchors.fill: parent
                    radius: Tokens.radius.m
                    color: entry.current ? Theme.fillSelected : entryHover.hovered ? Theme.fillControl : "transparent"
                }
                Artwork {
                    id: thumb
                    x: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: 36
                    hue: entry.modelData.hue ?? 280
                    glyphVisible: false
                }
                Column {
                    anchors.left: thumb.right
                    anchors.leftMargin: 10
                    anchors.right: entryDuration.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: entry.modelData.title
                        font: entry.current ? Theme.bodyStrong : Theme.body
                        color: entry.current ? Theme.accentText : Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: entry.modelData.artist
                        font: Theme.footnote
                        color: Theme.textTertiary
                        elide: Text.ElideRight
                    }
                }
                Text {
                    id: entryDuration
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: entry.modelData.duration > 0 ? app.formatTime(entry.modelData.duration) : ""
                    font: Theme.caption
                    color: Theme.textTertiary
                }
                HoverHandler { id: entryHover }
                TapHandler { onTapped: app.play(entry.index) }
            }
        }
    }

    // ── Library ─────────────────────────────────────────────────────────────
    Loader {
        id: library
        anchors.left: sidebar.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: app.topInset
        anchors.bottom: parent.bottom
        active: app.section !== "now"
        visible: active
        sourceComponent: app.section === "albums" ? albumsView : app.section === "artists" ? artistsView : songsView
    }

    // Title, summary and Play / Shuffle buttons above a library view.
    component LibraryHeader: Item {
        id: header
        property string title
        property string summary
        property var list: []
        width: parent ? parent.width : 400
        height: 92

        Column {
            x: 28
            y: 18
            spacing: 2
            Text { text: header.title; font: Theme.title1; color: Theme.textPrimary }
            Text { text: header.summary; font: Theme.footnote; color: Theme.textTertiary }
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: 24
            y: 28
            spacing: 8
            visible: header.list.length > 0
            Button {
                objectName: "libraryPlay"
                text: qsTr("Play")
                icon.name: "play"
                highlighted: true
                onClicked: {
                    app.media.shuffle = false;
                    app.play(header.list[0].index);
                }
            }
            Button {
                objectName: "libraryShuffle"
                text: qsTr("Shuffle")
                icon.name: "shuffle"
                onClicked: {
                    app.media.shuffle = true;
                    app.play(header.list[Math.floor(Math.random() * header.list.length)].index);
                }
            }
        }
    }

    Component {
        id: songsView
        Item {
            id: songsPage
            readonly property string title: app.collection === "session" ? qsTr("QOR Sessions")
                                            : app.collection === "file" ? qsTr("Music Folder") : qsTr("Songs")

            LibraryHeader {
                id: songsHeader
                title: songsPage.title
                summary: app.collection === "session"
                         ? qsTr("%1 sessions · generated live by the audio engine").arg(app.shownSongs.length)
                         : app.totalMinutes(app.shownSongs) > 0
                           ? qsTr("%1 songs · %2 min").arg(app.shownSongs.length).arg(app.totalMinutes(app.shownSongs))
                           : qsTr("%1 songs").arg(app.shownSongs.length)
                list: app.shownSongs
            }

            // Column headings.
            Item {
                id: headings
                anchors.top: songsHeader.bottom
                x: 20
                width: parent.width - 40
                height: 26
                visible: app.shownSongs.length > 0
                readonly property real titleX: 52
                readonly property real artistX: width * 0.42
                readonly property real albumX: width * 0.66
                Text { x: headings.titleX; text: qsTr("Title"); font: Theme.caption; color: Theme.textTertiary }
                Text { x: headings.artistX; text: qsTr("Artist"); font: Theme.caption; color: Theme.textTertiary }
                Text { x: headings.albumX; text: qsTr("Album"); font: Theme.caption; color: Theme.textTertiary }
                Icon { anchors.right: parent.right; anchors.rightMargin: 14; name: "clock"; size: 13; color: Theme.textTertiary }
                Divider { anchors.bottom: parent.bottom; width: parent.width }
            }

            ListView {
                anchors.top: headings.bottom
                anchors.topMargin: 4
                anchors.bottom: parent.bottom
                x: 20
                width: parent.width - 40
                clip: true
                model: app.shownSongs
                spacing: 1
                ScrollBar.vertical: ScrollBar { }
                delegate: Item {
                    id: song
                    required property var modelData
                    required property int index
                    readonly property bool current: modelData.index === app.media.queueIndex
                    objectName: "song" + modelData.index
                    width: ListView.view.width
                    height: 46

                    Surface {
                        anchors.fill: parent
                        radius: Tokens.radius.m
                        color: song.current ? Theme.fillSelected : songHover.hovered ? Theme.fillControl : "transparent"
                    }
                    // The track number, or the live spectrum while it plays.
                    Text {
                        x: 4
                        width: 26
                        anchors.verticalCenter: parent.verticalCenter
                        horizontalAlignment: Text.AlignRight
                        visible: !(song.current && app.media.playing)
                        text: song.index + 1
                        font: Theme.footnote
                        color: song.current ? Theme.accentText : Theme.textTertiary
                    }
                    Spectrum {
                        x: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 14
                        height: 12
                        visible: song.current && app.media.playing
                        bars: 3
                        spacing: 2
                        minimum: 0.25
                        values: visible ? Reactive.spectrum.slice(2, 30) : []
                        color: Theme.accentText
                    }
                    Artwork {
                        id: songArt
                        x: headings.titleX - 14
                        anchors.verticalCenter: parent.verticalCenter
                        width: 32
                        height: 32
                        hue: song.modelData.hue ?? 280
                        glyphVisible: false
                    }
                    Text {
                        anchors.left: songArt.right
                        anchors.leftMargin: 10
                        width: headings.artistX - x - 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: song.modelData.title
                        font: song.current ? Theme.bodyStrong : Theme.body
                        color: song.current ? Theme.accentText : Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        x: headings.artistX
                        width: headings.albumX - x - 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: song.modelData.artist ?? ""
                        font: Theme.callout
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                    }
                    Text {
                        x: headings.albumX
                        width: parent.width - x - 64
                        anchors.verticalCenter: parent.verticalCenter
                        text: song.modelData.album ?? ""
                        font: Theme.callout
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: song.modelData.duration > 0 ? app.formatTime(song.modelData.duration) : ""
                        font: Theme.footnote
                        color: Theme.textTertiary
                    }
                    HoverHandler { id: songHover }
                    TapHandler { onTapped: app.play(song.modelData.index) }
                }
            }

            // Nothing in the Music folder yet.
            Column {
                anchors.centerIn: parent
                width: Math.min(440, parent.width - 80)
                spacing: 10
                visible: app.shownSongs.length === 0
                Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "folder-open"; size: 40; color: Theme.textTertiary }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("No music files yet")
                    font: Theme.headline
                    color: Theme.textPrimary
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: qsTr("Add MP3, FLAC, Ogg, Opus, WAV or AAC files to %1 (or set QOR_MUSIC_DIR), then restart QOR OS.")
                          .arg(app.musicFolder())
                    font: Theme.callout
                    color: Theme.textSecondary
                }
            }
        }
    }

    Component {
        id: albumsView
        Item {
            LibraryHeader {
                id: albumsHeader
                title: qsTr("Albums")
                summary: qsTr("%1 albums").arg(app.albums.length)
            }
            GridView {
                anchors.top: albumsHeader.bottom
                anchors.bottom: parent.bottom
                x: 20
                width: parent.width - 40
                clip: true
                cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 186)))
                cellHeight: 232
                model: app.albums
                ScrollBar.vertical: ScrollBar { }
                delegate: Item {
                    id: album
                    required property var modelData
                    required property int index
                    objectName: "album" + index
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Artwork {
                        id: albumArt
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 6
                        width: parent.width - 24
                        height: width
                        hue: album.modelData.hue
                        elevation: albumHover.hovered ? 4 : 2
                        scale: albumHover.hovered ? 1.03 : 1
                        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    }
                    Column {
                        anchors.top: albumArt.bottom
                        anchors.topMargin: 8
                        x: 12
                        width: parent.width - 24
                        Text { width: parent.width; text: album.modelData.name; font: Theme.bodyStrong; color: Theme.textPrimary; elide: Text.ElideRight }
                        Text {
                            width: parent.width
                            text: album.modelData.detail + " · " + qsTr("%n song(s)", "", album.modelData.count)
                            font: Theme.footnote
                            color: Theme.textTertiary
                            elide: Text.ElideRight
                        }
                    }
                    HoverHandler { id: albumHover }
                    TapHandler {
                        onTapped: {
                            app.play(album.modelData.first);
                            app.section = "now";
                        }
                    }
                }
            }
        }
    }

    Component {
        id: artistsView
        Item {
            LibraryHeader {
                id: artistsHeader
                title: qsTr("Artists")
                summary: qsTr("%1 artists").arg(app.artists.length)
            }
            GridView {
                anchors.top: artistsHeader.bottom
                anchors.bottom: parent.bottom
                x: 20
                width: parent.width - 40
                clip: true
                cellWidth: Math.floor(width / Math.max(1, Math.floor(width / 168)))
                cellHeight: 200
                model: app.artists
                ScrollBar.vertical: ScrollBar { }
                delegate: Item {
                    id: artist
                    required property var modelData
                    required property int index
                    objectName: "artist" + index
                    width: GridView.view.cellWidth
                    height: GridView.view.cellHeight
                    Artwork {
                        id: portrait
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 8
                        width: Math.min(132, parent.width - 28)
                        height: width
                        radius: width / 2
                        exponent: 2
                        hue: artist.modelData.hue
                        elevation: artistHover.hovered ? 4 : 2
                        scale: artistHover.hovered ? 1.04 : 1
                        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
                    }
                    Column {
                        anchors.top: portrait.bottom
                        anchors.topMargin: 10
                        width: parent.width
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: artist.modelData.name
                            font: Theme.bodyStrong
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("%n song(s)", "", artist.modelData.count)
                            font: Theme.footnote
                            color: Theme.textTertiary
                        }
                    }
                    HoverHandler { id: artistHover }
                    TapHandler {
                        onTapped: {
                            app.play(artist.modelData.first);
                            app.section = "now";
                        }
                    }
                }
            }
        }
    }
}
