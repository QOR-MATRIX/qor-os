# Where QOR OS stands

For Andrew, 29 October 2026 (night). One page, rewritten every session
(`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **The freezes are stopped.** Linux test runs in WSL (*Windows' built-in
   Linux*) no longer touch the graphics card and use at most half the PC.
2. **Housekeeping.** qorsync.dev leads with QOR OS (#110). Entire is not used
   (QOR-0014; #111 and #112). CI's runners (*the PCs that run the checks*)
   are registered to `qor-os`, and its builds pass again.
3. **QQ, ARQADE and QOR Media are developed here** (QOR-0015). You merged
   demiurge-chain #113, which *freezes* their copies there (kept, not worked
   on); the Windows launcher keeps shipping those. The 374 files that came
   from demiurge-chain stay Apache-2.0 (*the open licence*); new work on them
   is ours. ARQADE's five rule files stay the same in both repositories.
4. **Prism hosts ARQADE and QOR Media.** Prism now starts them itself and
   speaks the same private protocol as the Windows launcher: a channel only
   the program it started can use, and a one-time password on that program's
   input. Prism lays out their windows by its own plan and names them from
   its own records. ARQADE is in Beam (*the search bar*) where it is
   installed; QOR Media opens a file Prism gives it, ready for the Library.
   - Checked on Windows and Linux, and in Linux CI with stand-ins, in a hidden
     Prism. The real ARQADE and QOR Media also ran in a hidden Prism, off the
     graphics card.
   - Not run: the shell's speed benchmark, which opens a window on your
     desktop. This change adds no drawing work; say if you want it run.

## Needs you

1. **A real graphics card for Linux testing:** a Linux boot (recommended), or
   a careful WSL trial on the NVIDIA card, which might freeze the PC again.
2. **Legal, still open:** trademark advice for "QOR OS", which the site now
   uses publicly; the legal entity; the Qt letter. When a lawyer reads the
   list, add the new paragraph in `LICENSE` that makes the products' new work
   proprietary. Also open: replacing the 5 October key, and moving the Store
   keys' backup off this PC.

## Next

Phase 1: QQ Studio and Player in Prism at their own size; the controller
service drives QQ; Lumen themes the products; then a controller-first Home
and Library.
