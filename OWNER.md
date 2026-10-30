# Where QOR OS stands

For Andrew, 30 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **QQ runs inside Prism.** Prism now starts QQ Studio (*QQ's editor*) and
   the QQ Player itself, as it already started ARQADE and QOR Media, so all
   three products run in Prism. That roadmap item is ticked. QQ Studio is in
   Beam (*the search bar*). The Player plays the game the shell hands it, one
   game at a time; the Library will be what hands it games. Publishing from
   the Studio needs the vault (*the service that will hold keys*, Phase 2), so
   for now the Studio says publishing needs the launcher.
2. **Every window opens at the size its app asks for.** Prism used to force
   960 × 640 on every app it did not place itself. Now the app chooses, and
   Prism fits the window to the screen. In a hidden Prism kept off the
   graphics card, the real Studio opened at its own 1600 × 960 and the Player
   played Orb Run at 1280 × 720.
3. **A bug found and fixed:** a window closed while minimised was never
   really removed. It stayed, unseen, in Prism's list of windows.
4. **Checked:**
   - Windows and Linux: builds and checks clean; 11 and 12 suites pass.
   - Prism's full window test, 52 checks, passed hidden.
   - QQ's own suites inside Prism: only the cases that time movement fail,
     as they do whenever QQ draws without the graphics card. Three Studio
     cases that fail on the test display passed there.
   - Not run: the shell's speed benchmark, which opens a window on your
     desktop. Nothing here draws more; say if you want it run.

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
