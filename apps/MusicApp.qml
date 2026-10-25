// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QTOS.Kit
import QTOS.Services

// Music: a now-playing hero bound to the media service, with transport,
// seeking, volume, a live visualiser and the queue. QTOS Sessions (music the
// audio engine generates live) come first; files from the Music folder follow.
Item {
    id: app

    property real topInset: 40
    property var launchArgs: ({})
    property string section: "now"

    readonly property var media: Services.media
    readonly property var track: media.queue[media.queueIndex] ?? ({})
    readonly property color tint: Oklch.color(Theme.darkness > 0.5 ? 0.76 : 0.6, 0.15, media.artworkHue)

    function formatTime(seconds: real): string {
        if (!(seconds > 0) && seconds !== 0)
            return "--:--";
        const s = Math.max(0, Math.floor(seconds));
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0");
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
        gradient: Gradient {
            GradientStop { position: 0.0; color: Oklch.color(Theme.darkness > 0.5 ? 0.30 : 0.92, 0.07, app.media.artworkHue, 1) }
            GradientStop { position: 0.75; color: Theme.surface }
        }
        Behavior on opacity { NumberAnimation { duration: 300 } }
    }

    FrostSidebar {
        id: sidebar
        width: 200
        height: parent.height
        topInset: app.topInset

        SectionHeader { text: qsTr("Library") }
        SidebarItem { width: 180; icon: "play"; text: qsTr("Now Playing"); selected: app.section === "now"; onClicked: app.section = "now" }
        SidebarItem { width: 180; icon: "images"; text: qsTr("Albums"); selected: app.section === "albums"; onClicked: app.section = "albums" }
        SidebarItem { width: 180; icon: "user"; text: qsTr("Artists"); selected: app.section === "artists"; onClicked: app.section = "artists" }
        SidebarItem { width: 180; icon: "music"; text: qsTr("Songs"); selected: app.section === "songs"; onClicked: app.section = "songs" }
        SectionHeader { text: qsTr("Playlists") }
        SidebarItem { width: 180; icon: "heart"; text: qsTr("Favourites"); badge: "128" }
        SidebarItem { width: 180; icon: "sparkles"; text: qsTr("Deep Focus"); badge: "42" }
        SidebarItem { width: 180; icon: "moon"; text: qsTr("Late Night"); badge: "36" }
    }

    // ── Now playing ─────────────────────────────────────────────────────────
    Item {
        id: hero
        anchors.left: sidebar.right
        anchors.right: queue.left
        anchors.top: parent.top
        anchors.topMargin: app.topInset
        anchors.bottom: parent.bottom

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
                    text: qsTr("QTOS Session · generated live, %1 BPM").arg(app.track.session?.bpm ?? "")
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
                IconButton { anchors.verticalCenter: parent.verticalCenter; icon: "shuffle"; iconSize: 16 }
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
                IconButton { anchors.verticalCenter: parent.verticalCenter; icon: "repeat"; iconSize: 16 }
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
        width: 280

        Divider { vertical: true; height: parent.height }

        Text {
            x: 18
            y: 14
            text: qsTr("Up Next")
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
            delegate: Item {
                id: track
                required property var modelData
                required property int index
                readonly property bool current: index === app.media.queueIndex
                width: ListView.view.width
                height: 50
                Surface {
                    anchors.fill: parent
                    radius: Tokens.radius.m
                    color: track.current ? Theme.fillSelected : trackHover.hovered ? Theme.fillControl : "transparent"
                }
                Artwork {
                    id: thumb
                    x: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 36
                    height: 36
                    hue: track.modelData.hue
                    glyphVisible: false
                }
                Column {
                    anchors.left: thumb.right
                    anchors.leftMargin: 10
                    anchors.right: duration.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    Text {
                        width: parent.width
                        text: track.modelData.title
                        font: track.current ? Theme.bodyStrong : Theme.body
                        color: track.current ? Theme.accentText : Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: track.modelData.artist
                        font: Theme.footnote
                        color: Theme.textTertiary
                        elide: Text.ElideRight
                    }
                }
                Text {
                    id: duration
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: track.modelData.duration > 0 ? app.formatTime(track.modelData.duration) : ""
                    font: Theme.caption
                    color: Theme.textTertiary
                }
                HoverHandler { id: trackHover }
                TapHandler { onTapped: app.media.playIndex(track.index) }
            }
        }
    }
}
