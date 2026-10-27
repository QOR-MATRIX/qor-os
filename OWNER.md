# Where QOR OS stands

For Andrew, 26 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: QTOS's Prism
shell — *the compositor and desktop in one* — and Lumen design, joined with
Demiurge's QQ engine, ARQADE and QOR Media, on a Linux base. Code:
`github.com/QOR-MATRIX/qor-os` (private), on this PC at `X:\Repoz\QOR-OS`.

## This session (all checked in the tree today)

1. **Space:** the build drive X: went from 9.6 GB to 204.7 GB free. I removed
   Demiurge's Rust build output and three finished agent worktrees, *spare
   copies of the repository*, whose work was already merged.
2. **The repository:** `qor-os` holds QTOS's full history. QTOS is frozen with
   a note pointing here.
3. **The products came with their history:** QQ, ARQADE (app, games and
   fixtures) and QOR Media, at the same paths as in demiurge-chain. In CI all
   their suites passed first time: QQ 8, ARQADE 4, QOR Media 9. On this PC,
   QOR Media's reader suite failed now and then while the desktop was busy,
   because a test window lost the keyboard; CI retries a failure once.
4. **The QOR core** — *the Rust code that holds keys* — is built from
   demiurge-chain at a pinned commit and never copied. Its test passes on
   Windows and Linux. Moving the rest of the launcher's Rust into it (QOR ID,
   chain, Social, Library, Mesh, Qontrol) is done in demiurge-chain #107,
   waiting for you.
5. **Eight decisions** written down (`docs/decisions/`):
   - one rulebook;
   - the repository layout;
   - the core pinned;
   - **Fedora Atomic** as the base;
   - **every display at its own refresh on every GPU**, with the display test
     deferred, not failed;
   - **no money in 1.0**;
   - what 1.0 ships;
   - the release gates.
6. **CI is green:** both runners on this PC now serve `qor-os`, and the first
   run passed on Windows and Linux, products included (25 minutes; later runs
   are incremental). WSL's clock was 6 h 25 min fast, which stopped the Linux
   runner; the runner script now fixes it.

## Needs you

1. **demiurge-chain #107** — *the rest of the launcher's Rust moved into the
   core* (L13.2). It also records ADR-120, QOR OS from Demiurge's side.
   - First decide **ADR-121** in it. Option A, recommended, ignores one
     advisory in a crate that is listed but never built, guarded by CI exactly
     as the launcher already does.
   - Then review and merge #107. Merging to that public `main` is yours; I
     was not allowed to.
   - After the merge I move QOR OS's pin, with the `desktop` feature.
   - ADR-120 also asks which licence the core's crates carry: MIT, as their
     manifests say, or Apache-2.0, as the repository is.
2. **Share the runners with QTOS too:** run
   `gh auth refresh -h github.com -s admin:org`, then
   `.\tools\ci\runners.ps1 -Scope org` in `X:\Repoz\QOR-OS`.
3. **Close QTOS's phase issues #2–#6** with a note pointing to QOR OS. I was
   not allowed to.
4. **The release gates**, before anything is public:
   - The Qt Company's written OK to ship Qt in an OS image;
   - a trademark attorney for "QOR OS";
   - the legal list in Demiurge's `OWNER.md`.

## Next

Phase 1: QQ, ARQADE and QOR Media built on Linux and running inside Prism,
driven by a DualSense. Then sign-in and keys (Phase 2), then booting on the
G15 with Steam (Phase 3).
