# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Writes Gloamreach's models, sky and sounds into assets/ (QOR-0017), from the valley make_world.py lays out. Standard
library only, the same bytes every time: python make_assets.py

  land.glb            the valley's ground: a corner of a mesh for each tile's corner, as high as logic/land.mjs says, under
                      one painted picture of grass, paths, mire, ash and snow, blended from tile to tile.
  scenery-N.glb       four quarters of what stands on it, flat-shaded from one small palette: trees for each region, boulders,
                      broken walls, houses, bridges, the beacons' plinths, the gate's posts and the king's dais. Each quarter
                      is sectors of 8 by 8 tiles, Sector_C_R, so the logic can hide what the eye cannot see.
  hero.glb, elder.glb, healer.glb, smith.glb, hunter.glb, wolf.glb, shade.glb, thornling.glb, revenant.glb, bonecaster.glb,
  warden.glb, king.glb
                      the people and what they fight: boxes and tapers bound to joints, each with its clips (idle, walk,
                      strike, swing1, swing2, swing3, roll, cast, hurt, fall), which QQ plays on the game's clock.
  gloam.hdr           256 x 128, an equirectangular HDR sky that lights the valley: dusk, low and red in the south-west, with
                      the first stars. Run-length encoded, as tools write Radiance files.
  swing, hit, crit, kill, hurt, roll, potion, coin, cast, bolt, burst, thorn, curse, rising, slam, herb .wav    from a place
  wake, warden, king, level, beacon, gate, talk, yes, no, won, lost, fallen .wav                     heard everywhere
  gloam.wav           16 s, the music, looping: a low drone, a slow pulse, and a far-off pipe.

