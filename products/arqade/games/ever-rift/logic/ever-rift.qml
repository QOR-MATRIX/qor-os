// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
// Ever Rift (QOR OS's QOR-0017): three floors down into the rift, cutting a
// way through husks, wisps, brutes and seers to break the warden at the bottom.
//
// The rules are logic/rules.mjs, the one place they live: ARQADE's service runs the same file to replay a run. This
// logic hands them every event as QQ's tick delivers it (a screen button's click, an action's press and release, a
// step) and draws what they say: the floor's rock, the delver, its blade, the enemies, bolts and what lies on the floor
// where the rules put them, and the screens for the rules' phase. So a run QQ records is the rules' input exactly, and
// its outcome is theirs (`outcome`). Nothing drawn here decides anything, which is why it may use trigonometry the
// rules may not.
//
// The floor is a board tilted toward where the eye looks from, so it is seen from above at an angle, and it moves under
// the eye so the delver stays in the middle. Its rock is built as blocks when a floor is carved, each row's runs of rock
// carried down while the row below has the same. What the rules hold is drawn from pools made as play starts, each
// thing keeping its piece while it lasts; nothing is made or removed during a floor. A torch goes with the delver. The
// world is held while the player pauses. With less motion asked for (Look), the eye keeps exactly to the delver, things
// on the floor do not turn, wisps do not bob, and the words do not swell.
//
// What happens somewhere is heard from there, through QQ's spatial audio (`at`): a blow from what it struck, a fall from
// the fallen, a bolt from its seer, the warden's slam from the warden, and the delver's own swings, wounds and finds
// from the delver. A level, a floor, the warden's waking, the run's end and the music are heard everywhere.

import QQ
import "rules.mjs" as Rules

