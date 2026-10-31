// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
// Darkveil (QOR OS's QOR-0017): a valley gone dark, three beacons to light and
// a king to end, on foot, with a blade, a bolt and what the people of Hearth can spare.
//
// The rules are logic/rules.mjs, the one place they live: ARQADE's service runs the same file to replay a run. This logic
// hands them every event as QQ's tick delivers it (a screen button's click, an action's press and release, a step) and
// draws what they say: the hero, the people, the enemies, shots and what lies on the ground where the rules put them, and
// the screens for the rules' phase. So a run QQ records is the rules' input exactly, and its outcome is theirs (`outcome`).
// Nothing drawn here decides anything, which is why it may use trigonometry the rules may not. How high the land stands
// (logic/land.mjs) is asked only here: the rules decide what can be walked on by the tile alone.
//
// The valley is a world tilted toward where the eye looks from, turned by the rules' view, and it moves under the eye so
// the hero stays in the middle. Its land and what stands on it are QQ props, from make_assets.py. What the rules hold is
// drawn from pools made as play starts, each thing keeping its piece while it lasts, and only what is near is drawn; an idle
// piece is hidden and plays no clip, so it costs nothing. The figures' clips run on the game's clock, each sped to last as
// long as the rules' count of what it shows. With less motion asked for (Look), the eye keeps exactly to the hero, the view
// turns at once, things on the ground do not turn, and the words do not swell.
//
// What happens somewhere is heard from there, through QQ's spatial audio (`at`). A level, a beacon, the gate, a great one's
// waking, words said, the run's end and the music are heard everywhere.

import QQ
import "rules.mjs" as Rules
import "land.mjs" as Land