A sound the game plays from a place is written in one channel, as Qt's spatial sounds take it, and one heard everywhere in
both, as its ambient sounds take it. The noise is a fixed sequence, so the files are the same bytes each time.
"""
import json
import math
import pathlib
import struct
import wave
import zlib

import make_world
from make_world import BLOCKED, COLS, HOUSES, PLACES, ROCK, ROWS, TREE, WALL, WATER, BRIDGE, GATE, PATH, MIRE, ASH, SNOW, GRASS

HERE = pathlib.Path(__file__).parent
ASSETS = HERE / "assets"
RATE = 22050

# The palette every model paints from: a cell of one small picture for each colour.
PALETTE = ["#c99a7a", "#3a2a20", "#39506b", "#5a4028", "#aeb7c0", "#20242c", "#c9a45c", "#8c2f2a",
           "#d6cfb8", "#5a6248", "#ff7a3d", "#1b1d2b", "#74cdd3", "#6a4a9c", "#3f5a32", "#4a3626",
           "#6d7a3a", "#4a4d55", "#e8e6e0", "#4d4f55", "#2c2d33", "#ffdd66", "#3f7a4a", "#54402e",
           "#2f4f8a", "#7d7f86", "#27351f", "#55606a", "#b6c6cf", "#3b2f2f", "#6f6240", "#6e6a64"]
SKIN, HAIR, TUNIC, LEATHER, STEEL, DARK, GOLD, RED, BONE, ROT, EMBER, GLOOM, TEAL, VIOLET, MOSS, BARK, THORN, IRON, WHITE, FUR, \
    FUR_DARK, EYE, GREEN, BROWN, BLUE, GREY, PINE, SLATE, FROST, ROOF, THATCH, STONE = range(32)
CELLS = 8
CELL = 8
# What stands on the valley comes in sectors this many tiles across.
SECTOR = 8


def hashed(a, b, salt=0):
    """A number from 0 to 1, the same for the same whole numbers on any machine."""
    x = (a * 374761393 + b * 668265263 + salt * 2147483647) & 0xFFFFFFFF
    x = ((x ^ (x >> 13)) * 1274126177) & 0xFFFFFFFF
    x = x ^ (x >> 16)
    return (x % 100000) / 99999


def png_bytes(w, h, pixel):
    """An 8-bit RGB PNG of `pixel(x, y)`, each channel from 0 to 1."""
    rows = bytearray()
    for y in range(h):
        rows.append(0)
        for x in range(w):
            rows.extend(max(0, min(255, int(round(c * 255)))) for c in pixel(x, y))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


def rgb(colour):
    return [int(colour[i:i + 2], 16) / 255 for i in (1, 3, 5)]


def palette_png():
    size = CELLS * CELL
    filled = PALETTE + ["#000000"] * (CELLS * CELLS - len(PALETTE))
    return png_bytes(size, size, lambda x, y: rgb(filled[(y // CELL) * CELLS + x // CELL]))


def uv_of(colour):
    return ((colour % CELLS + 0.5) / CELLS, (colour // CELLS + 0.5) / CELLS)


# glTF ------------------------------------------------------------------------------------------------------------------

FLOAT, UBYTE, UINT = 5126, 5121, 5125
ARRAY, ELEMENTS = 34962, 34963


class Gltf:
    """A glTF 2.0 file being put together, written as one binary .glb."""

    def __init__(self):
        self.blob = bytearray()
        self.views, self.accessors = [], []

    def view(self, data, target=None):
        while len(self.blob) % 4:
            self.blob.append(0)
        view = {"buffer": 0, "byteOffset": len(self.blob), "byteLength": len(data)}
        if target:
            view["target"] = target
        self.blob.extend(data)
        self.views.append(view)
        return len(self.views) - 1

    def add(self, data, kind, count, shape, target=None, minmax=None):
        accessor = {"bufferView": self.view(data, target), "componentType": kind, "count": count, "type": shape}
        if minmax:
            accessor["min"], accessor["max"] = minmax
        self.accessors.append(accessor)
        return len(self.accessors) - 1

    def floats(self, values, shape, width, target=None, bounded=False):
        minmax = None
        if bounded:
            minmax = ([min(values[i::width]) for i in range(width)], [max(values[i::width]) for i in range(width)])
        return self.add(struct.pack("<%df" % len(values), *values), FLOAT, len(values) // width, shape, target, minmax)

    def save(self, name, document):
        document = dict(document)
        document["asset"] = {"version": "2.0", "generator": "Gloamreach make_assets.py"}
        document["accessors"] = self.accessors
        document["bufferViews"] = self.views
        while len(self.blob) % 4:
            self.blob.append(0)
        document["buffers"] = [{"byteLength": len(self.blob)}]
        head = json.dumps(document, separators=(",", ":")).encode("utf-8")
        head += b" " * (-len(head) % 4)
        total = 12 + 8 + len(head) + 8 + len(self.blob)
        out = struct.pack("<III", 0x46546C67, 2, total)
        out += struct.pack("<II", len(head), 0x4E4F534A) + head
        out += struct.pack("<II", len(self.blob), 0x004E4942) + bytes(self.blob)
        (ASSETS / name).write_bytes(out)


def textured(gltf, picture, roughness=0.9):
    """The one material and texture a file paints from."""
    image = gltf.view(picture)
    return {"images": [{"bufferView": image, "mimeType": "image/png"}],
            "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 33071, "wrapT": 33071}],
            "textures": [{"source": 0, "sampler": 0}],
            "materials": [{"name": "Painted", "doubleSided": True,
                           "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}, "metallicFactor": 0.0,
                                                    "roughnessFactor": roughness}}]}


# Flat-shaded solids ----------------------------------------------------------------------------------------------------

def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


class Solid:
    """Triangles of one mesh, each face its own normal, each with a palette colour and, for a figure, a joint."""

    def __init__(self):
        self.points, self.normals, self.uvs, self.joints = [], [], [], []

    def blade(self, corners, colour):
        """A blade of grass: lit from above, as the ground under it is, whichever way it faces."""
        uv = uv_of(colour)
        for p in corners:
            self.points.extend(p)
            self.normals.extend((0.0, 1.0, 0.0))
            self.uvs.extend(uv)
            self.joints.extend((0, 0, 0, 0))

    def face(self, corners, colour, inside, joint=0):
        """A flat face, fanned from its first corner, its normal away from `inside`."""
        uv = uv_of(colour)
        a = corners[0]
        for i in range(1, len(corners) - 1):
            b, c = corners[i], corners[i + 1]
            n = cross(sub(b, a), sub(c, a))
            length = math.sqrt(dot(n, n))
            if length < 1e-9:
                continue
            n = (n[0] / length, n[1] / length, n[2] / length)
            if dot(n, sub(a, inside)) < 0:
                b, c = c, b
                n = (-n[0], -n[1], -n[2])
            for p in (a, b, c):
                self.points.extend(p)
                self.normals.extend(n)
                self.uvs.extend(uv)
                self.joints.extend((joint, 0, 0, 0))

    def box(self, centre, size, colour, joint=0, turn=0.0):
        cx, cy, cz = centre
        sx, sy, sz = size[0] / 2, size[1] / 2, size[2] / 2
        c, s = math.cos(turn), math.sin(turn)

        def at(dx, dy, dz):
            return (cx + dx * c + dz * s, cy + dy, cz - dx * s + dz * c)

        corners = [at(x, y, z) for x in (-sx, sx) for y in (-sy, sy) for z in (-sz, sz)]
        for a, b, d, e in ((0, 1, 3, 2), (4, 5, 7, 6), (0, 1, 5, 4), (2, 3, 7, 6), (0, 2, 6, 4), (1, 3, 7, 5)):
            self.face([corners[a], corners[b], corners[d], corners[e]], colour, centre, joint)

    def taper(self, base, r0, r1, height, sides, colour, joint=0, turn=0.0):
        """A cone (r1 = 0) or a cut cone, upright from `base`."""
        bx, by, bz = base
        inside = (bx, by + height / 2, bz)
        ring = [(math.cos(turn + 2 * math.pi * i / sides), math.sin(turn + 2 * math.pi * i / sides)) for i in range(sides)]
        for i in range(sides):
            (ax, az), (dx, dz) = ring[i], ring[(i + 1) % sides]
            low_a, low_b = (bx + ax * r0, by, bz + az * r0), (bx + dx * r0, by, bz + dz * r0)
            high_a, high_b = (bx + ax * r1, by + height, bz + az * r1), (bx + dx * r1, by + height, bz + dz * r1)
            self.face([low_a, low_b, high_b, high_a] if r1 > 0 else [low_a, low_b, high_a], colour, inside, joint)
        if r1 > 0:
            self.face([(bx + a * r1, by + height, bz + b * r1) for a, b in ring], colour, inside, joint)
        self.face([(bx + a * r0, by, bz + b * r0) for a, b in ring], colour, inside, joint)

    def blob(self, centre, radius, colour, seed, squash=1.0, joint=0):
        """A rough lump: an octahedron with its faces split, each corner pushed in or out by a little."""
        cx, cy, cz = centre

        def corner(x, y, z):
            length = math.sqrt(x * x + y * y + z * z)
            x, y, z = x / length, y / length, z / length
            k = radius * (0.8 + 0.35 * hashed(int(x * 90) + seed, int(y * 90) + int(z * 90) * 7, 9))
            return (cx + x * k, cy + y * k * squash, cz + z * k)

        axes = [(1, 0, 0), (0, 1, 0), (0, 0, 1), (-1, 0, 0), (0, -1, 0), (0, 0, -1)]
        for a, b, d in ((0, 1, 2), (2, 1, 3), (3, 1, 5), (5, 1, 0), (2, 4, 0), (3, 4, 2), (5, 4, 3), (0, 4, 5)):
            pa, pb, pd = axes[a], axes[b], axes[d]
            ab = tuple((u + v) / 2 for u, v in zip(pa, pb))
            bd = tuple((u + v) / 2 for u, v in zip(pb, pd))
            da = tuple((u + v) / 2 for u, v in zip(pd, pa))
            for tri in ((pa, ab, da), (ab, pb, bd), (da, bd, pd), (ab, bd, da)):
                self.face([corner(*tri[0]), corner(*tri[1]), corner(*tri[2])], colour, centre, joint)


# The land --------------------------------------------------------------------------------------------------------------

GROUND_COLOURS = {GRASS: "#46603a", PATH: "#7f6e52", MIRE: "#2f3d2b", ASH: "#4a4542", SNOW: "#c7ced4", BRIDGE: "#7f6e52",
                  TREE: "#3a5230", ROCK: "#5a5b60", WATER: "#26343a", WALL: "#55524d", GATE: "#4a4542"}


def paint_land(rows, size=1024):
    """The valley's ground as one picture: each tile's colour blended with its neighbours', flecked by a little noise."""
    per_tile = size / COLS
    colours = {tile: rgb(colour) for tile, colour in GROUND_COLOURS.items()}

    def tile_colour(c, r):
        c = max(0, min(COLS - 1, c))
        r = max(0, min(ROWS - 1, r))
        return colours[rows[r][c]]

    def pixel(x, y):
        u, v = (x + 0.5) / per_tile - 0.5, (y + 0.5) / per_tile - 0.5
        c, r = int(math.floor(u)), int(math.floor(v))
        fu, fv = u - c, v - r
        fu, fv = fu * fu * (3 - 2 * fu), fv * fv * (3 - 2 * fv)
        a, b, d, e = tile_colour(c, r), tile_colour(c + 1, r), tile_colour(c, r + 1), tile_colour(c + 1, r + 1)
        fleck = 0.88 + 0.08 * hashed(x // 2, y // 2, 1) + 0.08 * hashed(x // 7, y // 7, 2)
        return [((a[k] + (b[k] - a[k]) * fu) * (1 - fv) + (d[k] + (e[k] - d[k]) * fu) * fv) * fleck for k in range(3)]

    return png_bytes(size, size, pixel)


def write_land(rows, land):
    across = COLS + 1
    height = lambda c, r: land[r * across + c] / 100
    points, normals, uvs = [], [], []
    for r in range(ROWS + 1):
        for c in range(COLS + 1):
            points.extend((float(c), height(c, r), float(r)))
            left, right = height(max(0, c - 1), r), height(min(COLS, c + 1), r)
            up, down = height(c, max(0, r - 1)), height(c, min(ROWS, r + 1))
            nx, ny, nz = left - right, 2.0, up - down
            length = math.sqrt(nx * nx + ny * ny + nz * nz)
            normals.extend((nx / length, ny / length, nz / length))
            uvs.extend((c / COLS, r / ROWS))
    indices = []
    for r in range(ROWS):
        for c in range(COLS):
            a, b = r * across + c, r * across + c + 1
            d, e = a + across, b + across
            indices += [a, d, b, b, d, e]
    gltf = Gltf()
    position = gltf.floats(points, "VEC3", 3, ARRAY, bounded=True)
    normal = gltf.floats(normals, "VEC3", 3, ARRAY)
    uv = gltf.floats(uvs, "VEC2", 2, ARRAY)
    index = gltf.add(struct.pack("<%dI" % len(indices), *indices), UINT, len(indices), "SCALAR", ELEMENTS)
    document = textured(gltf, paint_land(rows), 0.95)
    document.update({"scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"name": "Land", "mesh": 0}],
                     "meshes": [{"name": "Land", "primitives": [{"attributes": {"POSITION": position, "NORMAL": normal,
                                                                                "TEXCOORD_0": uv},
                                                                 "indices": index, "material": 0}]}]})
    gltf.save("land.glb", document)


# What stands on it -----------------------------------------------------------------------------------------------------

def height_at(land, x, y):
    across = COLS + 1
    c, r = int(math.floor(x)), int(math.floor(y))
    c, r = max(0, min(COLS - 1, c)), max(0, min(ROWS - 1, r))
    fx, fy = x - c, y - r
    a, b = land[r * across + c], land[r * across + c + 1]
    d, e = land[(r + 1) * across + c], land[(r + 1) * across + c + 1]
    return ((a + (b - a) * fx) * (1 - fy) + (d + (e - d) * fx) * fy) / 100


def tree(s, x, y, z, ground, seed):
    size = 0.8 + 0.5 * hashed(seed, 3)
    turn = hashed(seed, 4) * math.pi
    if ground == MIRE:  # gnarled and dark, a low wide crown
        s.taper((x, y, z), 0.13 * size, 0.08 * size, 0.9 * size, 5, BARK, turn=turn)
        s.blob((x, y + 1.15 * size, z), 0.55 * size, PINE, seed, 0.65)
        s.blob((x + 0.2 * size, y + 0.9 * size, z + 0.15 * size), 0.34 * size, MOSS, seed + 1, 0.7)
    elif ground == SNOW:  # a pine with snow on its shoulders
        s.taper((x, y, z), 0.09 * size, 0.06 * size, 0.4 * size, 5, BARK, turn=turn)
        s.taper((x, y + 0.3 * size, z), 0.46 * size, 0, 0.85 * size, 6, PINE, turn=turn)
        s.taper((x, y + 0.75 * size, z), 0.34 * size, 0, 0.7 * size, 6, PINE, turn=turn + 0.4)
        s.taper((x, y + 1.15 * size, z), 0.2 * size, 0, 0.42 * size, 6, FROST, turn=turn)
    elif ground == ASH:  # dead, and bare
        s.taper((x, y, z), 0.11 * size, 0.04 * size, 1.2 * size, 5, DARK, turn=turn)
        s.taper((x + 0.03, y + 0.6 * size, z), 0.04 * size, 0.01, 0.55 * size, 4, DARK, turn=turn)
        s.box((x + 0.2 * size, y + 0.85 * size, z), (0.42 * size, 0.04, 0.04), DARK, turn=turn)
    else:  # the meadow's, round and full
        s.taper((x, y, z), 0.11 * size, 0.08 * size, 0.7 * size, 5, BARK, turn=turn)
        s.blob((x, y + 1.05 * size, z), 0.52 * size, GREEN, seed)
        s.blob((x - 0.18 * size, y + 0.8 * size, z + 0.1 * size), 0.32 * size, MOSS, seed + 1)


def house(s, cx, cy, y, seed):
    """Three tiles wide and two deep, around the corner the layout names: plank walls, thatch, a door and a lit window."""
    x, z = cx + 0.5, float(cy)
    s.box((x, y + 0.5, z), (2.9, 1.0, 1.9), BROWN)
    s.face([(x - 1.55, y + 1.0, z - 1.05), (x + 1.55, y + 1.0, z - 1.05), (x + 1.55, y + 1.8, z), (x - 1.55, y + 1.8, z)],
           THATCH, (x, y + 1.1, z))
    s.face([(x - 1.55, y + 1.0, z + 1.05), (x + 1.55, y + 1.0, z + 1.05), (x + 1.55, y + 1.8, z), (x - 1.55, y + 1.8, z)],
           THATCH, (x, y + 1.1, z))
    for end in (-1.45, 1.45):
        s.face([(x + end, y + 1.0, z - 0.95), (x + end, y + 1.0, z + 0.95), (x + end, y + 1.8, z)], BROWN, (x, y + 1.2, z))
    s.box((x, y + 0.36, z + 0.96), (0.42, 0.72, 0.04), BARK)
    s.box((x - 0.9, y + 0.55, z + 0.96), (0.36, 0.3, 0.04), EYE)
    s.box((x + 0.9, y + 0.55, z + 0.96), (0.36, 0.3, 0.04), EYE)
    s.box((x + 1.0, y + 1.75, z - 0.4), (0.26, 0.6, 0.26), STONE)


def beacon_base(s, cx, cy, y):
    x, z = cx + 0.5, cy + 0.5
    s.taper((x, y - 0.1, z), 0.85, 0.65, 0.35, 8, STONE)
    s.taper((x, y + 0.25, z), 0.4, 0.3, 0.9, 6, SLATE)
    s.taper((x, y + 1.15, z), 0.3, 0.5, 0.25, 8, IRON)


def scenery(rows, land):
    """What stands on the valley, in sectors, in four quarters."""
    sectors = {}
    quarter = lambda c, r: sectors.setdefault((int(c) // SECTOR, int(r) // SECTOR), Solid())
    houses = {(c, r) for cx, cy in HOUSES for r in range(cy - 1, cy + 1) for c in range(cx - 1, cx + 2)}

    def ground_near(c, r):
        for dc, dr in ((0, 1), (0, -1), (1, 0), (-1, 0), (1, 1), (-1, -1), (2, 0), (0, 2), (-2, 0), (0, -2)):
            nc, nr = c + dc, r + dr
            if 0 <= nc < COLS and 0 <= nr < ROWS and rows[nr][nc] in (GRASS, MIRE, ASH, SNOW, PATH):
                return rows[nr][nc]
        return GRASS

    for r in range(ROWS):
        for c in range(COLS):
            tile = rows[r][c]
            s = quarter(c, r)
            seed = r * COLS + c
            x = c + 0.5 + (hashed(seed, 1) - 0.5) * 0.3
            z = r + 0.5 + (hashed(seed, 2) - 0.5) * 0.3
            y = height_at(land, x, z)
            if tile == TREE:
                tree(s, x, y - 0.05, z, ground_near(c, r), seed)
            elif tile == ROCK:
                # Only what can be seen: rock with open ground beside it. The land itself rises behind it.
                if any(0 <= c + dc < COLS and 0 <= r + dr < ROWS and rows[r + dr][c + dc] not in BLOCKED
                       for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                    colour = FROST if ground_near(c, r) == SNOW and hashed(seed, 5) < 0.5 else STONE
                    s.blob((x, y + 0.1, z), 0.62 + 0.25 * hashed(seed, 6), colour, seed, 0.85)
            elif tile == WALL and (c, r) not in houses:
                s.box((c + 0.5, y + 0.55, r + 0.5), (1.02, 1.1 + 0.5 * hashed(seed, 7), 1.02), SLATE)
                s.box((c + 0.5, y + 1.2, r + 0.5), (0.8, 0.22, 0.8), STONE)
            elif tile == BRIDGE:
                s.box((c + 0.5, 0.12, r + 0.5), (1.02, 0.08, 1.02), BROWN)
                for n in range(3):
                    s.box((c + 0.5, 0.17, r + 0.17 + n / 3), (1.02, 0.02, 0.05), BARK)
            elif tile == GATE:
                pass
            elif tile in (GRASS, MIRE, SNOW) and hashed(seed, 8) < 0.5:
                # A tuft: three thin blades, the ground's own colour a shade darker.
                blade = {GRASS: MOSS, MIRE: THORN, SNOW: GREY}[tile]
                for n in range(3):
                    tx = c + hashed(seed, 10 + n)
                    tz = r + hashed(seed, 20 + n)
                    ty = height_at(land, tx, tz)
                    lean = hashed(seed, 30 + n) * 6.28
                    s.blade([(tx - 0.05, ty, tz), (tx + 0.05, ty, tz),
                             (tx + 0.08 * math.cos(lean), ty + 0.22, tz + 0.08 * math.sin(lean))], blade)

    for n, (cx, cy) in enumerate(HOUSES):
        house(quarter(cx, cy), cx, cy, height_at(land, cx + 0.5, cy), n)
    for name in ("mireBeacon", "ashBeacon", "highBeacon"):
        cx, cy = PLACES[name]
        beacon_base(quarter(cx, cy), cx, cy, height_at(land, cx + 0.5, cy + 0.5))
    # Hearth's well and its signpost at the crossroads.
    wx, wz = PLACES["hearth"][0] - 2.5, PLACES["hearth"][1] + 1.5
    wy = height_at(land, wx, wz)
    s = quarter(int(wx), int(wz))
    s.taper((wx, wy, wz), 0.42, 0.42, 0.45, 8, STONE)
    s.taper((wx, wy + 0.42, wz), 0.32, 0.32, 0.04, 8, BLUE)
    for side in (-0.34, 0.34):
        s.box((wx + side, wy + 0.8, wz), (0.06, 0.8, 0.06), BARK)
    s.face([(wx - 0.5, wy + 1.15, wz - 0.4), (wx + 0.5, wy + 1.15, wz - 0.4), (wx, wy + 1.5, wz)], THATCH, (wx, wy + 1.2, wz + 0.1))
    s.face([(wx - 0.5, wy + 1.15, wz + 0.4), (wx + 0.5, wy + 1.15, wz + 0.4), (wx, wy + 1.5, wz)], THATCH, (wx, wy + 1.2, wz - 0.1))
    cx, cy = PLACES["crossroads"]
    y = height_at(land, cx + 1.5, cy - 0.5)
    s = quarter(cx, cy)
    s.box((cx + 1.5, y + 0.6, cy - 0.5), (0.1, 1.2, 0.1), BARK)
    s.box((cx + 1.7, y + 1.05, cy - 0.5), (0.6, 0.16, 0.04), THATCH, turn=0.2)
    s.box((cx + 1.3, y + 0.8, cy - 0.5), (0.6, 0.16, 0.04), THATCH, turn=-0.3)
    # The king's dais, and the gate's two posts, which stand whether it is open or not.
    cx, cy = PLACES["crown"]
    y = height_at(land, cx + 0.5, cy + 0.5)
    s = quarter(cx, cy)
    s.taper((cx + 0.5, y - 0.1, cy + 0.5), 3.3, 2.9, 0.22, 10, SLATE)
    for n in range(6):
        a = n * math.pi / 3
        s.taper((cx + 0.5 + 3.5 * math.cos(a), y - 0.1, cy + 0.5 + 3.5 * math.sin(a)), 0.3, 0.14, 1.6 + 0.6 * hashed(n, 40), 5, STONE)
    gates = [(c, r) for r in range(ROWS) for c in range(COLS) if rows[r][c] == GATE]
    left, right = min(gates), max(gates)
    for c, r in (left, right):
        y = height_at(land, c + 0.5, r + 0.5)
        s = quarter(c, r)
        s.box((c + 0.5, y + 1.1, r + 0.5), (0.8, 2.4, 0.8), SLATE)
        s.taper((c + 0.5, y + 2.3, r + 0.5), 0.5, 0, 0.5, 4, IRON, turn=0.78)

    half = COLS // SECTOR // 2
    for n in range(4):
        gltf = Gltf()
        nodes, meshes = [], []
        for (sc, sr), solid in sorted(sectors.items()):
            if (1 if sc >= half else 0) + (2 if sr >= half else 0) != n or not solid.points:
                continue
            position = gltf.floats(solid.points, "VEC3", 3, ARRAY, bounded=True)
            normal = gltf.floats(solid.normals, "VEC3", 3, ARRAY)
            uv = gltf.floats(solid.uvs, "VEC2", 2, ARRAY)
            nodes.append({"name": "Sector_%d_%d" % (sc, sr), "mesh": len(meshes)})
            meshes.append({"name": "Sector_%d_%d" % (sc, sr),
                           "primitives": [{"attributes": {"POSITION": position, "NORMAL": normal, "TEXCOORD_0": uv}, "material": 0}]})
        document = textured(gltf, palette_png())
        document.update({"scene": 0, "scenes": [{"nodes": list(range(len(nodes)))}], "nodes": nodes, "meshes": meshes})
        gltf.save("scenery-%d.glb" % n, document)


# Figures ---------------------------------------------------------------------------------------------------------------

def quaternion(rx, ry, rz):
    """A turn from three angles in degrees, about x, then y, then z."""
    hx, hy, hz = math.radians(rx) / 2, math.radians(ry) / 2, math.radians(rz) / 2
    sx, cx, sy, cy, sz, cz = math.sin(hx), math.cos(hx), math.sin(hy), math.cos(hy), math.sin(hz), math.cos(hz)
    return (sx * cy * cz + cx * sy * sz, cx * sy * cz - sx * cy * sz, cx * cy * sz + sx * sy * cz, cx * cy * cz - sx * sy * sz)


def write_figure(name, joints, solid, clips):
    """A figure: its joints as (name, parent, place from the parent), its solid bound to them, and its clips, each
    (length, {joint: [(time, (rx, ry, rz)), ...]}, {joint: [(time, (dx, dy, dz)), ...]})."""
    gltf = Gltf()
    index = {joint[0]: i + 1 for i, joint in enumerate(joints)}  # node 0 holds the figure; the mesh comes last
    places = {}
    nodes = [{"name": name.capitalize(), "children": [1 + len(joints)]}]
    for joint_name, parent, place in joints:
        base = places[parent] if parent else (0.0, 0.0, 0.0)
        places[joint_name] = (base[0] + place[0], base[1] + place[1], base[2] + place[2])
        node = {"name": joint_name, "translation": list(place)}
        children = [index[j[0]] for j in joints if j[1] == joint_name]
        if children:
            node["children"] = children
        nodes.append(node)
    nodes[0]["children"] = [index[j[0]] for j in joints if j[1] is None] + [1 + len(joints)]
    nodes.append({"name": "Figure", "mesh": 0, "skin": 0})
    inverse = []
    for joint_name, _, _ in joints:
        x, y, z = places[joint_name]
        inverse += [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -x, -y, -z, 1]
    bind = gltf.floats(inverse, "MAT4", 16)
    position = gltf.floats(solid.points, "VEC3", 3, ARRAY, bounded=True)
    normal = gltf.floats(solid.normals, "VEC3", 3, ARRAY)
    uv = gltf.floats(solid.uvs, "VEC2", 2, ARRAY)
    joint_index = gltf.add(bytes(solid.joints), UBYTE, len(solid.joints) // 4, "VEC4", ARRAY)
    weights = gltf.floats([1.0, 0.0, 0.0, 0.0] * (len(solid.joints) // 4), "VEC4", 4, ARRAY)
    animations = []
    place_of = {joint[0]: joint[2] for joint in joints}
    for clip_name, (length, turns, moves) in clips.items():
        channels, samplers = [], []
        for joint_name, keys in turns.items():
            times = gltf.floats([t for t, _ in keys], "SCALAR", 1, bounded=True)
            values = gltf.floats([c for _, angles in keys for c in quaternion(*angles)], "VEC4", 4)
            samplers.append({"input": times, "output": values, "interpolation": "LINEAR"})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": index[joint_name], "path": "rotation"}})
        for joint_name, keys in moves.items():
            times = gltf.floats([t for t, _ in keys], "SCALAR", 1, bounded=True)
            base = place_of[joint_name]
            values = gltf.floats([base[k] + d[k] for _, d in keys for k in range(3)], "VEC3", 3)
            samplers.append({"input": times, "output": values, "interpolation": "LINEAR"})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": index[joint_name], "path": "translation"}})
        animations.append({"name": clip_name, "channels": channels, "samplers": samplers})
    document = textured(gltf, palette_png(), 0.75)
    document.update({"scene": 0, "scenes": [{"nodes": [0]}], "nodes": nodes,
                     "skins": [{"name": "Rig", "joints": [index[j[0]] for j in joints], "inverseBindMatrices": bind,
                                "skeleton": index[joints[0][0]]}],
                     "meshes": [{"name": "Figure", "primitives": [{"attributes": {"POSITION": position, "NORMAL": normal,
                                                                                  "TEXCOORD_0": uv, "JOINTS_0": joint_index,
                                                                                  "WEIGHTS_0": weights},
                                                                   "material": 0}]}],
                     "animations": animations})
    gltf.save(name + ".glb", document)


HUMAN_JOINTS = ["Hips", "Chest", "Head", "ArmL", "ArmR", "HandR", "LegL", "LegR", "ShinL", "ShinR"]


def human_rig(hip, chest, shoulder, thigh, arm):
    return [("Hips", None, (0.0, hip, 0.0)), ("Chest", "Hips", (0.0, 0.1 * hip, 0.0)), ("Head", "Chest", (0.0, chest, 0.0)),
            ("ArmL", "Chest", (shoulder, chest * 0.85, 0.0)), ("ArmR", "Chest", (-shoulder, chest * 0.85, 0.0)),
            ("HandR", "ArmR", (0.0, -arm, 0.0)), ("LegL", "Hips", (shoulder * 0.45, 0.0, 0.0)),
            ("LegR", "Hips", (-shoulder * 0.45, 0.0, 0.0)), ("ShinL", "LegL", (0.0, -thigh, 0.0)),
            ("ShinR", "LegR", (0.0, -thigh, 0.0))]


def human_clips(hip):
    """What a person's body does, the same joints for all who have them. Forward is +z; +x turns a limb backward."""
    a, b = 28, 20
    legs = lambda t, s: {"LegL": (t, (s * a, 0, 0)), "LegR": (t, (-s * a, 0, 0)), "ArmL": (t, (-s * b, 0, -6)),
                         "ArmR": (t, (s * b, 0, 6))}
    walk = {}
    for t, s in ((0.0, 1), (0.35, -1), (0.7, 1)):
        for joint, key in legs(t, s).items():
            walk.setdefault(joint, []).append(key)
    walk["ShinL"] = [(0.0, (10, 0, 0)), (0.175, (45, 0, 0)), (0.35, (6, 0, 0)), (0.7, (10, 0, 0))]
    walk["ShinR"] = [(0.0, (6, 0, 0)), (0.35, (10, 0, 0)), (0.525, (45, 0, 0)), (0.7, (6, 0, 0))]
    walk["Chest"] = [(0.0, (6, 4, 0)), (0.35, (6, -4, 0)), (0.7, (6, 4, 0))]
    idle = {"Chest": [(0.0, (0, 0, 0)), (1.0, (2.5, 0, 0)), (2.0, (0, 0, 0))],
            "Head": [(0.0, (0, -4, 0)), (1.0, (2, 5, 0)), (2.0, (0, -4, 0))],
            "ArmL": [(0.0, (0, 0, -5)), (1.0, (3, 0, -7)), (2.0, (0, 0, -5))],
            "ArmR": [(0.0, (0, 0, 5)), (1.0, (3, 0, 7)), (2.0, (0, 0, 5))]}
    swing1 = {"ArmR": [(0.0, (-20, 0, 40)), (0.13, (-130, 0, 50)), (0.27, (-60, 0, -30)), (0.4, (-20, 0, 5))],
              "Chest": [(0.0, (0, 0, 0)), (0.13, (0, 30, 0)), (0.27, (10, -35, 0)), (0.4, (0, 0, 0))]}
    swing2 = {"ArmR": [(0.0, (-60, 0, -30)), (0.13, (-100, 0, -60)), (0.27, (-70, 0, 55)), (0.4, (-20, 0, 5))],
              "Chest": [(0.0, (0, -25, 0)), (0.13, (0, -35, 0)), (0.27, (8, 35, 0)), (0.4, (0, 0, 0))]}
    swing3 = {"ArmR": [(0.0, (-30, 0, 10)), (0.3, (-190, 0, 10)), (0.42, (-50, 0, 0)), (0.6, (-20, 0, 5))],
              "ArmL": [(0.0, (0, 0, -5)), (0.3, (-170, 0, -10)), (0.42, (-50, 0, 0)), (0.6, (0, 0, -5))],
              "Chest": [(0.0, (0, 0, 0)), (0.3, (-18, 0, 0)), (0.42, (24, 0, 0)), (0.6, (0, 0, 0))],
              "Hips": [(0.0, (0, 0, 0)), (0.42, (8, 0, 0)), (0.6, (0, 0, 0))]}
    roll = {"Hips": [(0.0, (0, 0, 0)), (0.06, (90, 0, 0)), (0.12, (180, 0, 0)), (0.18, (270, 0, 0)), (0.24, (359, 0, 0))],
            "LegL": [(0.0, (-70, 0, 0)), (0.24, (-70, 0, 0))], "LegR": [(0.0, (-70, 0, 0)), (0.24, (-70, 0, 0))],
            "ShinL": [(0.0, (90, 0, 0)), (0.24, (90, 0, 0))], "ShinR": [(0.0, (90, 0, 0)), (0.24, (90, 0, 0))],
            "ArmL": [(0.0, (-80, 0, 0)), (0.24, (-80, 0, 0))], "ArmR": [(0.0, (-80, 0, 0)), (0.24, (-80, 0, 0))]}
    cast = {"ArmL": [(0.0, (0, 0, -5)), (0.14, (-110, 0, -8)), (0.27, (-70, 0, -5))],
            "ArmR": [(0.0, (0, 0, 5)), (0.14, (-110, 0, 8)), (0.27, (-70, 0, 5))],
            "Chest": [(0.0, (0, 0, 0)), (0.14, (-8, 0, 0)), (0.27, (6, 0, 0))]}
    hurt = {"Chest": [(0.0, (0, 0, 0)), (0.1, (-18, 0, 6)), (0.3, (0, 0, 0))],
            "Head": [(0.0, (0, 0, 0)), (0.1, (-14, 0, 0)), (0.3, (0, 0, 0))],
            "ArmL": [(0.0, (0, 0, -5)), (0.1, (20, 0, -28)), (0.3, (0, 0, -5))],
            "ArmR": [(0.0, (0, 0, 5)), (0.1, (20, 0, 28)), (0.3, (0, 0, 5))]}
    fall = {"Hips": [(0.0, (0, 0, 0)), (0.5, (-85, 0, 0)), (0.9, (-90, 0, 0))],
            "ArmL": [(0.0, (0, 0, -5)), (0.5, (-40, 0, -50)), (0.9, (-20, 0, -70))],
            "ArmR": [(0.0, (0, 0, 5)), (0.5, (-40, 0, 50)), (0.9, (-20, 0, 70))]}
    drop = {"Hips": [(0.0, (0, 0, 0)), (0.5, (0, -hip * 0.75, 0)), (0.9, (0, -hip * 0.82, 0))]}
    return {"idle": (2.0, idle, {}), "walk": (0.7, walk, {}), "swing1": (0.4, swing1, {}), "swing2": (0.4, swing2, {}),
            "swing3": (0.6, swing3, {}), "strike": (0.6, swing3, {}), "roll": (0.24, roll, {}), "cast": (0.27, cast, {}),
            "hurt": (0.3, hurt, {}), "fall": (0.9, fall, drop)}


def humanoid(name, height=1.0, build=1.0, skin=SKIN, hair=HAIR, top=TUNIC, belt=LEATHER, legs=DARK, boots=LEATHER, sleeves=None,
             hat=None, cape=None, weapon=None, skirt=None, hollow=False, horns=False, crown=False, glow=None):
    """A person, or something shaped like one: boxes bound to the rig, a head, a body, what it holds and what it wears."""
    s = Solid()
    hip = 0.47 * height
    chest = 0.3 * height
    shoulder = 0.17 * height * build
    thigh = 0.24 * height
    arm = 0.28 * height
    joints = human_rig(hip, chest, shoulder, thigh, arm)
    j = {n: i for i, n in enumerate(HUMAN_JOINTS)}
    place = {}
    for n, parent, at in joints:
        b = place[parent] if parent else (0.0, 0.0, 0.0)
        place[n] = (b[0] + at[0], b[1] + at[1], b[2] + at[2])
    w = 0.26 * height * build
    def at(joint, dx, dy, dz):
        p = place[joint]
        return (p[0] + dx, p[1] + dy, p[2] + dz)
    # The body.
    s.box(at("Chest", 0, chest * 0.42, 0), (w * 1.25, chest * 0.95, w * 0.72), top, j["Chest"])
    s.box(at("Hips", 0, 0.03 * height, 0), (w * 1.1, 0.12 * height, w * 0.68), belt, j["Hips"])
    # The head.
    head = 0.17 * height * (0.95 + 0.05 * build)
    if hollow:
        s.blob(at("Head", 0, head * 0.65, 0), head * 0.6, skin, 77, joint=j["Head"])
    else:
        s.box(at("Head", 0, head * 0.6, 0), (head, head * 1.1, head), skin, j["Head"])
        if hair is not None:
            s.box(at("Head", 0, head * 1.06, -head * 0.06), (head * 1.08, head * 0.32, head * 1.08), hair, j["Head"])
            s.box(at("Head", 0, head * 0.72, -head * 0.5), (head * 1.04, head * 0.72, head * 0.14), hair, j["Head"])
    eye = glow if glow is not None else DARK
    for side in (-1, 1):
        s.box(at("Head", side * head * 0.22, head * 0.66, head * 0.51), (head * 0.16, head * 0.1, head * 0.05), eye, j["Head"])
    if hat is not None:
        s.taper(at("Head", 0, head * 1.1, 0), head * 0.75, 0, head * 1.1, 6, hat, j["Head"])
    if crown:
        s.taper(at("Head", 0, head * 1.1, 0), head * 0.62, head * 0.72, head * 0.24, 6, GOLD, j["Head"])
        for n in range(6):
            a = n * math.pi / 3
            s.taper(at("Head", math.cos(a) * head * 0.62, head * 1.3, math.sin(a) * head * 0.62), head * 0.1, 0, head * 0.42, 4, GOLD, j["Head"])
    if horns:
        for side in (-1, 1):
            s.taper(at("Head", side * head * 0.55, head * 0.9, 0), head * 0.14, 0, head * 0.8, 4, BONE, j["Head"])
    # The arms.
    sleeve = sleeves if sleeves is not None else top
    for side, name_ in ((1, "ArmL"), (-1, "ArmR")):
        s.box(at(name_, 0, -arm * 0.5, 0), (0.1 * height * build, arm * 1.04, 0.1 * height * build), sleeve, j[name_])
        s.box(at(name_, 0, -arm * 1.06, 0), (0.09 * height * build, 0.09 * height, 0.09 * height * build), skin, j[name_])
    # The legs, or what it has instead.
    if skirt is not None:
        s.taper(at("Hips", 0, -hip + 0.02, 0), w * 0.95, w * 0.5, hip, 7, skirt, j["Hips"])
    else:
        for name_, shin in (("LegL", "ShinL"), ("LegR", "ShinR")):
            s.box(at(name_, 0, -thigh * 0.5, 0), (0.115 * height * build, thigh * 1.04, 0.115 * height * build), legs, j[name_])
            s.box(at(shin, 0, -thigh * 0.5, 0), (0.1 * height * build, thigh * 1.02, 0.1 * height * build), legs, j[shin])
            s.box(at(shin, 0, -hip + thigh + 0.035 * height, 0.03 * height), (0.11 * height * build, 0.07 * height, 0.18 * height), boots, j[shin])
    if cape is not None:
        s.box(at("Chest", 0, chest * 0.12, -w * 0.45), (w * 1.1, chest * 1.5, 0.03 * height), cape, j["Chest"])
    # What it holds, down the arm from the hand.
    hand = j["HandR"]
    if weapon == "sword":
        s.box(at("HandR", 0, -0.02, 0.28 * height), (0.05 * height, 0.02 * height, 0.56 * height), STEEL, hand)
        s.box(at("HandR", 0, -0.02, 0.0), (0.17 * height, 0.035 * height, 0.04 * height), GOLD, hand)
    elif weapon == "rust":
        s.box(at("HandR", 0, -0.02, 0.26 * height), (0.06 * height, 0.02 * height, 0.52 * height), IRON, hand)
    elif weapon == "staff":
        s.box(at("HandR", 0, 0.1 * height, 0.1 * height), (0.035 * height, 0.95 * height, 0.035 * height), BARK, hand)
        s.blob(at("HandR", 0, 0.62 * height, 0.1 * height), 0.075 * height, VIOLET, 5, joint=hand)
    elif weapon == "hammer":
        s.box(at("HandR", 0, -0.02, 0.32 * height), (0.05 * height, 0.05 * height, 0.75 * height), BARK, hand)
        s.box(at("HandR", 0, -0.02, 0.68 * height), (0.32 * height, 0.22 * height, 0.22 * height), IRON, hand)
    elif weapon == "greatblade":
        s.box(at("HandR", 0, -0.02, 0.46 * height), (0.1 * height, 0.03 * height, 0.92 * height), EMBER, hand)
        s.box(at("HandR", 0, -0.02, 0.0), (0.24 * height, 0.05 * height, 0.05 * height), GOLD, hand)
    elif weapon == "bow":
        s.box(at("HandR", 0, 0.0, 0.06 * height), (0.03 * height, 0.75 * height, 0.03 * height), BARK, hand)
    elif weapon == "tongs":
        s.box(at("HandR", 0, -0.02, 0.12 * height), (0.08 * height, 0.08 * height, 0.24 * height), IRON, hand)
    write_figure(name, joints, s, human_clips(hip))


def wolf():
    """Four legs and a tail: the wolf's own clips, the same names as a person's that it has."""
    s = Solid()
    high, long_ = 0.38, 0.4
    joints = [("Body", None, (0.0, high, 0.0)), ("Head", "Body", (0.0, 0.1, long_)), ("Tail", "Body", (0.0, 0.06, -long_)),
              ("FL", "Body", (0.12, -0.05, 0.28)), ("FR", "Body", (-0.12, -0.05, 0.28)),
              ("BL", "Body", (0.12, -0.05, -0.28)), ("BR", "Body", (-0.12, -0.05, -0.28))]
    place = {"Body": (0, high, 0), "Head": (0, high + 0.1, long_), "Tail": (0, high + 0.06, -long_),
             "FL": (0.12, high - 0.05, 0.28), "FR": (-0.12, high - 0.05, 0.28), "BL": (0.12, high - 0.05, -0.28),
             "BR": (-0.12, high - 0.05, -0.28)}
    index = {j[0]: i for i, j in enumerate(joints)}
    s.box((0, high + 0.03, 0.05), (0.3, 0.26, 0.8), FUR, index["Body"])
    s.box((0, high + 0.12, 0.2), (0.34, 0.12, 0.42), FUR_DARK, index["Body"])
    hx, hy, hz = place["Head"]
    s.box((hx, hy + 0.04, hz + 0.08), (0.22, 0.2, 0.22), FUR, index["Head"])
    s.box((hx, hy, hz + 0.26), (0.13, 0.1, 0.2), FUR_DARK, index["Head"])
    for side in (-1, 1):
        s.taper((hx + side * 0.08, hy + 0.13, hz + 0.02), 0.05, 0, 0.13, 4, FUR_DARK, index["Head"])
        s.box((hx + side * 0.07, hy + 0.07, hz + 0.19), (0.04, 0.03, 0.02), EYE, index["Head"])
    tx, ty, tz = place["Tail"]
    s.box((tx, ty - 0.02, tz - 0.17), (0.08, 0.08, 0.36), FUR_DARK, index["Tail"])
    for leg in ("FL", "FR", "BL", "BR"):
        lx, ly, lz = place[leg]
        s.box((lx, ly - ly / 2, lz), (0.08, ly, 0.09), FUR_DARK, index[leg])
    run = {}
    for t, f in ((0.0, 1), (0.2, -1), (0.4, 1)):
        for leg, sign in (("FL", 1), ("BR", 1), ("FR", -1), ("BL", -1)):
            run.setdefault(leg, []).append((t, (sign * f * 38, 0, 0)))
    run["Body"] = [(0.0, (3, 0, 0)), (0.1, (-3, 0, 0)), (0.2, (3, 0, 0)), (0.3, (-3, 0, 0)), (0.4, (3, 0, 0))]
    run["Tail"] = [(0.0, (-10, -12, 0)), (0.2, (-10, 12, 0)), (0.4, (-10, -12, 0))]
    idle = {"Head": [(0.0, (6, -8, 0)), (1.0, (0, 10, 0)), (2.0, (6, -8, 0))],
            "Tail": [(0.0, (22, -14, 0)), (0.5, (22, 14, 0)), (1.0, (22, -14, 0)), (1.5, (22, 14, 0)), (2.0, (22, -14, 0))],
            "Body": [(0.0, (0, 0, 0)), (1.0, (1.5, 0, 0)), (2.0, (0, 0, 0))]}
    bite = {"Body": [(0.0, (0, 0, 0)), (0.3, (-14, 0, 0)), (0.42, (18, 0, 0)), (0.6, (0, 0, 0))],
            "Head": [(0.0, (0, 0, 0)), (0.3, (-26, 0, 0)), (0.42, (22, 0, 0)), (0.6, (0, 0, 0))],
            "FL": [(0.0, (0, 0, 0)), (0.3, (-40, 0, 0)), (0.42, (-60, 0, 0)), (0.6, (0, 0, 0))],
            "FR": [(0.0, (0, 0, 0)), (0.3, (-40, 0, 0)), (0.42, (-60, 0, 0)), (0.6, (0, 0, 0))]}
    hurt = {"Body": [(0.0, (0, 0, 0)), (0.1, (-10, 0, 12)), (0.3, (0, 0, 0))], "Head": [(0.0, (0, 0, 0)), (0.1, (-24, 0, 0)), (0.3, (0, 0, 0))]}
    fall = {"Body": [(0.0, (0, 0, 0)), (0.5, (0, 0, 85)), (0.9, (0, 0, 90))],
            "Head": [(0.0, (0, 0, 0)), (0.9, (28, 0, 0))]}
    drop = {"Body": [(0.0, (0, 0, 0)), (0.5, (0, -0.22, 0)), (0.9, (0, -0.25, 0))]}
    write_figure("wolf", joints, s, {"idle": (2.0, idle, {}), "walk": (0.4, run, {}), "strike": (0.6, bite, {}),
                                     "hurt": (0.3, hurt, {}), "fall": (0.9, fall, drop)})


def figures():
    humanoid("hero", 1.0, 1.0, top=TUNIC, belt=LEATHER, legs=DARK, boots=LEATHER, sleeves=DARK, cape=RED, weapon="sword")
    humanoid("elder", 0.95, 0.95, hair=WHITE, top=VIOLET, legs=DARK, boots=BARK, skirt=VIOLET, weapon="staff")
    humanoid("healer", 0.92, 0.9, hair=BROWN, top=GREEN, sleeves=WHITE, skirt=GREEN)
    humanoid("smith", 1.02, 1.25, hair=DARK, top=BROWN, sleeves=SKIN, legs=DARK, boots=IRON, weapon="tongs")
    humanoid("hunter", 1.0, 1.0, hair=HAIR, top=MOSS, sleeves=BROWN, legs=BROWN, boots=BARK, hat=MOSS, weapon="bow")
    wolf()
    humanoid("shade", 0.95, 0.9, skin=GLOOM, hair=None, top=GLOOM, belt=GLOOM, sleeves=GLOOM, skirt=GLOOM, hollow=True, glow=TEAL, hat=GLOOM)
    humanoid("thornling", 0.72, 1.45, skin=THORN, hair=None, top=BARK, belt=BARK, sleeves=THORN, legs=BARK, boots=BARK, hollow=True, glow=EYE, horns=True)
    humanoid("revenant", 1.05, 1.1, skin=ROT, hair=None, top=IRON, belt=RED, sleeves=ROT, legs=ROT, boots=IRON, weapon="rust", glow=EMBER)
    humanoid("bonecaster", 1.0, 0.8, skin=BONE, hair=None, top=DARK, belt=VIOLET, sleeves=BONE, skirt=DARK, hollow=True, glow=VIOLET, weapon="staff", hat=VIOLET)
    humanoid("warden", 1.9, 1.3, skin=SLATE, hair=None, top=IRON, belt=EMBER, sleeves=SLATE, legs=IRON, boots=DARK, weapon="hammer", glow=EMBER, horns=True)
    humanoid("king", 2.2, 1.2, skin=BONE, hair=None, top=DARK, belt=GOLD, sleeves=DARK, skirt=RED, cape=RED, weapon="greatblade", hollow=True, glow=EMBER, crown=True)


# The sky ---------------------------------------------------------------------------------------------------------------

def rgbe(r, g, b):
    v = max(r, g, b)
    if v < 1e-32:
        return (0, 0, 0, 0)
    m, e = math.frexp(v)
    scale = m * 256.0 / v
    return (int(r * scale), int(g * scale), int(b * scale), e + 128)


def runs(values):
    out = bytearray()
    i = 0
    while i < len(values):
        run = 1
        while i + run < len(values) and run < 127 and values[i + run] == values[i]:
            run += 1
        if run > 2:
            out += bytes([128 + run, values[i]])
            i += run
            continue
        start = i
        while i < len(values) and i - start < 128:
            if i + 2 < len(values) and values[i] == values[i + 1] == values[i + 2]:
                break
            i += 1
        out += bytes([i - start]) + bytes(values[start:i])
    return bytes(out)


def hdr(name, w, h, pixel):
    out = bytearray(b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %d +X %d\n" % (h, w))
    for y in range(h):
        row = [rgbe(*pixel(x, y)) for x in range(w)]
        out += bytes([2, 2, w >> 8, w & 0xFF])
        for c in range(4):
            out += runs([p[c] for p in row])
    (ASSETS / name).write_bytes(bytes(out))


def dusk(x, y, w=256, h=128):
    latitude = math.pi / 2 - (y + 0.5) / h * math.pi
    longitude = (x + 0.5) / w * 2 * math.pi
    up = max(0.0, math.sin(latitude))
    near_horizon = math.exp(-abs(latitude) * 5.0)
    glow = max(0.0, math.cos(longitude - 3.9)) ** 3 * math.exp(-max(0.0, latitude) * 3.2)
    if latitude < 0:
        return [0.05 + 0.12 * near_horizon * c for c in (0.7, 0.45, 0.5)]
    star = 1.6 if up > 0.2 and hashed(x, y, 3) > 0.985 else 0.0
    return [0.06 + 0.5 * near_horizon * 0.9 + 1.5 * glow + star,
            0.075 + 0.22 * near_horizon + 0.62 * glow + star,
            0.17 + 0.2 * near_horizon + 0.28 * glow + star]


# The sounds ------------------------------------------------------------------------------------------------------------

def note(name):
    """A note's frequency from its name and octave: A4 is 440; "Bb" is B flat."""
    steps = {"C": -9, "D": -7, "E": -5, "F": -4, "G": -2, "A": 0, "B": 2}[name[0]]
    steps += 1 if "#" in name else -1 if "b" in name[1:-1] else 0
    return 440.0 * 2 ** ((steps + 12 * (int(name[-1]) - 4)) / 12)


def write(name, samples, peak=0.6, placed=False):
    """Scaled so its loudest sample is `peak` of full scale, and written 16-bit: in one channel if the game plays it from a
    place, otherwise the same in both."""
    top = max(abs(s) for s in samples) or 1.0
    scale = peak * 32767 / top
    frames = [int(round(s * scale)) for s in samples for _ in range(1 if placed else 2)]
    with wave.open(str(ASSETS / name), "wb") as w:
        w.setnchannels(1 if placed else 2)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack("<%dh" % len(frames), *frames))


def add(buffer, start, sound, wrap=False):
    n = len(buffer)
    for i, s in enumerate(sound):
        at = start + i
        if at >= n:
            if not wrap:
                break
            at %= n
        buffer[at] += s


def fade_end(out, seconds=0.02):
    for i in range(min(len(out), int(seconds * RATE))):
        out[-1 - i] *= i / (seconds * RATE)
    return out


class Noise:
    """A fixed sequence of noise, low-passed by `smooth` (0 none, toward 1 more)."""

    def __init__(self, seed):
        self.seed, self.low = seed, 0.0

    def next(self, smooth=0.0):
        self.seed = (self.seed * 1103515245 + 12345) & 0x7FFFFFFF
        raw = self.seed / 0x3FFFFFFF - 1.0
        self.low += (1 - smooth) * (raw - self.low)
        return self.low


def bell(f, seconds):
    out = []
    for i in range(int(seconds * RATE)):
        t = i / RATE
        attack = min(1.0, t / 0.003)
        out.append(attack * (math.sin(2 * math.pi * f * t) * math.exp(-t * 5.0)
                             + 0.35 * math.sin(2 * math.pi * 2 * f * t) * math.exp(-t * 8)
                             + 0.18 * math.sin(2 * math.pi * 2.76 * f * t) * math.exp(-t * 12)))
    return out


def sweep(f_start, f_end, seconds, decay, shape="sine"):
    out, phase = [], 0.0
    for i in range(int(seconds * RATE)):
        t = i / RATE
        f = f_start + (f_end - f_start) * (t / seconds)
        phase += 2 * math.pi * f / RATE
        wave_ = math.sin(phase) if shape == "sine" else (2 / math.pi) * math.asin(math.sin(phase))
        out.append(wave_ * math.exp(-t * decay) * min(1.0, t / 0.003))
    return out


def air(seconds, seed, smooth, envelope):
    noise = Noise(seed)
    n = int(seconds * RATE)
    return [noise.next(smooth) * envelope(i / n) for i in range(n)]


def thud(f0, f1, seconds, decay, seed, grit, smooth, grit_decay, voice="sine"):
    out = sweep(f0, f1, seconds, decay, voice)
    noise = Noise(seed)
    for i in range(len(out)):
        out[i] = out[i] + grit * noise.next(smooth) * math.exp(-i / RATE * grit_decay)
    return fade_end(out)


def chord(notes, seconds, gap, share=0.8):
    out = [0.0] * int(seconds * RATE)
    for k, n in enumerate(notes):
        add(out, int(k * gap * RATE), [share * s for s in bell(note(n), seconds - k * gap)])
    return fade_end(out)


def horn(f, seconds):
    out = []
    for i in range(int(seconds * RATE)):
        t = i / RATE
        swell = min(1.0, t / 0.25) * min(1.0, (seconds - t) / 0.3)
        out.append(swell * sum(math.sin(2 * math.pi * f * n * t) / n for n in (1, 2, 3, 4, 5)))
    return out


def rising():
    out = []
    for i in range(int(1.0 * RATE)):
        t = i / RATE
        f = 50 + 75 * t * t
        out.append((math.sin(2 * math.pi * f * t) + 0.4 * math.sin(2 * math.pi * 2 * f * t)) * (0.2 + 0.8 * t))
    return fade_end(out, 0.05)


def burst():
    out = [0.0] * int(0.7 * RATE)
    add(out, 0, thud(90, 40, 0.7, 5, 0xB057, 0.8, 0.6, 7))
    add(out, 0, [0.5 * s for s in sweep(900, 200, 0.5, 5)])
    return fade_end(out)


def curse():
    out = []
    for i in range(int(0.4 * RATE)):
        t = i / RATE
        f = 330 - 300 * t
        out.append(math.sin(2 * math.pi * f * t) * (0.6 + 0.4 * math.sin(2 * math.pi * 34 * t)) * math.exp(-t * 5))
    return fade_end(out)


def two_horns(a, b):
    out = [0.0] * int(1.7 * RATE)
    add(out, 0, horn(note(a), 0.85))
    add(out, int(0.75 * RATE), horn(note(b), 0.95))
    return fade_end(out)


def potion():
    out = [0.0] * int(0.5 * RATE)
    for k in range(7):
        add(out, int(k * 0.06 * RATE), [0.5 * s for s in sweep(500 + 90 * k, 900 + 120 * k, 0.06, 30)])
    return fade_end(out)


def falls(tones):
    out = [0.0] * int(1.4 * RATE)
    for k, f in enumerate(tones):
        add(out, int(k * 0.26 * RATE), sweep(f, f * 0.95, 0.75, 4))
    return fade_end(out)


def music():
    seconds = 16
    loop = [0.0] * (seconds * RATE)
    # The drone: G and D, two voices each a little apart, breathing slowly.
    for f in (note("G2"), note("D3"), note("G3")):
        for i in range(len(loop)):
            t = i / RATE
            breath = 0.6 + 0.4 * math.sin(2 * math.pi * t / seconds)
            loop[i] += 0.1 * breath * (math.sin(2 * math.pi * f * 0.998 * t) + math.sin(2 * math.pi * f * 1.002 * t))
    # A slow pulse, low in the ground.
    noise = Noise(0x6107)
    for beat in range(seconds // 4):
        thump = [0.5 * math.sin(2 * math.pi * 49 * i / RATE) * math.exp(-i / RATE * 9)
                 + 0.05 * noise.next(0.8) * math.exp(-i / RATE * 30) for i in range(int(0.5 * RATE))]
        add(loop, int(beat * 4 * RATE), thump, wrap=True)
    # A far-off pipe, a line that does not quite resolve.
    for at, n, length in ((2.0, "D5", 1.4), (3.5, "Bb4", 1.1), (4.75, "C5", 0.9), (6.0, "G4", 2.2), (10.0, "F5", 1.2), (11.25, "D5", 1.2), (12.5, "Eb5", 0.8), (13.5, "D5", 2.2)):
        f = note(n)
        pipe = []
        for i in range(int(length * RATE)):
            t = i / RATE
            shape = min(1.0, t / 0.18) * min(1.0, (length - t) / 0.3)
            vibrato = 1 + 0.004 * math.sin(2 * math.pi * 5 * t)
            pipe.append(0.13 * shape * (math.sin(2 * math.pi * f * vibrato * t) + 0.3 * math.sin(2 * math.pi * 2 * f * vibrato * t)))
        add(loop, int(at * RATE), pipe, wrap=True)
    return loop


def sounds():
    placed = {
        "swing": (lambda: fade_end(air(0.16, 0x51, 0.55, lambda p: math.sin(math.pi * p) ** 2)), 0.35),
        "hit": (lambda: thud(170, 90, 0.14, 26, 0x4B1, 0.6, 0.5, 40), 0.55),
        "crit": (lambda: fade_end([a + 0.5 * b for a, b in zip(thud(170, 90, 0.3, 14, 0x4B1, 0.6, 0.5, 30), bell(note("E6"), 0.3))]), 0.6),
        "kill": (lambda: thud(320, 70, 0.4, 7, 0x7A11, 0.7, 0.7, 9, "triangle"), 0.55),
        "hurt": (lambda: thud(130, 85, 0.35, 8, 0x2468, 0.8, 0.65, 16), 0.7),
        "roll": (lambda: fade_end(air(0.26, 0x0DA5, 0.45, lambda p: math.sin(math.pi * p))), 0.4),
        "potion": (potion, 0.5),
        "coin": (lambda: chord(("E7", "B7"), 0.18, 0.05), 0.4),
        "herb": (lambda: chord(("G5", "D6"), 0.5, 0.08), 0.5),
        "cast": (lambda: fade_end(sweep(300, 800, 0.24, 4)), 0.35),
        "bolt": (lambda: fade_end(sweep(1200, 500, 0.24, 7)), 0.4),
        "burst": (burst, 0.7),
        "thorn": (lambda: fade_end(air(0.14, 0x7402, 0.2, lambda p: (1 - p) ** 2)), 0.4),
        "curse": (curse, 0.45),
        "rising": (rising, 0.5),
        "slam": (lambda: thud(80, 32, 0.9, 4, 0x5A11, 0.9, 0.75, 6), 0.75),
        "wake": (lambda: fade_end(sweep(180, 240, 0.3, 5, "triangle")), 0.3),
    }
    everywhere = {
        "warden": (lambda: two_horns("G2", "Eb2"), 0.6),
        "king": (lambda: two_horns("C2", "Gb1"), 0.7),
        "level": (lambda: chord(("G5", "Bb5", "D6", "G6"), 0.9, 0.08, 0.7), 0.6),
        "beacon": (lambda: chord(("D5", "G5", "B5", "D6"), 1.6, 0.14), 0.65),
        "gate": (lambda: fade_end([a + 0.6 * b for a, b in zip(thud(60, 30, 2.0, 1.6, 0x6A7E, 0.5, 0.85, 1.4), [0.0] * int(0.3 * RATE) + bell(note("G4"), 1.7))]), 0.7),
        "talk": (lambda: fade_end(sweep(420, 520, 0.09, 12)), 0.3),
        "yes": (lambda: chord(("D6", "A6"), 0.3, 0.06), 0.4),
        "no": (lambda: fade_end(sweep(190, 150, 0.2, 8)), 0.4),
        "won": (lambda: chord(("G4", "B4", "D5", "G5", "B5", "D6"), 2.4, 0.16), 0.7),
        "lost": (lambda: falls((147, 123, 98)), 0.6),
        "fallen": (lambda: falls((196, 165)), 0.55),
        "gloam": (music, 0.45),
    }
    for name, (make, peak) in placed.items():
        write(name + ".wav", make(), peak, placed=True)
    for name, (make, peak) in everywhere.items():
        write(name + ".wav", make(), peak)


if __name__ == "__main__":
    ASSETS.mkdir(exist_ok=True)
    laid, heights_ = make_world.write()
    write_land(laid, heights_)
    scenery(laid, heights_)
    figures()
    hdr("gloam.hdr", 256, 128, dusk)
    sounds()
    for path in sorted(ASSETS.iterdir()):
        print(path.name, path.stat().st_size, "bytes")
