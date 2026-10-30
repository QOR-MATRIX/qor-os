// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
// Rift Delve's pilot for tst_games: what a person would do next, read from the rules' game as it stands, as
// products/arqade/tests/rift-delve.test.mjs's pilot chooses. tst_games evaluates this in the game's own engine, calls
// the function it gives with the game, and plays what it says by the keys, the pad and the screen's buttons.
//
// It drinks when wounded below half, whirls among three enemies, dashes away from the warden's slam, strikes at what
// is near, and otherwise heads for the nearest enemy awake, what lies on the floor once none is, or the stairs once
// strong enough for the floor below, finding its way along the floor's tiles. Calm, it never strikes or uses a skill,
// and walks to the nearest enemy, awake or not.
(function () {
    const TILE = 40, COLS = 44, ROWS = 32, SWING_REACH = 56, WHIRL_COST = 35, POTIONS_MOST = 5, WARDEN = 4
    const RADIUS = [13, 10, 20, 12, 30]
    const DIRS = [[1, 0], [-1, 0], [0, 1], [0, -1], [1, 1], [1, -1], [-1, 1], [-1, -1]]
    const rock = (g, c, r) => c < 0 || r < 0 || c >= COLS || r >= ROWS || g.map[r * COLS + c] === 0
    const apart = (a, b) => Math.sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y))
    let ways = { key: "", far: null }

    function walking(g, tc, tr) {
        const far = new Array(COLS * ROWS).fill(-1)
        if (rock(g, tc, tr))
            return far
        const queue = [tr * COLS + tc]
        far[queue[0]] = 0
        for (let head = 0; head < queue.length; head++) {
            const at = queue[head], c = at % COLS, r = (at - c) / COLS
            for (const [dc, dr] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
                const nc = c + dc, nr = r + dr
                if (!rock(g, nc, nr) && far[nr * COLS + nc] < 0) {
                    far[nr * COLS + nc] = far[at] + 1
                    queue.push(nr * COLS + nc)
                }
            }
        }
        return far
    }

    // The middle of the next tile on the way to (x, y).
    function nextTo(g, x, y) {
        const tc = Math.floor(x / TILE), tr = Math.floor(y / TILE), key = g.carved + ":" + tc + "," + tr
        if (ways.key !== key)
            ways = { key: key, far: walking(g, tc, tr) }
        const c = Math.floor(g.x / TILE), r = Math.floor(g.y / TILE), here = ways.far[r * COLS + c]
        if (here <= 1)
            return [x, y]
        let best = here, to = [x, y]
        for (const [dc, dr] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
            const v = ways.far[(r + dr) * COLS + c + dc]
            if (!rock(g, c + dc, r + dr) && v >= 0 && v < best) {
                best = v
                to = [(c + dc + 0.5) * TILE, (r + dr + 0.5) * TILE]
            }
        }
        return to
    }

    function wayTo(g, x, y) {
        const dx = x - g.x, dy = y - g.y
        if (dx * dx + dy * dy < 9)
            return [0, 0]
        let best = [0, 0], score = -Infinity
        for (const [a, b] of DIRS) {
            const s = (a * dx + b * dy) * (a && b ? 0.7071 : 1)
            if (s > score) {
                score = s
                best = [a, b]
            }
        }
        return best
    }

    return function (g, calm) {
        const out = { way: [0, 0], attack: false, whirl: false, dash: false, potion: false }
        const nearest = (list) => list.reduce((a, b) => apart(a, g) < apart(b, g) ? a : b)
        if (calm) {
            if (g.enemies.length)
                out.way = wayTo(g, ...nextTo(g, nearest(g.enemies).x, nearest(g.enemies).y))
            return out
        }
        const awake = g.enemies.filter((e) => e.awake)
        const close = awake.filter((e) => apart(e, g) < SWING_REACH + RADIUS[e.kind] - 4)
        out.potion = g.health < g.most * 0.45 && g.potions > 0 && g.potionCool === 0
        out.whirl = close.length >= 3 && g.fury >= WHIRL_COST && g.whirl === 0
        const slam = awake.find((e) => e.kind === WARDEN && e.act === "windup" && apart(e, g) < 130)
        if (slam && g.dashCool === 0) {
            out.dash = true
            out.way = wayTo(g, 2 * g.x - slam.x, 2 * g.y - slam.y)
            return out
        }
        if (close.length) {
            out.attack = true
            return out
        }
        const items = g.items.filter((i) => (i.what !== "potion" || g.potions < POTIONS_MOST) && apart(i, g) < 220)
        let goal = null
        if (items.length && !awake.length)
            goal = nearest(items)
        else if (awake.length)
            goal = nearest(awake)
        else if (g.enemies.length && (!g.stairs || g.level < g.floor + 1))
            goal = nearest(g.enemies)
        else if (g.stairs)
            goal = g.stairs
        if (goal)
            out.way = wayTo(g, ...nextTo(g, goal.x, goal.y))
        return out
    }
})()
