# Where QOR OS stands

For Andrew, 29 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **Why the PC froze.** It froze three times on 28 October (1:04, 3:08 and
   7:20 PM), each during a Linux test run in WSL (*Windows' built-in Linux*)
   that drew on the graphics card through Direct3D 12. Each run's log stops
   as a QQ suite starts, and two stayed silent far past the 15-minute limit
   that stops a stuck test. So a test was stuck inside WSL's graphics driver,
   where nothing can stop it, and Windows hung later. The 7:20 run was one
   the previous session started in the background and left.
2. **Linux runs can no longer reach the graphics card.** Under WSL,
   `tools/dev/products.sh` now runs itself in a *namespace*, a private view
   of the system, where WSL's graphics device is replaced by an empty one.
   Everything draws in software, on the processor. It also uses at most half
   the processors and three quarters of WSL's memory. A watcher checked every
   2 seconds through 45 minutes of runs, and no test reached the card. The PC
   stayed up through every run today. `--gpu` brings back the old way.
3. **Results in software:**
   - QOR Media passes everything.
   - ARQADE passes 3 suites of 4. One Orbital game case draws a frame that
     software could not finish in 30 minutes, so the 35 game cases after it
     cannot run.
   - QQ's agent key now lives in Linux's keyring (*the Secret Service*), and
     every key test passes. QQ Studio's F3 overlay, Group and Duplicate fail,
     as before. In software QQ runs slower than real time, so the tests that
     time movement fail. That is expected, and the tests were left as they are.
4. **A crash fixed in the hidden test display:** it died whenever a window
   closed while another was open.
5. **QQ on Windows passes every suite** after the keyring change. That run
   opened windows on your desktop without warning. That was my mistake: QQ's
   build script runs its tests too.
6. Your two choices of yesterday are records QOR-0010 and QOR-0011.

## Needs you

1. **A graphics card for Linux testing that does not hang.** Software can't
   pass ARQADE's Orbital case or QQ's timing tests, and Linux CI stays off
   (`QOR_RUN_LINUX_PRODUCTS`). Options:
   - a real Linux boot on this PC or another one (recommended: Phase 1's exit,
     Orb Run in Prism with a DualSense, needs real Linux anyway);
   - a careful WSL trial on the NVIDIA card, which might freeze the PC again.
2. **Windows check of ARQADE and QOR Media** (`products.ps1`, about ten
   minutes, with windows on your desktop). It completes this change. Say when.
3. **Still open from before:** which legal entity; trademarks for QQ, ARQADE
   and "QOR OS"; the Qt letter ([docs/release/](docs/release/README.md));
   replacing the key exposed on 5 October; moving the Store keys' backup off
   this PC (it is still in `~/.qor-os`).

## Next

Phase 1, in order: Prism hosts the products (it speaks their launcher protocol
and places their windows); QOR.Input drives QQ's controls; Lumen themes them;
then a controller-first Home and Library. Find out why F3 fails on Linux.
