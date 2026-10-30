# QOR-0017: ARQADE's new games, rewarded from the start, big ones first

- **Status:** Accepted
- **Date:** 2026-10-30
- **Deciders:** the owner, who asked for "a number of games to our arsenal in
  Arqade. Mario style platformer games. 3D RPG games, MMORPG. Racing. Pixel
  based games. Dungeon hack-n-slash rpg like Diablo. Arcade style games. All
  built with our QQ Game Engine", and chose from the assistant's questions:
  rewarded from the start, the big ones first, and the RPG first with the
  online world later. The rest is the assistant's.
- **Amends:** [QOR-0015](QOR-0015-the-products-are-developed-here.md)
  decisions 2 and 3, which name five shared rule modules: every new game's
  rules module, Node test and fixture are shared too (decision 3 below)
- **Relates to:** [QOR-0006](QOR-0006-qor-os-1-0-moves-no-money.md) (1.0
  moves no money), [QOR-0009](QOR-0009-the-name-is-qor-os.md) (names);
  Demiurge's ADR-087 (a game reaches nothing outside its package) and ADR-102
  (rules in one place; P7.22, ARQADE's service confirming runs)

## Context

- **ARQADE has six games**: four solo games (Orbital, Void Runner, Synapse,
  Rift Survivor) and two arenas (Flux Four, Rift Reversi). Each is a QQ package
  (`products/arqade/games/README.md`): a scene, a logic that draws, and the
  rules in one module, `logic/rules.mjs`, which ARQADE's server will replay to
  confirm a run (P7.22, not built yet). The rules keep to arithmetic every
  machine computes alike, and draw their chance from the run's seed.
- **The owner asked for seven genres**, named after well-known games. They are
  taken as genres. Each game is original: its own world, names, characters,
  levels, art and sound, and nothing taken from the games named.
- **What QQ gives.** 3D and 2D scenes, pixel art, pooled shapes and props,
  lights, particles, placed sound, screens, actions from the keyboard, pointer
  and controller, and a run that replays to its outcome. It has no vehicle
  physics, path-finding or real-time networking. Its physics (Qt Quick 3D
  Physics) is not the same on every machine, so a rewarded game cannot let it
  decide anything.
- **The big genres are big.** A dungeon hack-and-slash and an action RPG each
  need maps, combat, enemies that think, loot and growth. The online world
  needs a server, accounts and the sign-in, which is Phase 2.

## Decision

1. **Seven original games, by genre.** A dungeon hack-and-slash; a 3D action
   RPG; a platformer; a racer; a pixel-art game; an arcade game; and an online
   world grown from the RPG.
2. **The big ones first.** The dungeon hack-and-slash first, then the 3D RPG,
   then the platformer, the racer, the pixel-art game and the arcade game.
   Each lands whole before the next begins:
   - its package (rules, logic, scene, `make_assets.py` and its assets);
   - its card on ARQADE's home and its place in Prism's Library;
   - its cases in `tst_games`, and a run recorded as a fixture;
   - its Node test.
3. **Rewarded from the start.** Every new game is built as ARQADE's games are,
   so ARQADE's service can confirm its runs once P7.22 is built.
   - **The rules decide everything that counts**: movement, collisions,
     combat, chance and score, in `logic/rules.mjs`, in whole steps of 1/60 s,
     with the arithmetic the games' README allows. QQ's physics bodies are
     never used for these. QQ only draws and sounds what the rules say.
   - **Shared like the five.** Each new game's `logic/rules.mjs`, its Node
     test (`products/arqade/tests/<game>.test.mjs`) and its fixture go to
     demiurge-chain, where ARQADE's server lives, in one pull request, and
     stay identical in both places from then on. They are under Apache-2.0
     and say so in their first line (`SPDX-License-Identifier: Apache-2.0`).
     The game's scene, logic and assets are QOR OS's, under its `LICENSE`.
   - **Tested here too.** The Node tests of the games born here run in QOR
     OS's lint workflow, so a change to their rules fails here before it
     crosses.
   - **No money.** Rewarded means a run can be confirmed. What a confirmed run
     earns is not decided here, and QOR OS 1.0 moves no money (QOR-0006).
4. **The RPG first, the online world later.** The 3D RPG is single-player.
   Its rules keep a world's state apart from each player's, stepped by each
   player's input, so a server could run the same module for many players.
   The shared world's server is built in demiurge-chain, with the network,
   after Phase 2's sign-in, under a record of its own.
5. **Names wait for the owner** (`AGENTS.md` §8). Each game has a working
   title in this private repository. Nothing about it is published, upstream
   or anywhere else, until the owner names it. The first is **Rift Delve**,
   the dungeon hack-and-slash. Its working title was chosen after a search
   found no game of that name, which is not trademark clearance.

## Consequences

- ARQADE's home and Prism's Library grow by one card with each game.
- The new games reach people only through QOR OS's release. The Windows
  launcher ships ARQADE as it was frozen (QOR-0015 decision 5), and
  ARQADE's website plays the games it has.
- A rules module for a big game is larger than any of the five, and its
  arithmetic is held to the same limits: maps, path-finding and combat are
  written for the rules, not borrowed from QQ.
- Every game crosses to demiurge-chain once, when it is named, and again
  with each change to its rules.
- **A sign this is wrong:** a genre whose rules cannot be kept to that
  arithmetic, or that runs too slowly stepped in JavaScript. Then a later
  record lets that game go unrewarded, or moves its rules to the core.
