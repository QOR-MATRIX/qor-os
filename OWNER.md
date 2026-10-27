# Where QOR OS stands

For Andrew, 27 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
— *the compositor and desktop in one* — and Lumen design, joined with
Demiurge's QQ engine, ARQADE and QOR Media, on a Linux base. Code:
`github.com/QOR-MATRIX/qor-os` (private), on this PC at `X:\Repoz\QOR-OS`.

**Phase 0 is complete.** Next is Phase 1: QQ, ARQADE and QOR Media on Linux,
inside Prism, driven by a controller.

## This session

1. **The QOR core moved to your merge of #107** (`e99c7ce`) with every module
   the Windows launcher has: QOR ID's and Social's clients, the chain client,
   the Library's index, the Mesh and Qontrol. CI passed on Windows and Linux.
   I also fixed a trap: CI reuses its build folders, which would have kept the
   old core. A moved pin now always takes effect.
2. **One name: QOR OS**, as you asked, in the interface, the documents and the
   code ([QOR-0009](docs/decisions/QOR-0009-the-name-is-qor-os.md)).
   - Programs are `qor-shell`, `qorctl` and `qor-simd`; code is `qor`; QML
     modules are `QOR.Kit`, `QOR.Shell` and so on, beside the carried
     `QOR.Media`; settings are `QOR_*`.
   - The Store's items are `dev.qorsync.*`, on the ecosystem's real domain,
     and the catalogue is the "QOR OS Store".
   - "QTOS" now means only the predecessor: its frozen repository and its
     history.
   - Nothing of yours is lost. The shell and simulator copy your settings and
     installed Store items from the old names the first time they run. The
     Store was re-signed with **your existing key**, so its root of trust is
     unchanged.
3. **The runners serve the whole organisation.** You moved them. A second run
   of the script stopped the Windows runner mid-job; I restarted it, and the
   script now refuses to touch a busy runner and skips one already in place.
   That interrupted job was QTOS's last build, so I reran it and QTOS ends
   green.
4. **QTOS is tidy:** its phase issues #2–#6 and milestones are closed, each
   pointing to where its work went in QOR OS.

## Needs you

1. **Move the Store's signing keys when convenient** (optional): rename
   `C:\Users\Godmode\.qtos\store-signing` to
   `C:\Users\Godmode\.qor-os\store-signing`. Back them up first. The tools use
   either. `developer-org.qtos.pem` in there is no longer used.
2. **Two loose ends in demiurge-chain**, by your rules there:
   - ADR-121's file still says "Proposed", although #107 merged with its
     option A. Its next records update should mark it accepted.
   - The core's crates say MIT while the repository is Apache-2.0
     (ADR-120's open question).
3. **The release gates**, before anything is public:
   - The Qt Company's written OK to ship Qt in an OS image;
   - a trademark attorney for "QOR OS";
   - the legal list in Demiurge's `OWNER.md`.

## Next

Phase 1: QQ, ARQADE and QOR Media built on Linux and running inside Prism,
driven by a DualSense. Then sign-in and keys (Phase 2), then booting on the
G15 with Steam (Phase 3).
