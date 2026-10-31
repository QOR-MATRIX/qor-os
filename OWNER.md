# Where QOR OS stands

For Andrew, 31 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design (*QOR OS's look*), with
Demiurge's QQ engine, ARQADE and QOR Media, on Linux. Code:
`QOR-MATRIX/qor-os` (private), here at `X:\Repoz\QOR-OS`.

## This session

1. **The security bar** (decision record QOR-0018). Machines now check exploit
   mitigations (*defences compiled into every program*) and what CI (*the
   automatic build and test run*) trusts, and look for secrets. Two independent
   reviews found serious holes, now fixed: any local user could reach the
   service bus (*how apps talk to system services*), a game package could run
   native code, and a project from elsewhere ran in QQ Studio unasked. What is
   left is listed in `SECURITY.md` under Known limits.
2. **CI runs safely on this laptop**, one job at a time and held to 8 compilers.
   It reset itself during two full builds at once (Kernel-Power 41, no dump).
   The load is my doing and is fixed; **the true cause is not proven.** The
   crash left empty build files behind, so CI now clears those first.
3. **Gloamreach** (a working title), ARQADE's new 3D action RPG: a valley, the
   village of Hearth with four people and their errands, three wardens over
   three beacons, and the Hollow King behind a gate. A chain of sword swings, a
   roll, spells, levels, gold and a smith. A scripted player wins it in about
   8 minutes; a person will take far longer. It is rewarded (*ARQADE's server
   can replay a run to confirm it*), and its rules keep the world apart from
   each player, ready for the online world later.

## Checked

CI passed on both systems at `2096075`, which holds all of this: every suite, the
sanitizers (*memory-error detectors*) and, on Windows, all three products. There,
on a real graphics card, a scripted player talks, fights to Gloamreach's first
beacon and falls, and a run QQ recorded replays to the same result in Node. Its
19 rules tests pass, and seven faults planted in its rules are caught.

## Needs you

1. **Play Gloamreach**, on ARQADE's sixth card. Tests have played it, and I have
   seen screenshots, but nobody has watched it move or heard it. How it feels,
   and whether it is too easy, wants a person.
2. **Name the games.** "Rift Delve" and "Gloamreach" are working titles; a
   search found no game of either exact name, which is not trademark clearance.
   Their rules go to demiurge-chain only once you approve names.
3. **Move the Store's signing keys off this PC.** A CI job here could read them.
4. **Keep the laptop on AC during CI.** A low battery ended one run too.
5. **The Linux machine:** an SSD of 500 GB or more for USB-C and a USB stick of
   8 GB or more, then BUILDING.md's steps.
6. **Merge demiurge-chain #116**: the homepage's QOR OS facts.
7. **Legal, still open:** trademark advice, the legal entity, the Qt letter,
   replacing the 5 October key.

## Next

The platformer, then the racer, pixel-art and arcade games; and Phase 1's goal
on the Linux machine: Orb Run in Prism with a DualSense.
