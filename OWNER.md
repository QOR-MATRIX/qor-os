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
5. **Entire is not used** (QOR-0014), on my recommendation, which you asked
   for: it duplicated GitHub, had no CI for your licensed Qt kit, and its app
   could write to this private repository. You removed the app; GitHub alone,
   as before. You also merged demiurge-chain #111 and #112, its records.
6. **CI works again.** Its runners (*the PCs that run the checks*) had taken
   no jobs since 3:36 AM, even re-registered: GitHub had stopped giving jobs
   to organisation-wide runners. Registered to `qor-os` instead, they took the
   waiting Build within 15 seconds, and it passed on Windows and Linux; that
   is now the script's default.

## Needs you

1. **A real graphics card for Linux testing:** a Linux boot (recommended), or
   a careful WSL trial on the NVIDIA card, which might freeze the PC again.
2. **Trademark advice for "QOR OS"** is still open, and the site now uses the
   name publicly. Also open: the legal entity, the Qt letter, replacing the
   5 October key, moving the Store keys' backup off this PC.

## Next

Phase 1: Prism hosts the products and places their windows; QOR.Input drives
QQ; Lumen themes them; then a controller-first Home and Library.
