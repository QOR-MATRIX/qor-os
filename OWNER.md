# Where QOR OS stands

For Andrew, 29 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **The freezes are stopped.** Three on 28 October each came from a Linux
   test run in WSL (*Windows' built-in Linux*) using the graphics card, and a
   test stuck in its driver. Under WSL, `products.sh` now hides the card from
   the run, draws on the processor, and uses at most half the PC. 45 minutes
   of watched runs: nothing reached the card, and the PC stayed up.
2. **Results without the card:** QOR Media passes everything; ARQADE three
   suites of four (one Orbital case cannot finish a frame in software); QQ
   four of eight, with every agent-key test passing in Linux's keyring. QQ's
   timing tests fail because software runs slower than real time. A crash in
   the hidden test display was fixed.
3. **Committed and pushed** (`86dad74`). QQ on Windows passed every suite.
4. **qorsync.dev now leads with QOR OS** (*demiurge-chain's ADR-123*): you
   merged #110, it was read live from outside, and search engines were told
   (IndexNow). It says only what ADR-123 lists, including "no download yet".
5. **Entire** (*a new Git host from GitHub's former chief executive*): its
   app stays on every repository, QOR OS included, as you allowed (QOR-0013);
   GitHub stays the source of truth.
6. **CI's runners** (*the PCs that run the checks*) **take no jobs** since
   3:36 AM, though online. Re-registered at your word (`runners.ps1
   -Reregister`), they connected and still got none in 45 minutes. Labels,
   group, permissions, billing and GitHub's status are all in order.

## Needs you

1. **Ask GitHub Support why the runners get no jobs** (CI.md lists what was
   ruled out). QOR OS's Build has not run since 28 October; Lint passes.
2. **Merge demiurge-chain #111**, records of #110 only.
3. **Entire's session recording** (*saving AI transcripts beside commits*) is
   still undecided: say if "in full" includes it.
4. **A real graphics card for Linux testing:** a Linux boot (recommended), or
   a careful WSL trial on the NVIDIA card, which might freeze the PC again.
5. **Trademark advice for "QOR OS"** is still open, and the site now uses the
   name publicly. Also open: the legal entity, the Qt letter, replacing the
   5 October key, moving the Store keys' backup off this PC.

## Next

Phase 1: Prism hosts the products and places their windows; QOR.Input drives
QQ; Lumen themes them; then a controller-first Home and Library.
