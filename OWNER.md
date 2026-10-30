# Where QOR OS stands

For Andrew, 30 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **QQ runs inside Prism.** Prism now starts QQ Studio (*QQ's editor*) and
   the QQ Player, as it does ARQADE and QOR Media; that roadmap item is
   ticked. The Studio is in Beam (*the search bar*); the Player plays the game
   the shell hands it, one at a time. Publishing waits for the vault (*the
   service that will hold keys*, Phase 2).
2. **Every window opens at the size its app asks for**, fitted to the screen,
   instead of a forced 960 × 640. The real Studio opened at its own
   1600 × 960 and the Player played Orb Run at 1280 × 720, in a hidden Prism.
3. **The speed benchmark, as you asked:** three runs before this work and
   three after, on this PC. The same within noise: 2.78 and 2.79 ms a frame,
   about 358 frames a second, as on 23 October.
4. **QQ Studio's errors, fixed:**
   - The Agent panel lacked one import, so its colours and fonts failed and
     the Studio printed `Look is not defined`. QQ's tests now fail on any
     such error.
   - Duplicate, Group and the F3 overlay failed in the Linux tests: the tests
     pressed the keys before the Studio's window had the keyboard. They now
     wait for it, and pass.
   - Left: on Linux without the graphics card, QQ runs slower than real time,
     so the 7 tests that time movement fail. A real graphics card fixes that.
5. **Also fixed:** a window closed while minimised stayed, unseen, in Prism.

Checked on Windows and Linux: builds and checks clean, QOR OS's own suites
passing; Prism's window test (52 checks) hidden. Pushing the QQ fix runs the products'
Windows tests, which open windows on your desktop for about 10 minutes.

## Needs you

1. **A real graphics card for Linux testing:** a Linux boot (recommended), or
   a careful WSL trial on the NVIDIA card, which might freeze the PC again.
2. **Legal, still open:** trademark advice for "QOR OS", which the site now
   uses publicly; the legal entity; the Qt letter. When a lawyer reads the
   list, add the new paragraph in `LICENSE` that makes the products' new work
   proprietary. Also open: replacing the 5 October key, and moving the Store
   keys' backup off this PC.

## Next

Phase 1: the controller service drives QQ, so Orb Run plays in Prism with a
DualSense (*the PlayStation 5 controller*); Lumen themes the products; then a
controller-first Home and Library.
