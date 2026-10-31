# QOR-0019: Ever Rift and Darkveil

- **Status:** Accepted
- **Date:** 2026-10-31
- **Deciders:** the owner, who wrote: "Lets name Gloamreach to "Darkveil" and Rift Delve to "Ever Rift"". What follows from
  the names is the assistant's.
- **Relates to:** [QOR-0017](QOR-0017-arqades-new-games.md) decisions 3 and 5 (names wait for the owner; a named game's rules
  cross to demiurge-chain), [QOR-0009](QOR-0009-the-name-is-qor-os.md), `AGENTS.md` §8

## Context

QOR-0017 gave each new game a working title until the owner named it, and kept it from being published until then. Rift Delve,
the dungeon hack-and-slash, and Gloamreach, the 3D action RPG, were both built under working titles.

A web search on 31 October 2026 for each name found:

- **Darkveil:** no game of that exact name. Near it: *The Dark Veil: West Haven*, a survival horror game on ModDB; *dark veil*,
  a browser prototype on itch.io; and *Veil of Darkness*, from 1993 with a 2023 release of the same name.
- **Ever Rift:** *EverRift*, one word, a fantasy RPG made in RPG Maker MV, on itch.io. Near it: *Evenrift*, also on itch.io.

A search is not trademark clearance, which `OWNER.md` lists as still open. The owner has this finding.

## Decision

1. **Rift Delve is Ever Rift. Gloamreach is Darkveil.** Those are the names in prose, the interface and metadata, and on
   ARQADE's cards and in Prism's Library.
2. **Everything else follows the name:**

   | | Ever Rift | Darkveil |
   |---|---|---|
   | Package | `products/arqade/games/ever-rift/` | `products/arqade/games/darkveil/` |
   | Scene, logic, music, sky | `ever-rift.*` | `darkveil.*` |
   | Card, and in the Library | `everrift`, `arqade:everrift` | `darkveil`, `arqade:darkveil` |
   | Node test, fixture | `tests/ever-rift.test.mjs`, `tests/fixtures/ever-rift-run.json` | `tests/darkveil.test.mjs`, `tests/fixtures/darkveil-run.json` |
   | Pilot | `qt/tests/ever-rift-pilot.js` | `tests/darkveil-pilot.js` |

3. **Darkveil's story takes its word from the name.** What was "the gloam" is "the veil": what crept down the valley, and what
   closes over it when a run's time ends. The last blade, Gloambane, is Veilbane.
4. **The rules decide what they decided.** Only words change in them, so the digests their Node tests pin stay as they were. Each
   fixture stays the run QQ recorded, with its scene and its outcome's key renamed and nothing else.
5. **Both games may now cross to demiurge-chain,** as QOR-0017 decisions 3 and 5 say a named game does. The pull request
   publishes them, so it goes on the owner's word.
6. **QOR-0017 stays as it was written.** Where it says Rift Delve, read Ever Rift.

## Consequences

- The names are public from the pull request in demiurge-chain, which is a public repository. Changing one after that is another
  rename, here and there, and the old name stays in that history.
- Trademark advice stays open, and is the owner's.
- **A sign this is wrong:** an objection from the makers of a game of that name, or advice against either.
