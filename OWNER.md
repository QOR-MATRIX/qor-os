# Where QOR OS stands

For Andrew, 29 October 2026 (evening). One page, rewritten every session
(`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **The freezes are stopped.** Linux test runs in WSL (*Windows' built-in
   Linux*) no longer touch the graphics card and use at most half the PC.
   Without the card, QOR Media passes everything, ARQADE three suites of four
   and QQ four of eight; QQ's timing tests fail because software is slow.
2. **qorsync.dev leads with QOR OS** (*demiurge-chain's ADR-123*), live since
   you merged #110.
3. **Entire is not used** (QOR-0014); you removed its app and merged #111
   and #112, its records.
4. **CI works again.** Its runners (*the PCs that run the checks*) are now
   registered to `qor-os`, and every Build since has passed.
5. **QQ, ARQADE and QOR Media are now developed here** (QOR-0015), as you
   chose. Their copies in demiurge-chain are *frozen* (kept, not worked on)
   since you merged #113, its ADR-126, and the Windows launcher keeps shipping
   them. New work on them is
   proprietary until the public SDK's record decides about QQ's source. The
   374 files that came from demiurge-chain stay Apache-2.0 (*the open
   licence*), and a check now tells old files from new. ARQADE's five rule
   files stay the same in both repositories, because ARQADE's server judges
   runs by them.

## Needs you

1. **A real graphics card for Linux testing:** a Linux boot (recommended), or
   a careful WSL trial on the NVIDIA card, which might freeze the PC again.
2. **Legal, still open:** trademark advice for "QOR OS", which the site now
   uses publicly; the legal entity; the Qt letter. When a lawyer reads the
   list, add the new paragraph in `LICENSE` that makes the products' new work
   proprietary. Also open: replacing the 5 October key, and moving the Store
   keys' backup off this PC.

## Next

Phase 1: Prism hosts the products and places their windows; QOR.Input drives
QQ; Lumen themes them; then a controller-first Home and Library.