Logic {
    id: everRift

    /// The rules' game: their state, which only they change.
    property var game: null

    /// The rules' units to the dungeon's: a tile of 40 units is one.
    readonly property real k: 1 / Rules.TILE
    /// How high the rock stands, in tiles.
    readonly property real rockHeight: 0.55
    readonly property var away: Qt.vector3d(0, -30, 0)
    property var pools: ({})
    /// Each pool's pieces by the number of the thing each shows, so a thing keeps its piece while it lasts.
    property var assigned: ({})
    /// The floor drawn, by the rules' count of floors carved, and the rock built for it.
    property int carved: -1
    property var rock: []
    /// Where the eye's middle is, in the rules' units: it follows the delver.
    property real eyeX: 0
    property real eyeY: 0
    property real turn: 0
    /// The enemy last struck, whose health the screen shows while it stands.
    property int foe: -1

    function at(x, y, lift) {
        return Qt.vector3d(x * k, lift, y * k)
    }

    /// Where a sound at a place on the floor is heard from, in the scene.
    function heardAt(x, y) {
        const dungeon = entity("Dungeon")
        return dungeon ? dungeon.mapPositionToScene(at(x, y, 0.3)) : null
    }

    /// Something of the scene's look, made here: only looked at, so it has no body. Hidden until it shows something,
    /// so a pool's idle pieces cost nothing to draw, in the torch's shadows least of all.
    function piece(fields) {
        fields.body = "none"
        fields.position = away
        const made = spawn("Shape", fields, entity("Dungeon"))
        if (made)
            made.visible = false
        return made
    }

    function pool(name, count, fields) {
        const made = []
        for (let i = 0; i < count; i++) {
            const f = Object.assign({}, fields)
            f.name = name + " " + i
            made.push(piece(f))
        }
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

    /// Words across the middle of the screen for a moment: a floor reached, the warden waking, a relic found.
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
        after(2.4, () => {
            if (message.text === words)
                message.text = ""
        })
    }

    /// The screens, the world's hold and the words, for the rules' phase; and what the run came to.
    function show() {
        outcome = Rules.outcome(game)
        const phase = game.phase
        if (phase === "paused" && !paused)
            pause()
        else if (phase !== "paused" && paused)
            resume()
        onlyScreen(phase === "title" ? "title" : phase === "over" ? "result" : "hud")
        if (phase === "paused")
            showScreen("pause")
        const target = game.enemies.find((e) => e.id === foe) ?? game.enemies.find((e) => e.kind === Rules.WARDEN && e.awake)
        if (target && (phase === "playing" || phase === "paused")) {
            showScreen("foe")
            text("Foe", Rules.KINDS[target.kind].name.toUpperCase())
            bar("Foe health", target.health, target.most)
        }
        text("Score", "SCORE " + counted(game.score))
        text("Gold", "GOLD " + counted(game.gold))
        bar("Health", game.health, game.most)
        text("Health words", game.health + " / " + game.most)
        bar("Fury", game.fury, Rules.FURY_MOST)
        text("Potions", "POTIONS " + game.potions + "  ·  RELICS " + game.relics.reduce((a, b) => a + b, 0))
        text("Depth", "FLOOR " + game.floor + " OF " + Rules.FLOORS)
        text("Level", "LEVEL " + game.level + "  ·  " + game.xp + " / " + Rules.nextXp(game.level))
        if (phase === "over") {
            text("Result", game.cleared ? "RIFT CLEARED" : game.health <= 0 ? "FALLEN" : "THE RIFT CLOSED")
            text("Final", "SCORE " + counted(game.score))
            text("Details", "Floor " + game.floor + "  ·  level " + game.level + "  ·  " + game.kills + " fallen  ·  "
                 + counted(game.gold) + " gold")
        }
    }

    /// The floor's rock as blocks: each row's runs of rock, carried down while the next row has the same run.
    function build() {
        for (const old of rock)
            remove(old)
        const made = []
        const block = (c0, c1, r0, r1) => {
            const w = c1 - c0 + 1, h = r1 - r0 + 1
            made.push(spawn("Shape", {
                name: "Rock " + made.length, form: "cube", body: "none",
                position: Qt.vector3d(c0 + w / 2, rockHeight / 2, r0 + h / 2),
                scale: Qt.vector3d(w, rockHeight, h), colour: "#5b6470", metalness: 0, roughness: 0.92,
                image: "../assets/rock.png", imageSize: 2, normalMap: "../assets/rock-relief.png",
                emissive: "#000000", emissivePower: 0
            }, entity("Dungeon")))
        }
        let open = {}
        for (let r = 0; r <= Rules.ROWS; r++) {
            const runs = {}
            if (r < Rules.ROWS) {
                for (let c = 0; c < Rules.COLS; c++) {
                    if (!Rules.rock(game, c, r))
                        continue
                    let end = c
                    while (end + 1 < Rules.COLS && Rules.rock(game, end + 1, r))
                        end += 1
                    runs[c + ":" + end] = { c0: c, c1: end }
                    c = end
                }
            }
            const next = {}
            for (const key in open) {
                if (runs[key])
                    next[key] = open[key]
                else
                    block(open[key].c0, open[key].c1, open[key].r0, r - 1)
            }
            for (const key in runs) {
                if (!next[key])
                    next[key] = { c0: runs[key].c0, c1: runs[key].c1, r0: r }
            }
            open = next
        }
        rock = made
        carved = game.carved
        assigned = ({})
        for (const name in pools) {
            for (const p of pools[name]) {
                p.position = away
                p.visible = false
            }
        }
        const stairs = entity("Stairs")
        const motes = entity("Rift motes")
        if (stairs)
            stairs.position = game.stairs ? at(game.stairs.x, game.stairs.y, 0.02) : away
        if (motes)
            motes.position = game.stairs ? at(game.stairs.x, game.stairs.y, 0.1) : away
        eyeX = game.x
        eyeY = game.y
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
                list[was[id]].position = away
                list[was[id]].visible = false
            }
        }
        assigned[name] = now
    }

    /// Degrees about the floor's normal that turn a piece's +z toward (fx, fy).
    function toward(fx, fy) {
        return Math.atan2(fx, fy) * 180 / Math.PI
    }

    onStarted: {
        game = Rules.create(scene.playSeed)
        const p = {}
        p.husk = pool("Husk", 32, { form: "cylinder", scale: Qt.vector3d(0.42, 0.62, 0.42), colour: "#b9ad94",
                                    metalness: 0.1, roughness: 0.7, emissive: "#74cdd3", emissivePower: 0.15 })
        p.wisp = pool("Wisp", 24, { form: "sphere", scale: Qt.vector3d(0.3, 0.3, 0.3), colour: "#0e2a2e",
                                    metalness: 0.2, roughness: 0.3, emissive: "#74cdd3", emissivePower: 0.8 })
        p.brute = pool("Brute", 16, { form: "cube", scale: Qt.vector3d(0.8, 0.8, 0.8), colour: "#3a2a24",
                                      metalness: 0.6, roughness: 0.45, emissive: "#ff7a3d", emissivePower: 0.25 })
        p.seer = pool("Seer", 16, { form: "cone", scale: Qt.vector3d(0.42, 0.78, 0.42), colour: "#2b1a3d",
                                    metalness: 0.3, roughness: 0.4, emissive: "#b07cff", emissivePower: 0.5 })
        p.warden = pool("Warden", 1, { form: "cylinder", scale: Qt.vector3d(1.5, 1.25, 1.5), colour: "#2a0d0d",
                                       metalness: 0.7, roughness: 0.35, emissive: "#ff4a2a", emissivePower: 0.35 })
        p.bolts = pool("Bolt", Rules.MOST_BOLTS, { form: "sphere", scale: Qt.vector3d(0.16, 0.16, 0.16),
                                                    colour: "#e9d5ff", metalness: 0, roughness: 0.4,
                                                    emissive: "#c79bff", emissivePower: 0.9 })
        p.gold = pool("Gold", Rules.MOST_ITEMS, { form: "cylinder", scale: Qt.vector3d(0.24, 0.04, 0.24),
                                                   eulerRotation: Qt.vector3d(90, 0, 0), colour: "#fbd68d",
                                                   metalness: 0.9, roughness: 0.2, emissive: "#ffb83a",
                                                   emissivePower: 0.45 })
        p.potions = pool("Potion", 12, { form: "sphere", scale: Qt.vector3d(0.2, 0.2, 0.2), colour: "#5a0d12",
                                         metalness: 0.1, roughness: 0.15, emissive: "#ff3b4a", emissivePower: 0.7 })
        p.relics = pool("Relic", 8, { form: "cube", scale: Qt.vector3d(0.22, 0.22, 0.22),
                                      eulerRotation: Qt.vector3d(45, 0, 45), colour: "#fff1c9", metalness: 0.8,
                                      roughness: 0.2, emissive: "#ffd27a", emissivePower: 0.8 })
        pools = p
        for (const name of ["Play", "Resume", "Play again"])
            button(name)
        show()
    }

    // W, A, S and D, the arrows, the pad's d-pad or left stick, or the buttons under a finger move; Space, J or the pad's
    // A strikes; K, E or X whirls; L, Shift or B dashes; Q, H or Y drinks a potion; P or Start pauses.
    onPressed: (action) => {
        Rules.press(game, action)
        show()
    }
    onReleased: (action) => Rules.release(game, action)

    onStep: {
        const before = game.phase
        const health = game.health
        const floor = game.floor
        const level = game.level
        const gold = game.gold
        const kills = game.kills
        Rules.step(game)
        const hero = heardAt(game.x, game.y)
        const heard = new Set()
        // One sound of each kind a step, however many happened, from the first place it happened.
        for (const s of game.said) {
            if (heard.has(s.what))
                continue
            heard.add(s.what)
            const there = s.x !== undefined ? heardAt(s.x, s.y) : hero
            if (s.what === "swing")
                play("../assets/swing.wav", { volume: 0.35, at: hero })
            else if (s.what === "hit" || s.what === "crit") {
                play(s.what === "crit" ? "../assets/crit.wav" : "../assets/hit.wav", { volume: 0.55, at: there })
                foe = s.id
            } else if (s.what === "kill")
                play(s.kind === Rules.WARDEN ? "../assets/slam.wav" : "../assets/kill.wav", { volume: 0.6, at: there })
            else if (s.what === "hurt")
                play("../assets/hurt.wav", { volume: 0.8, at: hero })
            else if (s.what === "whirl")
                play("../assets/whirl.wav", { volume: 0.6, at: hero })
            else if (s.what === "dash")
                play("../assets/dash.wav", { volume: 0.5, at: hero })
            else if (s.what === "potion")
                play("../assets/potion.wav", { volume: 0.6, at: hero })
            else if (s.what === "pick") {
                play(s.item === "relic" ? "../assets/relic.wav" : "../assets/coin.wav", { volume: 0.45, at: hero })
                if (s.item === "relic")
                    say(Rules.RELICS[s.relic].toUpperCase() + " RELIC", "#ffd27a")
            } else if (s.what === "bolt")
                play("../assets/bolt.wav", { volume: 0.45, at: there })
            else if (s.what === "rising")
                play("../assets/rising.wav", { volume: 0.7, at: there })
            else if (s.what === "slam")
                play("../assets/slam.wav", { volume: 0.9, at: there })
            else if (s.what === "summon")
                play("../assets/rising.wav", { volume: 0.4, at: there })
            else if (s.what === "warden") {
                play("../assets/warden.wav", { volume: 0.8 })
                say("THE WARDEN WAKES", "#ff7a5c")
            } else if (s.what === "warden down")
                say("THE WARDEN IS BROKEN", "#ead39a")
        }
        if (game.level > level) {
            play("../assets/level.wav", { volume: 0.7 })
            say("LEVEL " + game.level, "#ead39a")
        }
        if (game.floor > floor) {
            play("../assets/descend.wav", { volume: 0.8 })
            say("FLOOR " + game.floor, "#74cdd3")
        }
        if (before !== "over" && game.phase === "over")
            play(game.cleared ? "../assets/cleared.wav" : "../assets/fallen.wav", { volume: 0.9 })
        if (!game.enemies.some((e) => e.id === foe))
            foe = -1
        if (before !== game.phase || health !== game.health || floor !== game.floor || gold !== game.gold
                || kills !== game.kills || game.steps % 6 === 0)
            show()
    }

    // Only looked at: everything where the rules put it.
    onFrame: (dt) => {
        if (game.carved !== carved)
            build()
        if (!Look.reducedMotion)
            turn = (turn + 90 * dt) % 360
        const spin = Look.reducedMotion ? 0 : turn
        const follow = Look.reducedMotion ? 1 : Math.min(1, dt * 7)
        eyeX += (game.x - eyeX) * follow
        eyeY += (game.y - eyeY) * follow
        const dungeon = entity("Dungeon")
        if (dungeon)
            dungeon.position = Qt.vector3d(-eyeX * k, 0, -eyeY * k)

        // The delver, its torch, and its blade: held to one side, swept across the arc in a swing, and round it in a
        // whirl.
        const facing = toward(game.facing[0], game.facing[1])
        const body = entity("Delver")
        if (body) {
            body.position = at(game.x, game.y, 0.32)
            body.eulerRotation = Qt.vector3d(0, facing, 0)
            const reeling = game.hurt > 0 && (Look.reducedMotion || game.hurt % 8 < 4)
            body.emissive = reeling ? "#ff3b2f" : game.dash > 0 ? "#cffaff" : "#ffd08a"
            body.emissivePower = reeling ? 1 : game.dash > 0 ? 0.9 : 0.35
        }
        const halo = entity("Halo")
        if (halo)
            halo.position = at(game.x, game.y, 0.02)
        const torch = entity("Torch")
        if (torch)
            torch.position = at(game.x, game.y, 2.2)
        const blade = entity("Blade")
        if (blade) {
            let angle = facing + 50
            if (game.whirl > 0)
                angle = facing + (Rules.WHIRL_STEPS - game.whirl) * 45
            else if (game.swing > 0)
                angle = facing - 70 + 140 * Math.min(1, (Rules.SWING_STEPS - game.swing) / (Rules.SWING_STEPS - Rules.SWING_STRIKES))
            const a = angle * Math.PI / 180
            blade.position = at(game.x + Math.sin(a) * 22, game.y + Math.cos(a) * 22, 0.42)
            blade.eulerRotation = Qt.vector3d(0, angle, 0)
            blade.emissivePower = game.swing > 0 || game.whirl > 0 ? 0.9 : 0.35
        }
        const ring = entity("Whirl")
        if (ring) {
            ring.position = game.whirl > 0 ? at(game.x, game.y, 0.03) : away
            ring.eulerRotation = Qt.vector3d(0, spin * 4, 0)
        }

        // Enemies: a blow wound up glows red, brighter as it comes; one struck flashes white.
        const look = (base, power) => (p, e) => {
            const k2 = Rules.KINDS[e.kind]
            const winding = e.act === "windup"
            p.emissive = e.struck > 0 ? "#ffffff" : winding ? "#ff3b2f" : base
            p.emissivePower = e.struck > 0 ? 1 : winding ? 0.4 + 0.6 * (1 - e.timer / k2.windup) : power
            p.eulerRotation.y = toward(e.fx, e.fy)
        }
        const kind = (n) => game.enemies.filter((e) => e.kind === n)
        draw("husk", kind(Rules.HUSK), (e) => at(e.x, e.y, 0.31), look("#74cdd3", 0.15))
        draw("wisp", kind(Rules.WISP), (e) => at(e.x, e.y, 0.45 + (Look.reducedMotion ? 0 : 0.08 * Math.sin((turn * 4 + e.id * 40) * Math.PI / 180))),
             look("#74cdd3", 0.8))
        draw("brute", kind(Rules.BRUTE), (e) => at(e.x, e.y, 0.4), look("#ff7a3d", 0.25))
        draw("seer", kind(Rules.SEER), (e) => at(e.x, e.y, 0.39), look("#b07cff", 0.5))
        draw("warden", kind(Rules.WARDEN), (e) => at(e.x, e.y, 0.63), look("#ff4a2a", 0.35))
        const slam = entity("Slam")
        const winding = game.enemies.find((e) => e.kind === Rules.WARDEN && e.act === "windup")
        if (slam) {
            if (winding) {
                const grown = 1 - winding.timer / Rules.KINDS[Rules.WARDEN].windup
                const across = 2 * (Rules.KINDS[Rules.WARDEN].reach + Rules.HERO_R) * k * Math.max(0.1, grown)
                slam.position = at(winding.x, winding.y, 0.02)
                slam.scale = Qt.vector3d(across, 0.01, across)
                slam.emissivePower = 0.3 + 0.6 * grown
            } else {
                slam.position = away
            }
        }
        draw("bolts", game.bolts, (b) => at(b.x, b.y, 0.45))
        draw("gold", game.items.filter((i) => i.what === "gold"), (i) => at(i.x, i.y, 0.14),
             (p) => p.eulerRotation = Qt.vector3d(90, 0, spin * 2))
        draw("potions", game.items.filter((i) => i.what === "potion"), (i) => at(i.x, i.y, 0.12))
        draw("relics", game.items.filter((i) => i.what === "relic"), (i) => at(i.x, i.y, 0.3),
             (p, i) => {
                 p.emissive = ["#ff6a4a", "#7dffa6", "#74cdd3", "#ffb83a"][i.relic]
                 p.eulerRotation = Qt.vector3d(45, spin * 2, 45)
             })
        const stairs = entity("Stairs")
        if (stairs && game.stairs)
            stairs.eulerRotation = Qt.vector3d(0, spin, 0)
    }
}
