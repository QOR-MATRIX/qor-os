# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Writes Ever Rift's pictures, sky and sounds into assets/ (QOR-0017). Standard library only: python make_assets.py

  flagstones.png         256 x 256, the floor: four by four worn flagstones, each its own grey, in dark mortar.
  flagstones-relief.png  256 x 256, their relief as a normal map: stones raised from the mortar, pitted.
  rock.png               256 x 256, the rock the floors are cut from: layered noise, blue-grey, with veins.
  rock-relief.png        256 x 256, its relief.
  ever-rift.hdr              256 x 128, an equirectangular HDR sky that lights the floor: near black, a faint teal glow
                         overhead and an ember warmth below the horizon. Run-length encoded, as tools write Radiance
                         files: Qt misreads one written plainly (QQ's README, "Materials").
  swing.wav     0.16 s  a swing: a short rush of air.
  hit.wav       0.14 s  a blow landing: a dull knock.
  crit.wav      0.3 s   a heavy blow: the knock with a ring over it.
  kill.wav      0.4 s   an enemy falling: a crumble falling in pitch.
  hurt.wav      0.35 s  the delver wounded: a low grunt of a tone and a thud.
  whirl.wav     0.45 s  a whirl: air rushing round, rising and falling.
  dash.wav      0.22 s  a dash: a quick swoosh upward.
  potion.wav    0.5 s   a potion: a bubbling glass rising.
  coin.wav      0.18 s  gold: two bright clinks.
  relic.wav     0.8 s   a relic: bells a fifth apart, shimmering.
  bolt.wav      0.3 s   a seer's bolt: a falling buzz.
  rising.wav    1.0 s   the warden winding up: a groan rising for as long as it takes to land.
  slam.wav      0.9 s   the warden's slam, and its fall: a deep boom under a burst of rubble.
  warden.wav    1.6 s   the warden waking: a low horn, falling a minor third.
  level.wav     0.9 s   a level: a rising run of bells.
  descend.wav   1.2 s   a floor down: a long fall of tone into a low bell.
  cleared.wav   1.8 s   the rift cleared: bells climbing D minor to the octave.
  fallen.wav    1.4 s   the delver fallen: three low tones falling.
  ever-rift.wav     12 s    the music, looping: a low drone on D and A, a slow heartbeat, and far-off bells.

A sound the game plays from a place is written in one channel, as Qt's spatial sounds take it, and one heard everywhere
in both, as its ambient sounds take it. The noise is a fixed sequence, so the files are the same bytes each time.
"""
import math
import pathlib
import struct
import wave
import zlib

HERE = pathlib.Path(__file__).parent
ASSETS = HERE / "assets"
RATE = 22050


# Pictures -------------------------------------------------------------------------------------------------------------

def png(name, w, h, pixel):
    """An 8-bit RGB PNG of `pixel(x, y)`, each channel from 0 to 1."""
    rows = bytearray()
    for y in range(h):
        rows.append(0)
        for x in range(w):
            rows.extend(max(0, min(255, int(round(c * 255)))) for c in pixel(x, y))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    data = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))
    (ASSETS / name).write_bytes(data)


def hashed(x, y, seed):
    """A whole-number hash of a lattice point, from 0 to 1."""
    n = (x * 374761393 + y * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFF) / 65535.0


def value(x, y, period, seed):
    """Smooth value noise that tiles every `period` lattice cells."""
    x0, y0 = math.floor(x), math.floor(y)
    fx, fy = x - x0, y - y0
    sx, sy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)

    def at(i, j):
        return hashed((x0 + i) % period, (y0 + j) % period, seed)

    top = at(0, 0) + (at(1, 0) - at(0, 0)) * sx
    bottom = at(0, 1) + (at(1, 1) - at(0, 1)) * sx
    return top + (bottom - top) * sy


def fractal(x, y, size, octaves, seed, base=8):
    """Layered value noise over a picture `size` pixels square, tiling."""
    total, weight, amplitude, cells = 0.0, 0.0, 1.0, base
    for o in range(octaves):
        total += amplitude * value(x * cells / size, y * cells / size, cells, seed + o)
        weight += amplitude
        amplitude *= 0.5
        cells *= 2
    return total / weight


SIZE = 256
STONES = 4


def flag_height(x, y):
    """The flagstones' height: each stone raised and slightly domed, the mortar low, both pitted."""
    cell = SIZE // STONES
    # Every other row of stones is set half a stone over, as flagstones are laid.
    row = y // cell
    sx = (x + (cell // 2 if row % 2 else 0)) % SIZE
    cx, cy = sx % cell, y % cell
    edge = min(cx, cy, cell - 1 - cx, cell - 1 - cy)
    wobble = 2.5 * (value(x / 9, y / 9, SIZE // 9 + 1, 31) - 0.5)
    mortar = 3 + wobble
    stone = 0.0 if edge < mortar else min(1.0, (edge - mortar) / 5)
    pits = fractal(x, y, SIZE, 4, 7, 16)
    return stone * (0.75 + 0.25 * pits) + 0.08 * pits


def flag_colour(x, y):
    cell = SIZE // STONES
    row = y // cell
    sx = (x + (cell // 2 if row % 2 else 0)) % SIZE
    shade = 0.78 + 0.22 * hashed(sx // cell, row, 3)
    grain = fractal(x, y, SIZE, 5, 11, 8)
    h = flag_height(x, y)
    if h < 0.05:
        return (0.10, 0.10, 0.11)
    k = shade * (0.62 + 0.38 * grain)
    return (0.43 * k, 0.45 * k, 0.49 * k)


def rock_height(x, y):
    ridges = 1 - abs(2 * fractal(x, y, SIZE, 5, 41, 4) - 1)
    return 0.6 * fractal(x, y, SIZE, 6, 23, 8) + 0.4 * ridges ** 3


def rock_colour(x, y):
    h = rock_height(x, y)
    vein = max(0.0, 1 - abs(fractal(x, y, SIZE, 4, 57, 4) - 0.5) * 18)
    k = 0.35 + 0.65 * h
    return (0.30 * k + 0.05 * vein, 0.33 * k + 0.16 * vein, 0.38 * k + 0.18 * vein)


def relief(height, strength):
    """A normal map of `height`, tiling, its tilt along x in red and along y in green."""

    def pixel(x, y):
        dx = height((x + 1) % SIZE, y) - height((x - 1) % SIZE, y)
        dy = height(x, (y + 1) % SIZE) - height(x, (y - 1) % SIZE)
        nx, ny, nz = -dx * strength, dy * strength, 1.0
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        return ((nx / length + 1) / 2, (ny / length + 1) / 2, (nz / length + 1) / 2)

    return pixel


# The sky --------------------------------------------------------------------------------------------------------------

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


def cavern(x, y, w=256, h=128):
    latitude = math.pi / 2 - (y + 0.5) / h * math.pi
    up = max(0.0, math.sin(latitude))
    down = max(0.0, -math.sin(latitude))
    drift = 0.5 + 0.5 * value(x / 16, y / 16, w // 16, 5)
    return [0.006 + 0.03 * up ** 3 * drift * c + 0.02 * down ** 2 * e
            for c, e in zip((0.35, 0.9, 1.0), (1.0, 0.45, 0.15))]


# The sounds -----------------------------------------------------------------------------------------------------------

def note(name):
    """A note's frequency from its name and octave: A4 is 440; "Bb" is B flat."""
    steps = {"C": -9, "D": -7, "E": -5, "F": -4, "G": -2, "A": 0, "B": 2}[name[0]]
    steps += 1 if "#" in name else -1 if "b" in name[1:-1] else 0
    return 440.0 * 2 ** ((steps + 12 * (int(name[-1]) - 4)) / 12)


def write(name, samples, peak=0.6, placed=False):
    """Scaled so its loudest sample is `peak` of full scale, and written 16-bit: in one channel if the game plays it
    from a place, otherwise the same in both."""
    top = max(abs(s) for s in samples) or 1.0
    scale = peak * 32767 / top
    frames = [int(round(s * scale)) for s in samples for _ in range(1 if placed else 2)]
    with wave.open(str(ASSETS / name), "wb") as w:
        w.setnchannels(1 if placed else 2)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack("<%dh" % len(frames), *frames))


def add(buffer, start, sound, wrap=False):
    """Mix `sound` into `buffer` from sample `start`; past the end it is laid over the start when the buffer loops."""
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
    """Noise shaped by `envelope(t)`, from 0 to 1 over the sound."""
    noise = Noise(seed)
    n = int(seconds * RATE)
    return [noise.next(smooth) * envelope(i / n) for i in range(n)]


def swing():
    return fade_end(air(0.16, 0x51, 0.55, lambda p: math.sin(math.pi * p) ** 2))


def hit():
    out = sweep(170, 90, 0.14, 26)
    noise = Noise(0x4B1)
    for i in range(len(out)):
        out[i] += 0.6 * noise.next(0.5) * math.exp(-i / RATE * 40)
    return fade_end(out)


def crit():
    out = [0.0] * int(0.3 * RATE)
    add(out, 0, hit())
    add(out, int(0.01 * RATE), [0.5 * s for s in bell(note("E6"), 0.29)])
    return fade_end(out)


def kill():
    out = sweep(320, 70, 0.4, 7, "triangle")
    noise = Noise(0x7A11)
    for i in range(len(out)):
        out[i] = 0.5 * out[i] + 0.7 * noise.next(0.7) * math.exp(-i / RATE * 9)
    return fade_end(out)


def hurt():
    out = sweep(130, 85, 0.35, 8)
    noise = Noise(0x2468)
    for i in range(len(out)):
        out[i] += 0.8 * noise.next(0.65) * math.exp(-i / RATE * 16)
    return fade_end(out)


def whirl():
    return fade_end(air(0.45, 0x3311, 0.4, lambda p: math.sin(math.pi * p) * (0.6 + 0.4 * math.sin(2 * math.pi * 6 * p))))


def dash():
    out = air(0.22, 0x0DA5, 0.3, lambda p: math.sin(math.pi * p))
    rise = sweep(300, 900, 0.22, 6)
    return fade_end([a + 0.3 * b for a, b in zip(out, rise)])


def potion():
    out = [0.0] * int(0.5 * RATE)
    for k in range(7):
        add(out, int(k * 0.06 * RATE), [0.5 * s for s in sweep(500 + 90 * k, 900 + 120 * k, 0.06, 30)])
    return fade_end(out)


def coin():
    out = [0.0] * int(0.18 * RATE)
    add(out, 0, [0.6 * s for s in bell(note("E7"), 0.18)])
    add(out, int(0.05 * RATE), [0.5 * s for s in bell(note("B7"), 0.13)])
    return fade_end(out)


def relic():
    out = [0.0] * int(0.8 * RATE)
    add(out, 0, bell(note("D6"), 0.8))
    add(out, int(0.07 * RATE), [0.8 * s for s in bell(note("A6"), 0.73)])
    add(out, int(0.14 * RATE), [0.5 * s for s in bell(note("D7"), 0.66)])
    return fade_end(out)


def bolt():
    out = []
    for i in range(int(0.3 * RATE)):
        t = i / RATE
        f = 700 - 900 * t
        out.append(math.sin(2 * math.pi * f * t) * (0.5 + 0.5 * math.sin(2 * math.pi * 60 * t)) * math.exp(-t * 6))
    return fade_end(out)


def rising():
    out = []
    for i in range(int(1.0 * RATE)):
        t = i / RATE
        f = 55 + 70 * t * t
        out.append((math.sin(2 * math.pi * f * t) + 0.4 * math.sin(2 * math.pi * 2 * f * t)) * (0.2 + 0.8 * t))
    return fade_end(out, 0.05)


def slam():
    out = sweep(80, 32, 0.9, 4)
    noise = Noise(0x5A11)
    for i in range(len(out)):
        out[i] += 0.9 * noise.next(0.75) * math.exp(-i / RATE * 6)
    return fade_end(out)


def horn(f, seconds):
    out = []
    for i in range(int(seconds * RATE)):
        t = i / RATE
        swell = min(1.0, t / 0.25) * min(1.0, (seconds - t) / 0.3)
        out.append(swell * sum(math.sin(2 * math.pi * f * n * t) / n for n in (1, 2, 3, 4, 5)))
    return out


def warden():
    out = [0.0] * int(1.6 * RATE)
    add(out, 0, horn(note("D2"), 0.8))
    add(out, int(0.7 * RATE), horn(note("B1"), 0.9))
    return fade_end(out)


def level():
    out = [0.0] * int(0.9 * RATE)
    for k, n in enumerate(("D5", "F5", "A5", "D6")):
        add(out, int(k * 0.08 * RATE), [0.7 * s for s in bell(note(n), 0.9 - k * 0.08)])
    return fade_end(out)


def descend():
    out = [0.0] * int(1.2 * RATE)
    add(out, 0, [0.6 * s for s in sweep(600, 120, 0.7, 2)])
    add(out, int(0.55 * RATE), bell(note("D4"), 0.65))
    return fade_end(out)


def cleared():
    out = [0.0] * int(1.8 * RATE)
    for k, n in enumerate(("D5", "F5", "A5", "D6", "F6")):
        add(out, int(k * 0.12 * RATE), [0.8 * s for s in bell(note(n), 1.8 - k * 0.12)])
    return fade_end(out)


def fallen():
    out = [0.0] * int(1.4 * RATE)
    for k, f in enumerate((147, 123, 98)):
        add(out, int(k * 0.26 * RATE), sweep(f, f * 0.95, 0.75, 4))
    return fade_end(out)


def music():
    seconds = 12
    loop = [0.0] * (seconds * RATE)
    # The drone: D and A, two voices each a little apart, breathing slowly.
    for f in (note("D2"), note("A2"), note("D3")):
        for i in range(len(loop)):
            t = i / RATE
            breath = 0.6 + 0.4 * math.sin(2 * math.pi * t / seconds)
            loop[i] += 0.12 * breath * (math.sin(2 * math.pi * f * 0.998 * t) + math.sin(2 * math.pi * f * 1.002 * t))
    # A slow heartbeat, twice every two seconds.
    noise = Noise(0xBEA7)
    for beat in range(seconds // 2):
        for k, at in enumerate((0.0, 0.28)):
            thump = [(0.9 - 0.3 * k) * math.sin(2 * math.pi * 52 * i / RATE) * math.exp(-i / RATE * 14)
                     + 0.1 * noise.next(0.8) * math.exp(-i / RATE * 30) for i in range(int(0.3 * RATE))]
            add(loop, int((beat * 2 + at) * RATE), thump, wrap=True)
    # Far-off bells.
    for at, n in ((1.5, "A4"), (5.0, "F4"), (8.5, "D5"), (10.25, "C5")):
        add(loop, int(at * RATE), [0.12 * s for s in bell(note(n), 2.5)], wrap=True)
    return loop


if __name__ == "__main__":
    ASSETS.mkdir(exist_ok=True)
    png("flagstones.png", SIZE, SIZE, flag_colour)
    png("flagstones-relief.png", SIZE, SIZE, relief(flag_height, 3.0))
    png("rock.png", SIZE, SIZE, rock_colour)
    png("rock-relief.png", SIZE, SIZE, relief(rock_height, 6.0))
    hdr("ever-rift.hdr", 256, 128, cavern)
    placed = {"swing": (swing, 0.35), "hit": (hit, 0.55), "crit": (crit, 0.6), "kill": (kill, 0.55),
              "hurt": (hurt, 0.7), "whirl": (whirl, 0.5), "dash": (dash, 0.45), "potion": (potion, 0.5),
              "coin": (coin, 0.4), "relic": (relic, 0.55), "bolt": (bolt, 0.4), "rising": (rising, 0.5),
              "slam": (slam, 0.75)}
    everywhere = {"warden": (warden, 0.6), "level": (level, 0.6), "descend": (descend, 0.6), "cleared": (cleared, 0.7),
                  "fallen": (fallen, 0.6), "ever-rift": (music, 0.45)}
    for name, (make, peak) in placed.items():
        write(name + ".wav", make(), peak, placed=True)
    for name, (make, peak) in everywhere.items():
        write(name + ".wav", make(), peak)
    for path in sorted(ASSETS.iterdir()):
        print(path.name, path.stat().st_size, "bytes")
