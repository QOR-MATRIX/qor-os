pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QQ
import ARQADE

// ARQADE's home (DIRECTION P7.19): the portal behind everything, the name and the player's Sparks over it, then the
// games, the arenas and their rankings. The four games and the two arenas are rebuilt in 3D (P7.20, P7.24), and each
// card plays its game here (playRequested): an arena against another person, on ARQADE's server.
//
// Changed for QOR OS: a controller's d-pad and the arrow keys take the keyboard's focus to the nearest card, button or
// tab that way, its ring showing, and the pad's A or Return presses it as it comes up. The page follows the focus, and
// back from a game the focus is on the card that started it. ARQADE has the controller to itself in Prism (its record
// takes it), so the home answers it here, through QQ's Input, as the games do.
Item {
    id: home
    required property var link
    required property var server
    required property var player
    required property bool launched

    readonly property bool wide: width >= 1180
    readonly property real gutter: Math.max(28, Math.min(72, width * 0.05))
    property string webError: ""

    /// A rebuilt game's card was pressed: play it, by its kind.
    signal playRequested(string kind)

    /// The card a game was started from by the keyboard or a controller, focused again when the home comes back.
    property Item returnTo: null
    /// The way the d-pad or an arrow is held ("arqade.left"), or "": held, the focus goes on that way again and again.
    property string held: ""
    readonly property var ways: ({ "arqade.up": [0, -1], "arqade.down": [0, 1], "arqade.left": [-1, 0], "arqade.right": [1, 0] })
    /// A or Return went down on the home: it presses as it comes up.
    property bool pressing: false
    /// The home's hands are being defined: what is held already was not pressed here.
    property bool defining: false

    function play(card) {
        home.webError = ""
        home.returnTo = card.visualFocus ? card : null
        home.playRequested(card.kind)
    }

    // The home's hands, QQ's Input actions, defined only while it shows: a game's run keeps every action held in it.
    // A button held as they are defined (on the way back from a game) presses nothing.
    function hands(on) {
        if (on) {
            home.defining = true
            Input.define("arqade.up", ["Up"], ["Up"])
            Input.define("arqade.down", ["Down"], ["Down"])
            Input.define("arqade.left", ["Left"], ["Left"])
            Input.define("arqade.right", ["Right"], ["Right"])
            Input.define("arqade.press", ["Enter"], ["A"])
            home.defining = false
        } else {
            home.held = ""
            home.pressing = false
            again.stop()
            for (const name of ["arqade.up", "arqade.down", "arqade.left", "arqade.right", "arqade.press"])
                Input.forget(name)
        }
    }

    function holds(item) {
        for (let i = item; i; i = i.parent)
            if (i === home)
                return true
        return false
    }

    // The first game, where the focus starts.
    function first() {
        const card = soloCards.itemAt(0)
        if (card)
            card.forceActiveFocus(Qt.TabFocusReason)
    }

    function step(way) {
        const from = home.Window.activeFocusItem
        if (!from || !from.activeFocusOnTab || !home.holds(from)) {
            home.first()
            return
        }
        const to = home.nearest(from, way[0], way[1])
        if (to)
            to.forceActiveFocus(Qt.TabFocusReason)
    }

    function press() {
        const item = home.Window.activeFocusItem
        // A button, whatever its kind: QML's instanceof does not know the controls' C++ types.
        if (item && home.holds(item) && typeof item.animateClick === "function")
            item.animateClick()
        else
            home.first()
    }

    // The control nearest `from` that way: one Tab reaches on the home, both of whose edges across the way lie further
    // that way than `from`'s. One level with it (overlapping it across the way) comes before one that is not; then the
    // nearest along the way, then across it, as a television's remote moves.
    function nearest(from, dx, dy) {
        const a = from.mapToItem(null, 0, 0, from.width, from.height)
        let best = null
        let bestScore = Infinity
        for (let item = from.nextItemInFocusChain(true), n = 0; item && item !== from && n < 100;
             item = item.nextItemInFocusChain(true), n++) {
            if (!home.holds(item))
                continue
            const b = item.mapToItem(null, 0, 0, item.width, item.height)
            const ahead = dx > 0 ? b.x > a.x && b.x + b.width > a.x + a.width
                        : dx < 0 ? b.x < a.x && b.x + b.width < a.x + a.width
                        : dy > 0 ? b.y > a.y && b.y + b.height > a.y + a.height
                                 : b.y < a.y && b.y + b.height < a.y + a.height
            if (!ahead)
                continue
            const gap = Math.max(0, dx > 0 ? b.x - (a.x + a.width) : dx < 0 ? a.x - (b.x + b.width)
                                  : dy > 0 ? b.y - (a.y + a.height) : a.y - (b.y + b.height))
            const across = dx !== 0 ? Math.abs(b.y + b.height / 2 - (a.y + a.height / 2))
                                    : Math.abs(b.x + b.width / 2 - (a.x + a.width / 2))
            const level = dx !== 0 ? b.y < a.y + a.height && b.y + b.height > a.y
                                   : b.x < a.x + a.width && b.x + b.width > a.x
            const score = (level ? 0 : 1e9) + 13 * gap * gap + across * across
            if (score < bestScore) {
                best = item
                bestScore = score
            }
        }
        return best
    }

    // The page scrolls to show what has the focus, with a margin around it.
    function reveal(item) {
        const r = item.mapToItem(page.contentItem, 0, 0, item.width, item.height)
        const margin = 40
        let y = page.contentY
        if (r.y - margin < y)
            y = r.y - margin
        else if (r.y + r.height + margin > y + page.height)
            y = r.y + r.height + margin - page.height
        y = Math.max(0, Math.min(Math.max(0, page.contentHeight - page.height), y))
        if (Math.abs(y - page.contentY) < 1)
            return
        scroll.to = y
        scroll.restart()
    }

    onVisibleChanged: {
        home.hands(visible)
        if (visible && home.returnTo)
            home.returnTo.forceActiveFocus(Qt.TabFocusReason)
    }
    Component.onDestruction: home.hands(false)

    Connections {
        target: Input
        enabled: home.visible
        function onPressed(action) {
            if (home.defining)
                return
            if (action === "arqade.press") {
                home.pressing = true
            } else if (home.ways[action]) {
                home.held = action
                home.step(home.ways[action])
                again.interval = 380
                again.restart()
            }
        }
        // A press as A or Return comes up, as a console's does: a game it starts never has it held, and never takes it
        // as its own first press.
        function onReleased(action) {
            if (action === "arqade.press" && home.pressing) {
                home.pressing = false
                home.press()
            } else if (action === home.held) {
                home.held = ""
                again.stop()
            }
        }
    }

    // Held, the d-pad or an arrow goes on, after a moment, a few times a second.
    Timer {
        id: again
        repeat: true
        onTriggered: {
            interval = 120
            if (home.held !== "")
                home.step(home.ways[home.held])
        }
    }

    Connections {
        target: home.Window
        function onActiveFocusItemChanged() {
            const item = home.Window.activeFocusItem
            if (item && home.visible && item !== home && home.holds(item))
                home.reveal(item)
        }
    }

    NumberAnimation {
        id: scroll
        target: page
        property: "contentY"
        duration: Theme.enter
        easing.type: Easing.BezierSpline
        easing.bezierCurve: Theme.ease
    }

    Portal {
        anchors.fill: parent
        pointer: Qt.point(hover.point.position.x / Math.max(1, home.width) * 2 - 1,
                          hover.point.position.y / Math.max(1, home.height) * 2 - 1)
        scroll: page.contentY
        coreAt: home.wide ? 0.6 : 0.72
        coreDown: home.wide ? 0.36 : 0.3
    }

    // The page darkens below the hero, so what is read there reads against the void, not the scene.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: Math.min(0.9, 520 / Math.max(1, home.height)); color: Qt.rgba(Theme.void_.r, Theme.void_.g, Theme.void_.b, 0.35) }
            GradientStop { position: 1.0; color: Qt.rgba(Theme.void_.r, Theme.void_.g, Theme.void_.b, 0.82) }
        }
    }

    // And the hero's words read against a shade the portal fades into from the left.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        width: parent.width * 0.62
        height: Math.max(520, parent.height * 0.7) - page.contentY
        visible: height > 0
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(Theme.void_.r, Theme.void_.g, Theme.void_.b, 0.72) }
            GradientStop { position: 0.55; color: Qt.rgba(Theme.void_.r, Theme.void_.g, Theme.void_.b, 0.4) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    HoverHandler { id: hover }

    Connections {
        target: home.link
        function onWebFailed(message) { home.webError = message }
    }

    Component.onCompleted: {
        // Started by hand there is no launcher to sign in with: the rankings say so rather than wait.
        if (!home.launched)
            home.server.signIn()
        home.hands(home.visible)
    }

    Flickable {
        id: page
        objectName: "page"
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.implicitHeight + 96
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: column
            x: home.gutter
            y: 56
            width: page.width - home.gutter * 2
            spacing: 56

            // The hero: the name, what ARQADE is, and who is playing.
            // Wide, the name sits left, the portal's core right of it, and the player at the far right below the core.
            GridLayout {
                columns: home.wide ? 3 : 1
                columnSpacing: 32
                rowSpacing: 32
                Layout.fillWidth: true
                Layout.minimumHeight: home.wide ? Math.max(420, home.height * 0.56) : 0

                ColumnLayout {
                    spacing: 18
                    Layout.preferredWidth: home.wide ? Math.min(720, column.width * 0.5) : column.width
                    Layout.alignment: Qt.AlignTop

                    Text {
                        text: qsTr("THE QOR ARCADE")
                        font.family: Theme.ui
                        font.pixelSize: Theme.sizeBody
                        font.weight: Font.DemiBold
                        font.letterSpacing: Theme.sizeBody * Theme.trackingEyebrow
                        color: Theme.gilt
                    }
                    Wordmark {
                        size: Math.max(56, Math.min(112, home.width * 0.075))
                    }
                    Text {
                        text: qsTr("Play the games of the Demiurge devnet, climb the arenas' rankings, and be rewarded in Sparks for runs ARQADE confirms.")
                        font.family: Theme.body
                        font.pixelSize: Theme.sizeHeading
                        color: Theme.inkBody
                        wrapMode: Text.WordWrap
                        lineHeight: 1.25
                        Layout.fillWidth: true
                        Layout.maximumWidth: 560
                    }
                    RowLayout {
                        spacing: 12
                        ArqButton {
                            primary: true
                            text: qsTr("Play today's versions")
                            onClicked: { home.webError = ""; home.link.openWeb() }
                        }
                    }
                    Text {
                        visible: home.webError.length > 0
                        text: home.webError
                        font.family: Theme.body
                        font.pixelSize: Theme.sizeBody
                        color: Theme.bad
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                Item {
                    visible: home.wide
                    Layout.fillWidth: true
                }

                PlayerPanel {
                    player: home.player
                    launched: home.launched
                    Layout.alignment: home.wide ? Qt.AlignBottom : Qt.AlignLeft
                    Layout.preferredWidth: 400
                    Layout.maximumWidth: column.width
                }
            }

            // The four games played alone.
            ColumnLayout {
                spacing: 24
                Layout.fillWidth: true

                Heading {
                    eyebrow: qsTr("Games")
                    title: qsTr("Four games, rebuilt in 3D")
                    line: qsTr("Each is rebuilt with physically based light, particles and sound, and with rules that replay exactly, so a run can be confirmed and rewarded. All four play here.")
                    Layout.fillWidth: true
                }
                GridLayout {
                    columns: Math.max(1, Math.min(4, Math.floor((column.width + 20) / 280)))
                    columnSpacing: 20
                    rowSpacing: 20
                    Layout.fillWidth: true
                    Repeater {
                        id: soloCards
                        model: [
                            { kind: "void", number: "01", name: "Void Runner", tag: qsTr("Reflex / survival"), line: qsTr("Outrun the collapse."), here: true },
                            { kind: "synapse", number: "02", name: "Synapse", tag: qsTr("Memory / pattern"), line: qsTr("Trust the signal. Question your mind."), here: true },
                            { kind: "orbital", number: "03", name: "Orbital", tag: qsTr("Precision / timing"), line: qsTr("Find order in the chaos."), here: true },
                            { kind: "rift", number: "04", name: "Rift Survivor", tag: qsTr("Survival / evolution"), line: qsTr("One signal. Ninety seconds. Evolve."), here: true },
                        ]
                        GameCard {
                            id: solo
                            required property var modelData
                            kind: modelData.kind
                            number: modelData.number
                            name: modelData.name
                            tag: modelData.tag
                            line: modelData.line
                            here: modelData.here === true
                            Layout.fillWidth: true
                            Layout.preferredHeight: 360
                            onClicked: {
                                home.webError = ""
                                if (solo.here)
                                    home.play(solo)
                                else
                                    home.link.openWeb()
                            }
                        }
                    }
                }
            }

            // The arenas, played against another person, and their rankings.
            ColumnLayout {
                spacing: 24
                Layout.fillWidth: true

                Heading {
                    eyebrow: qsTr("Arenas")
                    title: qsTr("Two arenas, one ranking")
                    line: qsTr("Played against another person on ARQADE's server. Wins, losses and draws rank you in each arena and across both.")
                    Layout.fillWidth: true
                }
                GridLayout {
                    columns: home.wide ? 3 : (column.width >= 640 ? 2 : 1)
                    columnSpacing: 20
                    rowSpacing: 20
                    Layout.fillWidth: true

                    GameCard {
                        id: flux
                        kind: "flux"
                        number: "I"
                        name: "Flux Four"
                        tag: qsTr("Arena / strategy")
                        line: qsTr("Gravity is a weapon. Think four moves ahead.")
                        here: true
                        Layout.fillWidth: true
                        Layout.preferredHeight: 360
                        Layout.alignment: Qt.AlignTop
                        onClicked: home.play(flux)
                    }
                    GameCard {
                        id: reversi
                        kind: "reversi"
                        number: "II"
                        name: "Rift Reversi"
                        tag: qsTr("Arena / territory")
                        line: qsTr("Turn their territory into your advantage.")
                        here: true
                        Layout.fillWidth: true
                        Layout.preferredHeight: 360
                        Layout.alignment: Qt.AlignTop
                        onClicked: home.play(reversi)
                    }
                    Rankings {
                        server: home.server
                        launcherSignedIn: home.player.signed_in === true
                        Layout.fillWidth: true
                        Layout.columnSpan: home.wide ? 1 : (column.width >= 640 ? 2 : 1)
                        Layout.minimumWidth: 300
                        Layout.preferredWidth: 420
                        Layout.alignment: Qt.AlignTop
                    }
                }
            }

            Text {
                text: qsTr("ARQADE runs on the Demiurge devnet. Sparks are thousandths of a CGT; on the devnet they are test CGT, with no value.")
                font.family: Theme.body
                font.pixelSize: Theme.sizeUi
                color: Theme.inkMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
