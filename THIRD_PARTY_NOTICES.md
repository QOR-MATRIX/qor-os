# Third-party notices

QOR OS includes or depends on the following third-party components. Their
license texts ship next to the files they cover.

## Code from demiurge-chain (Apache-2.0)

These came from [`QOR-MATRIX/demiurge-chain`](https://github.com/QOR-MATRIX/demiurge-chain)
with their history
([QOR-0002](docs/decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md),
[docs/UPSTREAM.md](docs/UPSTREAM.md)). They remain under the Apache License,
Version 2.0, whose text is [`LICENSES/Apache-2.0.txt`](LICENSES/Apache-2.0.txt)
("Copyright 2026 Demiurge"). A file from demiurge-chain changed in QOR OS says
so in a comment near its top, as the license's §4(b) requires.

| Path | What it is | What is Apache-2.0 |
|------|------------|--------------------|
| `products/qq/` | QQ, the QOR Engine | The files listed in [`LICENSES/from-demiurge-chain.txt`](LICENSES/from-demiurge-chain.txt) |
| `products/arqade/qt/`, `products/arqade/games/` | ARQADE's Qt app and its games | The same |
| `tools/qor-media/` | QOR Media | The same |
| `products/arqade/tests/fixtures/` | The runs ARQADE's games replay, still carried | All of it |
| `docs/design/tokens.json` | Demiurge's one look, still copied | All of it |

Since 29 October 2026 QQ, ARQADE's app and games, and QOR Media are developed
in QOR OS ([QOR-0015](docs/decisions/QOR-0015-the-products-are-developed-here.md)).
Changes made to their files after that, and files added to them, are QOR OS's,
under [LICENSE](LICENSE), as the Apache License's §4 allows for
modifications. The exception is ARQADE's five rule modules (each game's
`logic/rules.mjs`, and the arenas' `logic/arena.mjs`). ARQADE's server shares
them, so they are kept the same as demiurge-chain's and stay Apache-2.0 as
they change. Everything else in this repository is proprietary too.

The **QOR core** is built from demiurge-chain at a pinned commit and linked
into QOR OS, not carried in this repository
([QOR-0003](docs/decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)). At
the current pin, `6a4822d`, the core's manifest (`qor-launcher-core`) declares
`license = "Apache-2.0"`, the licence of demiurge-chain's root `LICENSE`
("Copyright 2026 Demiurge"); its text is in
[`LICENSES/Apache-2.0.txt`](LICENSES/Apache-2.0.txt). It has declared that
since demiurge-chain's ADR-122, which the owner accepted on 27 October 2026
and which merged on 28 October (that repository's pull request #109). Earlier
commits declared `MIT`, and so did the core at QOR OS's earlier pins. The
`mesh-proof` crate it compiles in from the same repository (through its
`chain` feature, which `desktop` turns on) declares Apache-2.0 too. Who holds
the copyright is for the owner to settle
([docs/release/README.md](docs/release/README.md#one-question-first-which-legal-entity)).
The core's Rust dependencies are listed by its `Cargo.lock` at the pinned
commit. Their notices must accompany any distributed binary (Phase 3:
generated with the image's SBOM).

## Components

| Component | Version | Where | License |
|-----------|---------|-------|---------|
| [Qt](https://www.qt.io) | 6.12.0 | build and runtime dependency | Commercial Qt license held by QOR-MATRIX |
| [FFmpeg](https://ffmpeg.org) (through Qt Multimedia's FFmpeg backend) | 9.0.1, shipped with Qt | runtime dependency for decoding audio and video | LGPL 2.1 or later; distributed as Qt's shared libraries. See Qt's third-party notices |
| PulseAudio client library (`libpulse0`, Linux) | system package | runtime dependency of Qt Multimedia | LGPL 2.1 or later |
| [PDFium](https://pdfium.googlesource.com/pdfium/) (binary build by [bblanchon/pdfium-binaries](https://github.com/bblanchon/pdfium-binaries)) | `chromium/8111`, pinned by SHA-256 in `tools/qor-media/build.ps1` | fetched at build time; deployed beside QOR Media with its licences | BSD-3-Clause and Apache-2.0, with its own third-party licences in the release |
| [Inter](https://rsms.me/inter/) (variable, roman and italic) | 4.1 | `kit/fonts/InterVariable*.ttf` | SIL Open Font License 1.1, see [`kit/fonts/LICENSE.Inter.txt`](kit/fonts/LICENSE.Inter.txt) |
| [JetBrains Mono](https://www.jetbrains.com/lp/mono/) (variable) | 2.304 | `kit/fonts/JetBrainsMono-Variable.ttf` | SIL Open Font License 1.1, see [`kit/fonts/LICENSE.JetBrainsMono.txt`](kit/fonts/LICENSE.JetBrainsMono.txt) |
| [Cinzel](https://fonts.google.com/specimen/Cinzel) | as carried | `products/qq/runtime/fonts/`, `products/arqade/qt/fonts/` | SIL Open Font License 1.1, `Cinzel-OFL.txt` beside each copy |
| [Barlow and Barlow Semi Condensed](https://fonts.google.com/specimen/Barlow) | as carried | `products/*/fonts/`, `tools/qor-media/fonts/` | SIL Open Font License 1.1, `Barlow-OFL.txt` and `BarlowSemiCondensed-OFL.txt` beside each copy |
| [IBM Plex Mono](https://fonts.google.com/specimen/IBM+Plex+Mono) | as carried | `products/qq/studio/fonts/`, `tools/qor-media/fonts/` | SIL Open Font License 1.1, `IBMPlexMono-OFL.txt` beside each copy |
| [Great Vibes](https://fonts.google.com/specimen/Great+Vibes) | as carried | `tools/qor-media/fonts/` | SIL Open Font License 1.1, `GreatVibes-OFL.txt` |
| [Lucide](https://lucide.dev) icons (113-icon subset, recoloured) | lucide-static 1.61.0 | `kit/icons/*.svg` | ISC; icons derived from Feather are MIT. See [`kit/icons/LICENSE.lucide.txt`](kit/icons/LICENSE.lucide.txt) |
| [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols): fractional-scale-v1, cursor-shape-v1, tablet-v2 | 1.47 | `prism/compositor/protocols/*.xml` | MIT; the notice is in each file's `<copyright>` element |

## Build-time tooling

These tools are used during development and CI. They are not distributed with
QOR OS.

| Tool | License |
|------|---------|
| CMake, Ninja | BSD-3-Clause, Apache-2.0 |
| [Corrosion](https://github.com/corrosion-rs/corrosion) v0.6.1 (builds the QOR core from CMake) | MIT |
| Rust toolchain (`cargo`, `rustc`) | MIT or Apache-2.0 |
| clang-format (PyPI `clang-format`) | Apache-2.0 WITH LLVM-exception |
| pre-commit | MIT |
| Python standard library | PSF |

## Updating this file

When you add a third-party asset or library, add a row here and place its
license text beside it. Fonts and icons are imported reproducibly:
`tools/icons/import_lucide.py` for icons, and a direct copy of the upstream
release for fonts (see [`kit/README.md`](kit/README.md)). Fonts in the carried
products are managed upstream by Demiurge's `docs/design/tokens.mjs`.
