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
4. **The website now leads with QOR OS**, as you chose: pull request **#110**
   in demiurge-chain (ADR-123, *its decision record*), with QOR OS's mark, an
   aurora and the lens.
   It says only what ADR-123 lists: what QOR OS is, where it stands, and that
   there is no download yet. **It goes live when you merge #110.**
5. **Entire** (*a new Git host from GitHub's former chief executive*): you
   chose to mirror, with GitHub the source of truth (ADR-124 there, QOR-0012
   here), because CI runs on your licensed Qt kit through GitHub.
6. **CI's runners** (*the PCs that run the checks*) **have gone deaf**:
   online, taking no jobs since 3:36 AM. Restarted, the Windows one reconnected
   but still got no jobs; restarting the WSL one was blocked. GitHub changed
   both runners' settings at 10:48 PM on 28 October, the likely cause.

## Needs you

1. **Merge #110** to publish the site. Its failing security check was a new
   advisory (*a published library flaw*), now fixed on the branch.
2. **Entire's app can read QOR OS.** It was installed on 28 October for every
   QOR-MATRIX repository. To keep QOR-0012, limit it to demiurge-chain: the
   organisation's settings, GitHub Apps, Entire, Configure, Repository access.
3. **Re-register both runners**, or let me (CI.md says what was tried). QOR
   OS's Build has not run since; Lint ran and passed.
4. **A real graphics card for Linux testing:** a Linux boot (recommended), or
   a careful WSL trial on the NVIDIA card, which might freeze the PC again.
5. **Trademark advice for "QOR OS"** is still open, and the site uses the name
   publicly once #110 merges. Also open: the legal entity, the Qt letter, replacing the
   5 October key, moving the Store keys' backup off this PC.

## Next

Phase 1: Prism hosts the products and places their windows; QOR.Input drives
QQ; Lumen themes them; then a controller-first Home and Library.
