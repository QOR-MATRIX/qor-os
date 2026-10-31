# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Lays out Gloamreach's valley and writes it where each part of the game reads it, the same bytes every time.

The map, its places and its camps go between the markers in logic/rules.mjs, because the rules decide what can be walked
on and ARQADE's service replays them. How high the land stands goes to logic/land.mjs, because only the drawing asks.
make_assets.py then builds the land's meshes and pictures from the same layout. Standard library only:

    python make_world.py
"""
import json
import pathlib
import re

HERE = pathlib.Path(__file__).parent
COLS = 64
ROWS = 64

GRASS, PATH, MIRE, ASH, SNOW, BRIDGE, TREE, ROCK, WATER, WALL, GATE = ".,:;'=TR~#G"
BLOCKED = {TREE, ROCK, WATER, WALL, GATE}

# Where things stand, in tiles (column, row): the middle of each place.
PLACES = {
    "hearth": [30, 47],
    "elder": [30, 44],
    "healer": [25, 48],
    "smith": [36, 47],
    "hunter": [31, 53],
    "crossroads": [31, 35],
    "den": [8, 30],
    "mireBeacon": [10, 53],
    "ashBeacon": [55, 33],
    "highBeacon": [13, 12],
    "gate": [44, 20],
    "crown": [47, 11],
}

# The camps: what waits where. Each is [column, row, region, how many, the kinds it may hold].
# Kinds are the rules' numbers: 0 wolf, 1 shade, 2 thornling, 3 revenant, 4 bonecaster. Great ones are placed by name.
CAMPS = [
    # The meadow, for the first errand.
    [24, 38, "meadow", 4, [0]],
    [39, 40, "meadow", 4, [0]],
    [38, 30, "meadow", 3, [0, 1]],
    [22, 33, "meadow", 3, [0, 1]],
    # The Mirewood.
    [20, 46, "mire", 6, [2, 1]],
    [15, 40, "mire", 6, [2, 1]],
    [8, 45, "mire", 5, [2]],
    [17, 53, "mire", 6, [2, 1]],
    # The Ashen Hold.
    [46, 39, "ash", 6, [3]],
    [52, 44, "ash", 6, [3, 4]],
    [57, 40, "ash", 6, [3, 4]],
    [48, 31, "ash", 6, [3, 4]],
    # The High Pass.
    [17, 22, "high", 6, [0, 1, 4]],
    [8, 20, "high", 6, [1, 3]],
    [24, 13, "high", 7, [3, 4]],
    [18, 7, "high", 6, [1, 3, 4]],
    # The way to the Crown.
    [44, 25, "crown", 6, [1, 3, 4]],
    [37, 20, "crown", 6, [3, 4]],
]

# The village's houses: the middle of each.
HOUSES = [[24, 44], [35, 44], [23, 50], [37, 50], [30, 51]]

# What the errands ask to be found: moonherbs in the Mirewood, ironroot in the Ashen Hold.
HERBS = [[12, 37], [19, 50], [6, 49], [14, 57], [20, 38], [18, 42], [11, 47]]
ORE = [[47, 45], [55, 46], [59, 36], [50, 28], [43, 35], [54, 38]]


def noise(c, r, salt=0):
    """A whole number from 0 to 9999, the same for the same tile and salt on any machine."""
    x = (c * 374761393 + r * 668265263 + salt * 2147483647) & 0xFFFFFFFF
    x = ((x ^ (x >> 13)) * 1274126177) & 0xFFFFFFFF
    x = x ^ (x >> 16)
    return x % 10000


def inside(c, r, cx, cy, across):
    """Whether a tile lies within `across` tiles of a place, by the square of the distance, so in whole numbers."""
    return (c - cx) * (c - cx) + (r - cy) * (r - cy) <= across * across


def lay_out():
    """The valley, a list of rows of tiles."""
    grid = [[GRASS] * COLS for _ in range(ROWS)]

    def put(c, r, tile):
        if 0 <= c < COLS and 0 <= r < ROWS:
            grid[r][c] = tile

    def clear(cx, cy, across, tile=None):
        """Nothing that blocks within `across` tiles of a place; `tile` underfoot if given."""
        for r in range(cy - across, cy + across + 1):
            for c in range(cx - across, cx + across + 1):
                if inside(c, r, cx, cy, across) and 0 <= c < COLS and 0 <= r < ROWS:
                    if tile is not None:
                        grid[r][c] = tile
                    elif grid[r][c] in BLOCKED:
                        grid[r][c] = GRASS

    # The regions underfoot.
    for r in range(ROWS):
        for c in range(COLS):
            wobble = noise(c, r, 1) % 3 - 1
            if inside(c, r, 14, 45, 14 + wobble):
                grid[r][c] = MIRE
            elif inside(c, r, 51, 38, 13 + wobble):
                grid[r][c] = ASH
            elif inside(c, r, 16, 13, 16 + wobble) or inside(c, r, 37, 12, 12 + wobble):
                grid[r][c] = SNOW
            elif inside(c, r, 47, 11, 11 + wobble):
                grid[r][c] = ASH

    # What grows and stands on them.
    for r in range(ROWS):
        for c in range(COLS):
            tile = grid[r][c]
            roll = noise(c, r, 2)
            if tile == MIRE and roll < 2600:
                grid[r][c] = TREE
            elif tile == MIRE and roll < 2950:
                grid[r][c] = WATER
            elif tile == SNOW and roll < 900:
                grid[r][c] = TREE
            elif tile == SNOW and roll < 2000:
                grid[r][c] = ROCK
            elif tile == ASH and roll < 400:
                grid[r][c] = TREE
            elif tile == ASH and roll < 1000:
                grid[r][c] = ROCK
            elif tile == GRASS and roll < 750:
                grid[r][c] = TREE
            elif tile == GRASS and roll < 900:
                grid[r][c] = ROCK

    # The lake, and the river that runs from it to the east wall, with two bridges on the paths below.
    for r in range(ROWS):
        for c in range(COLS):
            dx, dy = c - 30, (r - 25) * 3
            if dx * dx + dy * dy // 2 <= 60:
                grid[r][c] = WATER
    river = [(38, 26), (40, 27), (42, 28), (44, 27), (46, 27), (48, 26), (50, 25), (52, 24), (55, 23), (58, 22), (61, 22)]
    for (ax, ay), (bx, by) in zip(river, river[1:]):
        steps = max(abs(bx - ax), abs(by - ay))
        for i in range(steps + 1):
            x, y = ax + (bx - ax) * i // steps, ay + (by - ay) * i // steps
            put(x, y, WATER)
            put(x, y + 1, WATER)

    # The Ashen Hold's broken walls.
    for c in range(44, 59):
        if noise(c, 1, 3) % 5:
            put(c, 34, WALL)
        if noise(c, 2, 3) % 5:
            put(c, 47, WALL)
    for r in range(34, 48):
        if noise(1, r, 3) % 5:
            put(44, r, WALL)
        if noise(2, r, 3) % 5:
            put(59, r, WALL)

    # The valley's rock, thick and uneven at its edges.
    for r in range(ROWS):
        for c in range(COLS):
            edge = min(c, r, COLS - 1 - c, ROWS - 1 - r)
            if edge < 2 + noise(c, r, 4) % 2:
                grid[r][c] = ROCK

    # The Crown's ring of rock, with its one gate to the south-west.
    for r in range(ROWS):
        for c in range(COLS):
            d = (c - 47) * (c - 47) + (r - 11) * (r - 11)
            if 49 <= d <= 81:
                grid[r][c] = ROCK
    clear(47, 11, 6, ASH)

    # The ways between the places, two tiles wide, bridged over water.
    def way(points):
        for (ax, ay), (bx, by) in zip(points, points[1:]):
            steps = max(abs(bx - ax), abs(by - ay), 1)
            for i in range(steps + 1):
                x, y = ax + (bx - ax) * i // steps, ay + (by - ay) * i // steps
                for ox, oy in ((0, 0), (1, 0), (0, 1), (1, 1)):
                    nx, ny = x + ox, y + oy
                    if 0 <= nx < COLS and 0 <= ny < ROWS:
                        grid[ny][nx] = BRIDGE if grid[ny][nx] == WATER else PATH

    way([PLACES["hearth"], PLACES["crossroads"]])
    way([PLACES["crossroads"], [21, 41], [14, 47], PLACES["mireBeacon"]])
    way([PLACES["crossroads"], [45, 37], [52, 37], PLACES["ashBeacon"]])
    way([PLACES["crossroads"], [20, 33], [16, 24], PLACES["highBeacon"]])
    way([PLACES["crossroads"], [41, 31], [44, 25], PLACES["gate"]])
    way([[20, 33], PLACES["den"]])

    # The village, its clearing and its houses; each beacon, gate and camp in a clearing of its own.
    clear(30, 47, 8, GRASS)
    way([PLACES["hearth"], PLACES["crossroads"]])
    for name in ("mireBeacon", "ashBeacon", "highBeacon", "den"):
        cx, cy = PLACES[name]
        clear(cx, cy, 4)
    for c, r, *_ in CAMPS:
        clear(c, r, 3)
    for c, r in HERBS + ORE:
        clear(c, r, 1)
    for cx, cy in HOUSES:
        for r in range(cy - 1, cy + 1):
            for c in range(cx - 1, cx + 2):
                put(c, r, WALL)

    # The gate, in the ring where the way to it arrives.
    gate = PLACES["gate"]
    way([gate, [46, 16]])
    clear(47, 11, 6, ASH)
    for c in range(43, 49):
        for r in range(16, 20):
            d = (c - 47) * (c - 47) + (r - 11) * (r - 11)
            if 49 <= d <= 81 and grid[r][c] == PATH:
                grid[r][c] = GATE

    check(grid)
    return ["".join(row) for row in grid]


def check(grid):
    """Every place and camp can be walked to from the village, and the gate is what shuts the Crown."""

    def walk(open_tile):
        start = PLACES["hearth"]
        seen = {tuple(start)}
        queue = [tuple(start)]
        for c, r in queue:
            for nc, nr in ((c + 1, r), (c - 1, r), (c, r + 1), (c, r - 1)):
                if (nc, nr) not in seen and 0 <= nc < COLS and 0 <= nr < ROWS and open_tile(grid[nr][nc]):
                    seen.add((nc, nr))
                    queue.append((nc, nr))
        return seen

    shut = walk(lambda t: t not in BLOCKED)
    opened = walk(lambda t: t not in BLOCKED or t == GATE)
    for name, place in PLACES.items():
        place = tuple(place)
        if name == "crown":
            assert place not in shut, "the Crown can be walked to with its gate shut"
            assert place in opened, "the Crown cannot be walked to with its gate open"
        elif name in ("elder", "healer", "smith", "hunter"):
            continue
        else:
            assert place in shut, f"{name} cannot be walked to"
    for c, r, *_ in CAMPS:
        assert (c, r) in shut, f"the camp at {c},{r} cannot be walked to"
    for c, r in HERBS + ORE:
        assert (c, r) in shut, f"what is to be found at {c},{r} cannot be walked to"
    for name in ("elder", "healer", "smith", "hunter"):
        c, r = PLACES[name]
        assert grid[r][c] not in BLOCKED, f"{name} stands in something"


def smooth_noise(x, y, salt):
    """Value noise over whole tiles, from 0 to 1."""
    c, r = int(x), int(y)
    fx, fy = x - c, y - r
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a, b = noise(c, r, salt) / 9999, noise(c + 1, r, salt) / 9999
    e, f = noise(c, r + 1, salt) / 9999, noise(c + 1, r + 1, salt) / 9999
    return (a + (b - a) * fx) * (1 - fy) + (e + (f - e) * fx) * fy


def heights(rows):
    """How high the land stands at each tile's corner, in hundredths of a tile: rolling ground, rock raised, and every
    corner that touches water at one level, under the water's one surface, whatever the climb around it. A bridge's stay
    just over it. Only the drawing asks this. The rules decide what can be walked on by the tile alone."""
    raised = {ROCK: 2.2, WALL: 0, TREE: 0.2, PATH: 0, GATE: 0.1}
    land = []
    for r in range(ROWS + 1):
        for c in range(COLS + 1):
            around = [rows[y][x] for y in (r - 1, r) for x in (c - 1, c) if 0 <= x < COLS and 0 <= y < ROWS]
            base = sum(raised.get(t, 0.0) for t in around) / len(around)
            rolling = (smooth_noise(c / 7, r / 7, 5) - 0.5) * 1.1 + (smooth_noise(c / 3, r / 3, 6) - 0.5) * 0.35
            # High ground to the north, low to the south-west, so the valley reads as a climb.
            climb = (ROWS - r) / ROWS * 1.4
            if any(t in (PATH, WALL) for t in around):
                rolling *= 0.5
            height = base + rolling + climb
            if any(t == BRIDGE for t in around):
                height = 0.1
            elif any(t == WATER for t in around):
                height = -0.55
            else:
                height = max(height, 0.1)
            # The rim rises into mountains, so the valley's edge is never seen.
            edge = min(c, r, COLS - c, ROWS - r)
            if edge < 5:
                height += (5 - edge) * 2.4 * (0.75 + 0.5 * smooth_noise(c / 4, r / 4, 7))
            land.append(round(height * 100))
    return land


def write():
    rows = lay_out()
    block = "// WORLD BEGIN: written by make_world.py; change it there.\n"
    block += "export const WORLD = [\n" + "".join(f'    "{row}",\n' for row in rows) + "]\n"
    block += "export const PLACES = " + json.dumps(PLACES) + "\n"
    block += "export const CAMPS = " + json.dumps(CAMPS, separators=(", ", ": ")) + "\n"
    block += "export const HERBS = " + json.dumps(HERBS) + "\n"
    block += "export const ORE = " + json.dumps(ORE) + "\n"
    block += "export const HOUSES = " + json.dumps(HOUSES) + "\n"
    block += "// WORLD END\n"

    rules = HERE / "logic" / "rules.mjs"
    text = rules.read_text(encoding="utf-8")
    text, count = re.subn(r"// WORLD BEGIN.*?// WORLD END\n", lambda _: block, text, flags=re.S)
    assert count == 1, "logic/rules.mjs has no world markers"
    rules.write_bytes(text.encode("utf-8"))

    land = heights(rows)
    out = ("// SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary\n"
           "// How high Gloamreach's land stands, written by make_world.py. Only the drawing asks: what can be walked on is\n"
           "// the rules' alone. A corner of a tile each, row by row, in hundredths of a tile.\n"
           f"export const ACROSS = {COLS + 1}\n"
           "export const CORNERS = [\n")
    for r in range(ROWS + 1):
        out += "    " + ",".join(str(h) for h in land[r * (COLS + 1) : (r + 1) * (COLS + 1)]) + ",\n"
    out += "]\n"
    (HERE / "logic" / "land.mjs").write_bytes(out.encode("utf-8"))
    return rows, land


if __name__ == "__main__":
    laid, _ = write()
    print("\n".join(laid))
