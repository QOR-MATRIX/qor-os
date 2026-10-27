#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
"""Synthesises the Lumen interface sounds into kit/sounds/*.wav.

Every sound is generated from first principles (sines, FM bells, filtered
noise), so the set is original, deterministic and free of third-party
rights. Rerun after changing a recipe; the output is byte-for-byte stable.

    python tools/sounds/generate_sounds.py
"""

from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

RATE = 48000
OUT = Path(__file__).resolve().parents[2] / "kit" / "sounds"


def silence(seconds: float) -> list[float]:
    return [0.0] * int(seconds * RATE)


def mix(target: list[float], source: list[float], at: float = 0.0, gain: float = 1.0) -> None:
    start = int(at * RATE)
    if start + len(source) > len(target):
        target.extend([0.0] * (start + len(source) - len(target)))
    for i, value in enumerate(source):
        target[start + i] += value * gain


def envelope(t: float, attack: float, decay: float) -> float:
    if t < attack:
        return t / attack
    return math.exp(-(t - attack) / decay)


def bell(freq: float, seconds: float, decay: float, ratio: float = 2.0, index: float = 1.6) -> list[float]:
    """Two-operator FM bell: bright at the strike, mellowing as it rings."""
    out = []
    for n in range(int(seconds * RATE)):
        t = n / RATE
        mod_index = index * math.exp(-t / (decay * 0.35))
        modulator = math.sin(2 * math.pi * freq * ratio * t) * mod_index
        out.append(math.sin(2 * math.pi * freq * t + modulator) * envelope(t, 0.002, decay))
    return out


def tone(freq: float, seconds: float, attack: float, decay: float, glide: float = 0.0) -> list[float]:
    out = []
    phase = 0.0
    for n in range(int(seconds * RATE)):
        t = n / RATE
        f = freq * (1.0 + glide * t / seconds)
        phase += 2 * math.pi * f / RATE
        out.append(math.sin(phase) * envelope(t, attack, decay))
    return out


def noise_burst(seconds: float, decay: float, cutoff: float, seed: int = 1) -> list[float]:
    """Low-passed noise (one-pole), for soft mechanical textures."""
    state = seed
    out = []
    lowpass = 0.0
    k = 1 - math.exp(-2 * math.pi * cutoff / RATE)
    for n in range(int(seconds * RATE)):
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        white = state / 0x3FFFFFFF - 1.0
        lowpass += k * (white - lowpass)
        out.append(lowpass * math.exp(-n / RATE / decay))
    return out


def finish(samples: list[float], peak_db: float = -6.0) -> list[float]:
    """Normalises to `peak_db` and fades the last 5 ms to avoid clicks."""
    peak = max(1e-9, max(abs(s) for s in samples))
    gain = 10 ** (peak_db / 20) / peak
    fade = int(0.005 * RATE)
    out = [s * gain for s in samples]
    for i in range(min(fade, len(out))):
        out[-1 - i] *= i / fade
    return out


def write(name: str, samples: list[float]) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / f"{name}.wav"
    with wave.open(str(path), "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(b"".join(struct.pack("<h", int(round(max(-1.0, min(1.0, s)) * 32767))) for s in samples))
    print(f"{path.relative_to(OUT.parents[1])}: {len(samples) / RATE:.2f} s")


def notification() -> list[float]:
    # A rising fifth on glassy bells.
    out = silence(0.9)
    mix(out, bell(1318.5, 0.8, 0.28), 0.0, 0.8)    # E6
    mix(out, bell(1975.5, 0.75, 0.30), 0.09, 0.6)  # B6
    return finish(out, -7.0)


def lock() -> list[float]:
    # A soft, low "thock" settling downwards.
    out = silence(0.3)
    mix(out, noise_burst(0.06, 0.012, 900, seed=7), 0.0, 0.5)
    mix(out, tone(220.0, 0.25, 0.003, 0.06, glide=-0.35), 0.0, 0.9)
    return finish(out, -8.0)


def unlock() -> list[float]:
    # Two airy notes lifting upwards.
    out = silence(0.45)
    mix(out, bell(659.3, 0.35, 0.14, ratio=3.0, index=0.9), 0.0, 0.7)    # E5
    mix(out, bell(987.8, 0.35, 0.16, ratio=3.0, index=0.9), 0.07, 0.6)   # B5
    return finish(out, -8.0)


def volume() -> list[float]:
    # A short, rounded tick for volume changes.
    out = tone(1046.5, 0.07, 0.001, 0.014)
    mix(out, noise_burst(0.02, 0.003, 4000, seed=3), 0.0, 0.15)
    return finish(out, -9.0)


def connect() -> list[float]:
    # A bright major triad arpeggio: something was attached.
    out = silence(0.6)
    for i, f in enumerate((783.99, 987.77, 1174.66)):  # G5 B5 D6
        mix(out, bell(f, 0.45, 0.16, ratio=2.0, index=1.1), 0.06 * i, 0.55)
    return finish(out, -8.0)


def main() -> None:
    write("notification", notification())
    write("lock", lock())
    write("unlock", unlock())
    write("volume", volume())
    write("connect", connect())


if __name__ == "__main__":
    main()
