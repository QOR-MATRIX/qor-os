# QOR-0009: The name is QOR OS, in the code too

- **Status:** Accepted
- **Date:** 2026-10-27
- **Deciders:** the owner
- **Relates to:** supersedes decision 2 of
  [QOR-0008](QOR-0008-before-a-public-release.md) (decision 1, the release
  gates, stands); [QOR-0002](QOR-0002-one-repository-from-qtos-and-demiurge.md);
  Demiurge
  [ADR-120](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-120-qor-os-carries-the-device-side-products.md)

## Context

QOR-0008 kept QTOS's names in the code until a trademark attorney cleared
"QOR OS", so that a rejected name would cost one rename instead of two. On
27 October 2026 the owner asked that the fork of QTOS and Demiurge be named
"QOR OS" properly. Until then:

- the code said QTOS in about 1,100 places across 439 files: the `qtos::`
  namespace, `QTOS.*` QML modules, `QTOS_*` options, `qtos-*` programs, the
  licence identifier and the text the shell shows;
- the shell called itself "QTOS" in its About page and window titles;
- the Store's packages were `org.qtos.*` (a domain no one holds) and named
  their catalogue "QTOS Store";
- every contributor and agent had to learn two names for one thing.

The carried products had already set the pattern: QOR Media is
`namespace qormedia` and the QML module `QOR.Media`; QQ is `qq`; ARQADE is
`arqade`. The QOR core is `qor::core` and its options are `QOR_*`. The
ecosystem's domain is `qorsync.dev` (Demiurge's homepage and QOR ID at
`id.qorsync.dev`).

Renaming now costs one change while the code has one developer, no release
and no outside users. The ecosystem's other names (QOR ID, QOR Media, QOR
Launcher) are already QOR, so clearance of "QOR OS" is a question about one
word, which the release gates still hold.

## Decision

1. **The product is "QOR OS"**: two words, capitals, one space. That is the
   form in prose, in everything the interface shows, and in metadata. "QTOS"
   names only the predecessor: its frozen repository, its records
   (QTOS-0001 to QTOS-0010), its changelog sections and its history.
2. **Where a space cannot go:**

   | Where | Form | Examples |
   |-------|------|----------|
   | Repository, packages, artifacts, the CMake project | `qor-os` | `QOR-MATRIX/qor-os`, `project(qor-os)`, `qor-os-windows-x64` |
   | C++ namespaces, CMake targets and functions | `qor` | `qor::sim`, `qor_kit`, `qor_add_test` |
   | Programs and services | `qor-*`, `qorctl` | `qor-shell`, `qor-simd`, `qor-gamepads`, `qor-audiod` |
   | Logging categories, resource prefixes | `qor.*`, `/qor/` | `qor.audio`, `:/qor/sim/` |
   | QML modules | `QOR.*`, beside the carried `QOR.Media` | `QOR.Kit`, `QOR.Shell`, `QOR.Services` |
   | Options, environment and CI variables | `QOR_*` | `QOR_BUILD_TESTS`, `QOR_QT_DIR`, `QOR_WINDOWS_RUNNER` |
   | Identifiers that need a domain | `dev.qorsync.*` | `dev.qorsync.theme.ember`, developer `dev.qorsync` |
   | Store package files | `.qorpkg` | `dev.qorsync.widget.clock-1.0.0.qorpkg` |
   | Source licence | `LicenseRef-QOR-OS-Proprietary` | every file's SPDX line |
   | File names | `qor`, `Qor` | `QorMark.qml`, `qorplatformtheme.cpp`, `cmake/QorLocateQt.cmake` |

   Feature names drop the OS where it adds nothing: **QOR Sessions** (the
   generated music), **QOR Kit** (`QOR.Kit`). The Store is the **QOR OS
   Store**. The parts keep their own names: Prism, Lumen, Beam, Shelf, Rail,
   Stash.
3. **Nothing the owner has is lost in the move:**
   - the shell and the simulator copy their settings from the old names
     ("QTOS Shell", "QTOS Simulator") the first time they run under the new
     ones, and move the old data folder, with installed Store items, to the
     new one (`sim/src/carryover.cpp`); the old settings stay in place;
   - the Store's catalogue is re-signed with the same store key, so
     `store/keys/store.pub`, the root every installation trusts, is
     unchanged. The first-party developer `dev.qorsync` has a new key of its
     own;
   - the signing keys stay where they are: `build_store.py` reads QTOS's
     `~/.qtos/store-signing/` until the owner moves it to
     `~/.qor-os/store-signing/`, and never creates a store key while the old
     folder holds one.
4. **Machine-local names stay**: the runners' folders and registered names
   (`ghr\qtos`, `<COMPUTERNAME>-qtos`). They name a workstation's setup, not
   the product, and renaming them means registering the runners again.
5. QOR-0008's release gates stand, the name's clearance among them. If an
   attorney rejects "QOR OS", a later record renames it once more, by the
   same table.

## Consequences

- One name for one thing, in the code, the interface, the documents and the
  CI. `git grep -i qtos` outside the carried products finds only history and
  the carry-over.
- Build folders configured before the rename keep stale `QTOS_*` cache
  entries, which nothing reads; a fresh configure drops them. The QML output
  folder gains `QOR/` beside the old `QTOS/`, which nothing imports.
- The CI variables are `QOR_*` on `qor-os`. The frozen QTOS keeps its
  `QTOS_*` set, so its history still builds.
- Store items installed under an `org.qtos.*` ID are no longer in the
  catalogue; the `dev.qorsync.*` items install afresh.
- The decision is wrong if a name the owner prefers clears instead. The
  table above is then the checklist for the next rename.
