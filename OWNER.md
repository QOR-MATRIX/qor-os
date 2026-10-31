# Where QOR OS stands

For Andrew, 31 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design (*QOR OS's look*), with
Demiurge's QQ engine, ARQADE and QOR Media, on Linux. Code:
`QOR-MATRIX/qor-os` (private), here at `X:\Repoz\QOR-OS`.

## This session

1. **Your names are in:** Rift Delve is **Ever Rift** and Gloamreach is
   **Darkveil**, in every folder, file, card and test (decision record QOR-0019).
   In Darkveil's story the darkness is now "the veil", and the last blade
   Veilbane; that wording is mine, and easy to change.
2. **The rules did not change.** Only their words did, and the rename altered two
   lines in each recorded run: its scene and its game's name.
3. **You merged demiurge-chain #116.** The homepage shows it, and the five live
   services answered when read from outside.
4. **Earlier today:** the security bar (QOR-0018), with what is left listed in
   `SECURITY.md`; CI (*the automatic build and test run*) one job at a time on this
   laptop, after it reset itself during two full builds, the true cause unproven;
   and Darkveil, built and played by a scripted player on a real graphics card.

## Checked

CI passed on both systems at `8d93b19`, with the new names: every suite, and on
Windows all three products. Both games' 33 rules tests pass, and their recorded
runs replay in Node and in QQ. **Two things went wrong on the way.** My rename
broke QQ's build for one run, now fixed. And ARQADE passed only on its automatic
second attempt in the last two runs: once a test found the game paused mid-fight,
most likely a click taken by the real mouse, though that is not proven. I cut that
exposure; it passes hidden on Linux, and is committed here, unpushed, so it opens
no windows until the next push.

## Needs you

1. **Say the word to publish the two games' rules** to demiurge-chain, which is
   public. It is committed on a branch there, with its record ADR-129, and not
   pushed: a safety check stopped me, and it should be your call. Nothing players
   see changes.
2. **Know this about the names.** There is a small RPG called EverRift on itch.io,
   one word. Near Darkveil are *The Dark Veil: West Haven* and a *dark veil*
   prototype. A search is not trademark clearance.
3. **Play Darkveil and Ever Rift.** Nobody has watched them move or heard them.
4. **Move the Store's signing keys off this PC.** A CI job here could read them.
5. **During CI, keep the laptop on AC,** and leave the mouse and keyboard alone
   while test windows are open: they reach those windows.
6. **The Linux machine:** an SSD of 500 GB or more for USB-C and a USB stick of
   8 GB or more, then BUILDING.md's steps.
7. **Legal, still open:** trademark advice, the legal entity, the Qt letter,
   replacing the 5 October key.

## Next

The platformer, then the racer, pixel-art and arcade games; and Phase 1's goal
on the Linux machine: Orb Run in Prism with a DualSense.