Logic {
    id: darkveil

    /// The rules' game: their state, which only they change.
    property var game: null
    /// The rules themselves, for ARQADE's games suite to hand its pilot.
    readonly property var rules: Rules
    /// The one player QQ plays.
    readonly property var hero: game ? game.players[0] : null

    /// The rules' units to the valley's: a tile of 40 units is one.
    readonly property real k: 1 / Rules.TILE
    readonly property var away: Qt.vector3d(0, -40, 0)
    /// How far from the hero anything is drawn, in the rules' units.
    readonly property real drawReach: 26 * Rules.TILE
    property var pools: ({})
    /// Each pool's pieces by the number of the thing each shows, so a thing keeps its piece while it lasts.
    property var assigned: ({})
    /// Where the eye's middle is, in the rules' units and the land's height, and how far the valley is turned.
    property real eyeX: 0
    property real eyeY: 0
    property real eyeHigh: 0
    property real turned: 0
    property real spin: 0
    /// The enemy last struck, whose health the screen shows while it stands.
    property int foe: -1
    /// Those just fallen, shown falling for a moment: the rules have already forgotten them.
    property var fallen: []
    /// What the screens last showed, so nothing is asked of them until it changes.
    property string showing: ""
    /// The region the hero was last in.
    property string where: ""
    /// The valley's scenery by sector, once its files have loaded, and what the sectors were last shown for.
    property var sectors: []
    property string sectorsFor: ""

    /// How high the land stands at (x, z), in tiles.
    function groundAt(x, z) {
        const across = Land.ACROSS, last = across - 2
        const c = Math.max(0, Math.min(last, Math.floor(x))), r = Math.max(0, Math.min(last, Math.floor(z)))
        const fx = x - c, fz = z - r, at = r * across + c
        const a = Land.CORNERS[at], b = Land.CORNERS[at + 1], d = Land.CORNERS[at + across], e = Land.CORNERS[at + across + 1]
        return ((a + (b - a) * fx) * (1 - fz) + (d + (e - d) * fx) * fz) / 100
    }

    function at(x, y, lift) {
        return Qt.vector3d(x * k, groundAt(x * k, y * k) + lift, y * k)
    }

    /// Where a sound at a place in the valley is heard from, in the scene.
    function heardAt(x, y) {
        const valley = entity("Valley")
        return valley ? valley.mapPositionToScene(at(x, y, 0.4)) : null
    }

    /// Something only looked at, made here, hidden until it shows something.
    function piece(kind, fields) {
        fields.position = away
        if (kind === "Shape")
            fields.body = "none"
        const made = spawn(kind, fields, entity("Valley"))
        if (made)
            made.visible = false
        return made
    }

    function shapes(name, count, fields) {
        const made = []
        for (let i = 0; i < count; i++) {
            const f = Object.assign({}, fields)
            f.name = name + " " + i
            made.push(piece("Shape", f))
        }
        return made
    }

    function figures(name, source, count, size) {
        const made = []
        for (let i = 0; i < count; i++)
            made.push(piece("Prop", { name: name + " " + i, source: "../assets/" + source + ".glb",
                                      scale: Qt.vector3d(size, size, size) }))
        return made
    }

    function text(name, words) {
        const shown = entity(name)
        if (shown)
            shown.text = words
    }

    function bar(name, value, most) {
        const shown = entity(name)
        if (shown) {
            shown.maximum = Math.max(1, most)
            shown.value = value
        }
    }

    function button(name) {
        const b = entity(name)
        if (b)
            b.clicked.connect(() => {
                Rules.click(game, name)
                show()
            })
    }

    function counted(n) {
        return n.toLocaleString(Qt.locale("en_GB"), "f", 0)
    }

    /// Words across the middle of the screen for a moment: a place reached, a warden waking, a level.
    function say(words, colour) {
        const message = entity("Message")
        if (!message)
            return
        message.text = words
        message.colour = colour
        if (!Look.reducedMotion) {
            message.size = 38
            tween(message, "size", 30, 0.5, "outCubic")
        }
        after(2.6, () => {
            if (message.text === words)
                message.text = ""
        })
    }

    /// `words` broken to lines of at most `most` letters, as many as there are lines for.
    function broken(words, most, lines) {
        const out = []
        let line = ""
        for (const word of words.split(" ")) {
            if (line.length + word.length + 1 > most && out.length < lines - 1) {
                out.push(line)
                line = word
            } else {
                line = line === "" ? word : line + " " + word
            }
        }
        out.push(line)
        while (out.length < lines)
            out.push("")
        return out
    }

    /// The region a place lies in, as the screen names it.
    function regionOf(x, y) {
        const c = Math.floor(x / Rules.TILE), r = Math.floor(y / Rules.TILE)
        const apart = (name) => {
            const [pc, pr] = Rules.PLACES[name]
            return Math.max(Math.abs(pc - c), Math.abs(pr - r))
        }
        if (apart("hearth") <= 9)
            return "HEARTH"
        if (apart("crown") <= 8)
            return "THE CROWN"
        const tile = Rules.WORLD[r][c]
        return tile === ":" ? "THE MIREWOOD" : tile === ";" ? "THE ASHEN HOLD" : tile === "'" ? "THE HIGH PASS"
             : tile === "," || tile === "=" ? where : "THE MEADOW"
    }

    /// The screens, the world's hold and the words, for the rules' phase; and what the run came to.
    function show() {
        outcome = Rules.outcome(game)
        const phase = game.phase, p = hero
        if (phase === "paused" && !paused)
            pause()
        else if (phase !== "paused" && paused)
            resume()
        const talking = p.talk !== null && phase === "playing"
        const list = talking ? Rules.offers(game) : []
        const target = game.world.enemies.find((e) => e.id === foe)
                       ?? game.world.enemies.find((e) => e.kind >= Rules.WARDEN && e.awake && e.act !== "home")
        const showFoe = target && !talking && (phase === "playing" || phase === "paused")
        const key = [phase, talking, list.length, showFoe, p.hurt > Rules.HURT_HOLD - 10].join()
        if (key !== showing) {
            showing = key
            onlyScreen(phase === "title" ? "title" : phase === "over" ? "result" : talking ? "talk" : "hud")
            if (phase === "paused")
                showScreen("pause")
            if (showFoe)
                showScreen("foe")
            if (p.hurt > Rules.HURT_HOLD - 10 && phase === "playing")
                showScreen("hurt")
            for (let n = 0; n < list.length; n++)
                showScreen("say" + (n + 1))
        }
        if (showFoe) {
            text("Foe", Rules.KINDS[target.kind].name.toUpperCase())
            bar("Foe health", target.health, target.most)
        }
        if (talking) {
            text("Who", Rules.PEOPLE[p.talk.who].name)
            const lines = broken(p.talk.line, 74, 4)
            for (let n = 0; n < 4; n++)
                text("Line " + n, lines[n])
            list.forEach((o, n) => {
                const b = entity("Say " + (n + 1))
                if (b) {
                    b.text = (n === p.talk.cursor ? "› " : "") + (n + 1) + ".  " + o.words
                    b.colour = o.able ? (n === p.talk.cursor ? "#f3ede1" : "#c9a45c") : "#6c6f76"
                }
            })
        }
        text("Objective", Rules.objective(game).words)
        text("Place", where)
        bar("Health", p.health, p.most)
        text("Health words", p.health + " / " + p.most)
        bar("Mana", p.mana, p.manaMost)
        const from = p.level > 1 ? Rules.nextXp(p.level - 1) : 0
        bar("Experience", p.xp - from, Rules.nextXp(p.level) - from)
        text("Stats", "LEVEL " + p.level + "  ·  GOLD " + counted(p.gold)
             + (p.falls > 0 ? "  ·  FALLS " + p.falls + " OF " + Rules.LIVES : ""))
        text("Kit", "POTIONS " + p.potions + "  ·  " + Rules.BLADES[p.blade].name + "  ·  " + Rules.WARDS[p.ward].name)
        const person = Rules.PEOPLE[nearestPerson()]
        text("Hint", person ? "Talk to " + person.name + "  (E, or the pad's LT)" : "")
        if (phase === "over") {
            text("Result", game.won ? "THE VALLEY BURNS BRIGHT" : p.out ? "THE VEIL TOOK YOU" : "THE VEIL HAS CLOSED")
            text("Final", "SCORE " + counted(p.score))
            text("Details", game.world.beacons.filter((lit) => lit).length + " of 3 beacons  ·  level " + p.level + "  ·  "
                 + p.kills + " fallen  ·  " + counted(p.gold) + " gold")
        }
    }

    /// Who stands in reach of the hero, for the hint: the rules decide who answers.
    function nearestPerson() {
        let found = "", best = Rules.TALK_REACH * Rules.TALK_REACH
        for (const who in Rules.PEOPLE) {
            const place = Rules.placeAt(Rules.PEOPLE[who].place)
            const dx = place.x - hero.x, dy = place.y - hero.y, d = dx * dx + dy * dy
            if (d < best) {
                best = d
                found = who
            }
        }
        return found
    }

    /// Draw `things` from pool `name`, each keeping its piece while it lasts, at `place(thing)`, then `look(piece, thing)`.
    function draw(name, things, place, look) {
        const list = pools[name]
        const was = assigned[name] || {}
        const now = {}
        const taken = list.map(() => false)
        for (const t of things) {
            if (was[t.id] !== undefined) {
                now[t.id] = was[t.id]
                taken[was[t.id]] = true
            }
        }
        let free = 0
        for (const t of things) {
            if (now[t.id] === undefined) {
                while (free < list.length && taken[free])
                    free += 1
                if (free >= list.length)
                    continue
                now[t.id] = free
                taken[free] = true
            }
            const p = list[now[t.id]]
            p.position = place(t)
            p.visible = true
            if (look)
                look(p, t)
        }
        for (const id in was) {
            if (now[id] === undefined) {
                const p = list[was[id]]
                p.position = away
                p.visible = false
                if (p.clip !== undefined)
                    p.clip = ""
            }
        }
        assigned[name] = now
    }

    /// Only the scenery the eye can see: near enough, and not well behind it. It is asked again as the hero's tile or view
    /// changes, and the shadows of what is hidden are no loss so far off.
    function showSectors() {
        const size = 8, reach = 34, behind = 10
        if (sectors.length === 0) {
            const found = []
            for (let n = 0; n < 4; n++) {
                const scenery = entity("Scenery " + n)
                if (!scenery || scenery.status !== 1)
                    return
                for (let c = 0; c < Rules.COLS / size; c++) {
                    for (let r = 0; r < Rules.ROWS / size; r++) {
                        const part = scenery.part("Sector_" + c + "_" + r)
                        if (part)
                            found.push({ part: part, x: (c + 0.5) * size, y: (r + 0.5) * size })
                    }
                }
            }
            sectors = found
        }
        const p = hero, hx = p.x * k, hy = p.y * k
        const key = Math.floor(hx) + ":" + Math.floor(hy) + ":" + p.view
        if (key === sectorsFor)
            return
        sectorsFor = key
        const [fx, fy] = Rules.VIEWS[p.view]
        for (const s of sectors) {
            const dx = s.x - hx, dy = s.y - hy
            s.part.visible = dx * dx + dy * dy < reach * reach && dx * fx + dy * fy > -behind
        }
    }

    /// Degrees about the vertical that turn a figure's +z toward (fx, fy).
    function toward(fx, fy) {
        return Math.atan2(fx, fy) * 180 / Math.PI
    }

    /// A figure's clip, played at `speed` (looping or not), changed only when it changes.
    function plays(figure, clip, speed, loops) {
        if (figure.clip !== clip || figure.clipLoops !== loops) {
            figure.clipLoops = loops
            figure.clipSpeed = speed
            figure.clip = clip
        } else {
            figure.clipSpeed = speed
        }
    }

    /// What an enemy's body is doing, from what the rules say it is about.
    function enemyLook(figure, e) {
        const kind = Rules.KINDS[e.kind]
        figure.eulerRotation = Qt.vector3d(0, toward(e.fx, e.fy), 0)
        if (e.act === "windup") {
            const seconds = kind.windup / 60
            if (kind.reach === 0)
                plays(figure, "cast", 0.14 / seconds, false)
            else
                plays(figure, "strike", 0.42 / seconds, false)
        } else if (e.act === "recover") {
            plays(figure, "idle", 1, true)
        } else if (e.awake) {
            plays(figure, "walk", Math.max(0.6, kind.speed / Rules.HERO_SPEED) * 1.3, true)
        } else {
            plays(figure, "idle", 1, true)
        }
    }

    onStarted: {
        game = Rules.create(scene.playSeed)
        const sun = entity("Sun")
        if (sun) {
            // A valley's width of shadow, no more, so what is near has shadows worth the name.
            sun.shadowMapFar = 34
            sun.shadowMapQuality = 2
            sun.pcfFactor = 1
        }
        const p = {}
        p.wolf = figures("Wolf", "wolf", 12, 1)
        p.shade = figures("Shade", "shade", 10, 1)
        p.thornling = figures("Thornling", "thornling", 10, 1)
        p.revenant = figures("Revenant", "revenant", 12, 1)
        p.bonecaster = figures("Bonecaster", "bonecaster", 10, 1)
        p.greatWolf = figures("Great wolf", "wolf", 1, 1.7)
        p.warden = figures("Warden", "warden", 1, 1)
        p.king = figures("King", "king", 1, 1)
        p.shots = shapes("Shot", 12, { form: "sphere", scale: Qt.vector3d(0.2, 0.2, 0.2), colour: "#0a2a3a", metalness: 0,
                                       roughness: 0.4, emissive: "#8fd8ff", emissivePower: 0.95 })
        p.thorns = shapes("Thorn", 24, { form: "cone", scale: Qt.vector3d(0.09, 0.26, 0.09), colour: "#23300f", metalness: 0,
                                         roughness: 0.5, emissive: "#a8d04a", emissivePower: 0.55 })
        p.curses = shapes("Curse", 24, { form: "sphere", scale: Qt.vector3d(0.24, 0.24, 0.24), colour: "#1a0f2e", metalness: 0,
                                         roughness: 0.4, emissive: "#b07cff", emissivePower: 0.9 })
        p.gold = shapes("Gold", Rules.MOST_ITEMS, { form: "cylinder", scale: Qt.vector3d(0.24, 0.04, 0.24),
                                                    eulerRotation: Qt.vector3d(90, 0, 0), colour: "#fbd68d", metalness: 0.9,
                                                    roughness: 0.25, emissive: "#ffb83a", emissivePower: 0.4 })
        p.potions = shapes("Potion", 12, { form: "sphere", scale: Qt.vector3d(0.2, 0.26, 0.2), colour: "#5a0d12", metalness: 0.1,
                                           roughness: 0.15, emissive: "#ff3b4a", emissivePower: 0.7 })
        p.herbs = shapes("Moonherb", Rules.HERBS.length, { form: "cone", scale: Qt.vector3d(0.24, 0.42, 0.24), colour: "#16261c",
                                                           metalness: 0, roughness: 0.5, emissive: "#a8ffd8", emissivePower: 0.8 })
        p.ore = shapes("Ironroot", Rules.ORE.length, { form: "cube", scale: Qt.vector3d(0.32, 0.24, 0.3), colour: "#3a2b24",
                                                       metalness: 0.8, roughness: 0.4, emissive: "#ff9a4a", emissivePower: 0.5 })
        p.sparks = shapes("Spark", 8, { form: "sphere", scale: Qt.vector3d(0.3, 0.3, 0.3), colour: "#fff4e0", metalness: 0,
                                        roughness: 0.5, emissive: "#fff4e0", emissivePower: 0.95 })
        pools = p
        for (const who in Rules.PEOPLE) {
            const name = who.charAt(0).toUpperCase() + who.substring(1)
            const place = Rules.placeAt(Rules.PEOPLE[who].place)
            const made = spawn("Prop", { name: name, source: "../assets/" + who + ".glb", position: at(place.x, place.y, 0),
                                         clip: "idle" }, entity("Valley"))
            if (made)
                made.visible = true
        }
        Rules.BEACONS.forEach((beacon, n) => {
            const place = Rules.placeAt(beacon.place)
            entity("Beacon " + n).position = at(place.x, place.y, 1.45)
            entity("Beacon light " + n).position = at(place.x, place.y, 2.2)
        })
        for (const name of ["Play", "Resume", "Play again", "Say 1", "Say 2", "Say 3", "Say 4", "Say 5", "Say 6"])
            button(name)
        eyeX = hero.x
        eyeY = hero.y
        eyeHigh = groundAt(hero.x * k, hero.y * k)
        where = "HEARTH"
        show()
    }

    // W, A, S and D, the arrows, the pad's d-pad or left stick, or the buttons under a finger move, as the view faces; Z, C or
    // the shoulders turn the view; Space, J or A strikes; L, Shift or B rolls; K or X looses a bolt; I or RT a nova; Q, H or Y
    // drinks; E, Return or LT talks; 1 to 6 answer; P or Start pauses.
    onPressed: (action) => {
        Rules.press(game, action)
        show()
    }
    onReleased: (action) => Rules.release(game, action)

    onStep: {
        const p = hero
        const before = game.phase
        const health = p.health, level = p.level, gold = p.gold, kills = p.kills, talking = p.talk !== null
        const cursor = p.talk ? p.talk.cursor : -1, line = p.talk ? p.talk.line : ""
        Rules.step(game)
        const heroAt = heardAt(p.x, p.y)
        const heard = new Set()
        // One sound of each kind a step, however many happened, from the first place it happened.
        for (const s of game.said) {
            if (heard.has(s.what))
                continue
            heard.add(s.what)
            const there = s.x !== undefined ? heardAt(s.x, s.y) : heroAt
            if (s.what === "swing") {
                play("../assets/swing.wav", { volume: 0.35, at: heroAt })
                const figure = entity("Hero")
                if (figure)
                    figure.clipTime = 0
            } else if (s.what === "hit" || s.what === "crit") {
                play(s.what === "crit" ? "../assets/crit.wav" : "../assets/hit.wav", { volume: 0.55, at: there })
                foe = s.id
            } else if (s.what === "kill") {
                play(s.kind >= Rules.WARDEN ? "../assets/slam.wav" : "../assets/kill.wav", { volume: 0.6, at: there })
            } else if (s.what === "hurt") {
                play("../assets/hurt.wav", { volume: 0.8, at: heroAt })
            } else if (s.what === "roll") {
                play("../assets/roll.wav", { volume: 0.5, at: heroAt })
            } else if (s.what === "potion") {
                play("../assets/potion.wav", { volume: 0.6, at: heroAt })
            } else if (s.what === "cast" || s.what === "nova") {
                play("../assets/cast.wav", { volume: 0.45, at: heroAt })
            } else if (s.what === "bolt") {
                play("../assets/bolt.wav", { volume: 0.5, at: heroAt })
            } else if (s.what === "burst") {
                play("../assets/burst.wav", { volume: 0.7, at: heroAt })
            } else if (s.what === "pick") {
                play(s.item === "gold" ? "../assets/coin.wav" : "../assets/potion.wav", { volume: 0.45, at: heroAt })
            } else if (s.what === "herb" || s.what === "ore") {
                play("../assets/herb.wav", { volume: 0.55, at: heroAt })
                say((s.what === "herb" ? "MOONHERB " : "IRONROOT ") + s.have, "#a8ffd8")
            } else if (s.what === "thorn" || s.what === "curse") {
                play("../assets/" + s.what + ".wav", { volume: 0.45, at: there })
            } else if (s.what === "wake") {
                play("../assets/wake.wav", { volume: 0.3, at: there })
            } else if (s.what === "rising") {
                play("../assets/rising.wav", { volume: 0.7, at: there })
            } else if (s.what === "slam") {
                play("../assets/slam.wav", { volume: 0.9, at: there })
            } else if (s.what === "summon") {
                play("../assets/rising.wav", { volume: 0.4, at: there })
            } else if (s.what === "warden") {
                play("../assets/warden.wav", { volume: 0.8 })
                say("A WARDEN WAKES", "#ff7a5c")
            } else if (s.what === "king") {
                play("../assets/king.wav", { volume: 0.9 })
                say("THE HOLLOW KING RISES", "#ff7a5c")
            } else if (s.what === "warden down") {
                say("THE WARDEN IS BROKEN", "#ead39a")
            } else if (s.what === "king down") {
                say("THE HOLLOW KING IS ENDED", "#ead39a")
            } else if (s.what === "beacon") {
                play("../assets/beacon.wav", { volume: 0.8 })
                say(Rules.BEACONS[s.beacon].name.toUpperCase() + " BURNS", "#ffd27a")
            } else if (s.what === "gate") {
                play("../assets/gate.wav", { volume: 0.9 })
                after(2.8, () => say("THE GATE STANDS OPEN", "#ffd27a"))
            } else if (s.what === "talk") {
                play("../assets/talk.wav", { volume: 0.4 })
            } else if (s.what === "yes") {
                play("../assets/yes.wav", { volume: 0.5 })
            } else if (s.what === "no") {
                play("../assets/no.wav", { volume: 0.5 })
            } else if (s.what === "fallen" && game.phase === "playing") {
                play("../assets/fallen.wav", { volume: 0.8 })
                say("YOU WAKE IN HEARTH", "#ff9a8a")
            }
        }
        for (const s of game.said) {
            if (s.what === "kill" && near(s, 30 * Rules.TILE))
                fallen.push({ id: "fell" + (game.steps) + s.kind + fallen.length, kind: s.kind, x: s.x, y: s.y, left: 1.1 })
        }
        if (p.level > level) {
            play("../assets/level.wav", { volume: 0.7 })
            say("LEVEL " + p.level + (p.level === Rules.NOVA_LEVEL ? "  ·  NOVA LEARNED" : ""), "#ead39a")
        }
        if (before !== "over" && game.phase === "over")
            play(game.won ? "../assets/won.wav" : "../assets/lost.wav", { volume: 0.9 })
        if (!game.world.enemies.some((e) => e.id === foe))
            foe = -1
        const region = regionOf(p.x, p.y)
        if (region !== where && game.phase === "playing") {
            where = region
            say(region, "#d6d1c6")
        }
        if (before !== game.phase || health !== p.health || gold !== p.gold || kills !== p.kills
                || talking !== (p.talk !== null) || (p.talk && (cursor !== p.talk.cursor || line !== p.talk.line))
                || game.steps % 6 === 0 || p.hurt === Rules.HURT_HOLD - 10)
            show()
    }

    function near(t, reach) {
        const dx = t.x - hero.x, dy = t.y - hero.y
        return dx * dx + dy * dy < reach * reach
    }

    // Only looked at: everything where the rules put it.
    onFrame: (dt) => {
        const p = hero, world = game.world
        showSectors()
        if (!Look.reducedMotion)
            spin = (spin + 90 * dt) % 360
        const turning = Look.reducedMotion ? 0 : spin
        const follow = Look.reducedMotion ? 1 : Math.min(1, dt * 8)
        eyeX += (p.x - eyeX) * follow
        eyeY += (p.y - eyeY) * follow
        eyeHigh += (groundAt(eyeX * k, eyeY * k) - eyeHigh) * follow
        // The short way round to the rules' view.
        const wanted = p.view * 45
        let by = ((wanted - turned) % 360 + 540) % 360 - 180
        turned = (turned + (Look.reducedMotion ? by : by * Math.min(1, dt * 7)) + 360) % 360
        const turn = entity("Turn")
        if (turn)
            turn.eulerRotation = Qt.vector3d(0, turned, 0)
        const valley = entity("Valley")
        if (valley)
            valley.position = Qt.vector3d(-eyeX * k, -eyeHigh, -eyeY * k)
        if (scene)
            scene.skyTurn = -turned

        const figure = entity("Hero")
        if (figure) {
            figure.position = at(p.x, p.y, 0)
            figure.eulerRotation = Qt.vector3d(0, toward(p.facing[0], p.facing[1]), 0)
            const moving = p.held.up || p.held.down || p.held.left || p.held.right
            if (p.roll > 0)
                plays(figure, "roll", 0.24 * 60 / Rules.ROLL_STEPS, false)
            else if (p.cast > 0)
                plays(figure, "cast", 0.27 * 60 / (p.casting === "nova" ? Rules.NOVA_CAST : Rules.BOLT_CAST), false)
            else if (p.swing > 0)
                plays(figure, "swing" + (p.chain + 1), 1, false)
            else if (p.hurt > Rules.HURT_HOLD - 16)
                plays(figure, "hurt", 1, false)
            else if (moving && !p.talk)
                plays(figure, "walk", 1.1, true)
            else
                plays(figure, "idle", 1, true)
        }
        const lantern = entity("Lantern")
        if (lantern)
            lantern.position = at(p.x, p.y, 2.2)

        const objective = Rules.objective(game)
        const mark = entity("Mark")
        if (mark) {
            const close = near(objective, 3 * Rules.TILE)
            mark.position = close || world.kingDown ? away : at(objective.x, objective.y, 7)
            mark.visible = !(close || world.kingDown)
        }

        const nearby = world.enemies.filter((e) => near(e, drawReach))
        const of = (n) => nearby.filter((e) => e.kind === n)
        const place = (e) => at(e.x, e.y, 0)
        draw("wolf", of(Rules.WOLF), place, enemyLook)
        draw("shade", of(Rules.SHADE), (e) => at(e.x, e.y, 0.08), enemyLook)
        draw("thornling", of(Rules.THORNLING), place, enemyLook)
        draw("revenant", of(Rules.REVENANT), place, enemyLook)
        draw("bonecaster", of(Rules.BONECASTER), place, enemyLook)
        draw("greatWolf", of(Rules.GREAT_WOLF), place, enemyLook)
        draw("warden", of(Rules.WARDEN), place, enemyLook)
        draw("king", of(Rules.KING), place, enemyLook)

        const slam = entity("Slam")
        const winding = nearby.find((e) => e.kind >= Rules.WARDEN && e.act === "windup")
        if (slam) {
            if (winding) {
                const kind = Rules.KINDS[winding.kind]
                const grown = 1 - winding.timer / kind.windup
                const across = 2 * (kind.reach + Rules.HERO_R) * k * Math.max(0.1, grown)
                slam.position = at(winding.x, winding.y, 0.05)
                slam.scale = Qt.vector3d(across, 0.01, across)
                slam.emissivePower = 0.3 + 0.6 * grown
            } else {
                slam.position = away
            }
        }
        const burst = entity("Burst")
        if (burst) {
            const bursting = p.cast > 0 && p.casting === "nova"
            if (bursting) {
                const grown = 1 - Math.max(0, p.cast - Rules.NOVA_STRIKES) / (Rules.NOVA_CAST - Rules.NOVA_STRIKES)
                const across = 2 * Rules.NOVA_REACH * k * Math.max(0.1, grown)
                burst.position = at(p.x, p.y, 0.06)
                burst.scale = Qt.vector3d(across, 0.01, across)
            } else {
                burst.position = away
            }
        }

        draw("shots", world.shots, (s) => at(s.x, s.y, 0.65))
        draw("thorns", world.bolts.filter((b) => b.kind === Rules.THORNLING), (b) => at(b.x, b.y, 0.5),
             (piece, b) => piece.eulerRotation = Qt.vector3d(90, toward(b.vx, b.vy), 0))
        draw("curses", world.bolts.filter((b) => b.kind !== Rules.THORNLING), (b) => at(b.x, b.y, 0.65))
        draw("sparks", nearby.filter((e) => e.struck > 0), (e) => at(e.x, e.y, 0.55),
             (piece, e) => {
                 const size = (Rules.KINDS[e.kind].r * k) * (0.7 + e.struck * 0.08)
                 piece.scale = Qt.vector3d(size, size, size)
             })
        const lying = world.items.filter((i) => near(i, drawReach))
        draw("gold", lying.filter((i) => i.what === "gold"), (i) => at(i.x, i.y, 0.16),
             (piece) => piece.eulerRotation = Qt.vector3d(90, 0, turning * 2))
        draw("potions", lying.filter((i) => i.what === "potion"), (i) => at(i.x, i.y, 0.16))
        const found = (list, had) => list.map(([c, r], n) => ({ id: n, x: (c + 0.5) * Rules.TILE, y: (r + 0.5) * Rules.TILE, had: had[n] }))
                                         .filter((f) => !f.had && near(f, drawReach))
        draw("herbs", found(Rules.HERBS, p.herbs), (f) => at(f.x, f.y, 0.22),
             (piece) => piece.eulerRotation = Qt.vector3d(0, turning, 0))
        draw("ore", found(Rules.ORE, p.ore), (f) => at(f.x, f.y, 0.16),
             (piece) => piece.eulerRotation = Qt.vector3d(0, turning, 0))

        // Those just fallen go down where they stood, from the first free piece of their kind.
        for (const f of fallen)
            f.left -= dt
        fallen = fallen.filter((f) => f.left > 0)
        for (const [kind, name] of [[Rules.WOLF, "wolf"], [Rules.SHADE, "shade"], [Rules.THORNLING, "thornling"], [Rules.REVENANT, "revenant"],
                                    [Rules.BONECASTER, "bonecaster"], [Rules.GREAT_WOLF, "greatWolf"], [Rules.WARDEN, "warden"], [Rules.KING, "king"]]) {
            const list = pools[name], used = assigned[name] || {}
            const held = Object.values(used)
            fallen.filter((f) => f.kind === kind).forEach((f) => {
                const free = f.piece !== undefined ? f.piece : list.findIndex((_, n) => !held.includes(n))
                if (free < 0)
                    return
                f.piece = free
                const body = list[free]
                body.position = at(f.x, f.y, 0)
                body.visible = true
                plays(body, "fall", 1, false)
            })
        }

        // The beacons burn once lit; the gate's bars sink once it is open.
        Rules.BEACONS.forEach((beacon, n) => {
            const flame = entity("Beacon " + n), light = entity("Beacon light " + n)
            if (flame)
                flame.rate = world.beacons[n] ? 70 : 0
            if (light)
                light.brightness = world.beacons[n] ? 2.2 : 0
        })
        const bars = entity("Gate bars")
        if (bars) {
            bars.position = Qt.vector3d(45.5, groundAt(45.5, 19.5) + (world.gate ? -3 : 1.05), 19.5)
        }
    }
}
