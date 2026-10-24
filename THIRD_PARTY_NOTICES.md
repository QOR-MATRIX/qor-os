# Third-party notices

QTOS includes or depends on the following third-party components. Their
license texts ship next to the files they cover.

| Component | Version | Where | License |
|-----------|---------|-------|---------|
| [Qt](https://www.qt.io) | 6.12.0 | build and runtime dependency | Commercial Qt license held by QOR-MATRIX |
| [Inter](https://rsms.me/inter/) (variable, roman and italic) | 4.1 | `kit/fonts/InterVariable*.ttf` | SIL Open Font License 1.1, see [`kit/fonts/LICENSE.Inter.txt`](kit/fonts/LICENSE.Inter.txt) |
| [JetBrains Mono](https://www.jetbrains.com/lp/mono/) (variable) | 2.304 | `kit/fonts/JetBrainsMono-Variable.ttf` | SIL Open Font License 1.1, see [`kit/fonts/LICENSE.JetBrainsMono.txt`](kit/fonts/LICENSE.JetBrainsMono.txt) |
| [Lucide](https://lucide.dev) icons (106-icon subset, recoloured) | lucide-static 1.61.0 | `kit/icons/*.svg` | ISC; icons derived from Feather are MIT. See [`kit/icons/LICENSE.lucide.txt`](kit/icons/LICENSE.lucide.txt) |
| [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols): fractional-scale-v1, cursor-shape-v1, tablet-v2 | 1.47 | `prism/compositor/protocols/*.xml` | MIT; the notice is in each file's `<copyright>` element |

## Build-time tooling

These tools are used during development and CI. They are not distributed with
QTOS.

| Tool | License |
|------|---------|
| CMake, Ninja | BSD-3-Clause, Apache-2.0 |
| clang-format (PyPI `clang-format`) | Apache-2.0 WITH LLVM-exception |
| pre-commit | MIT |
| Python standard library | PSF |

## Updating this file

When you add a third-party asset or library, add a row here and place its
license text beside it. Fonts and icons are imported reproducibly:
`tools/icons/import_lucide.py` for icons, and a direct copy of the upstream
release for fonts (see [`kit/README.md`](kit/README.md)).
