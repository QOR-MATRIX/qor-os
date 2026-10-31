# ARQADE's games, rebuilt in QQ

Changed for QOR OS: Ever Rift and Darkveil, the first of QOR OS's own games (its QOR-0017), and the left stick's four
ways among the pad's bindings.

ARQADE's four solo games and its two arenas are rebuilt as QQ games (ADR-102 decision 2; DIRECTION P7.20 and P7.24).
They are played in ARQADE's Qt app ([`../qt/`](../qt/README.md)) by QQ's runtime ([`products/qq/`](../../qq/README.md)).
Each game here is a QQ package, laid out as QQ Studio keeps a project, so it also opens in QQ Studio and plays in the QQ
Player as it is (an arena there has nobody to ask, so it stays in its lobby).

| Game | State |
| --- | --- |
| Orbital | Rebuilt, 24 October 2026: plays in ARQADE, its rules replay a run in QQ and in Node |
| Void Runner | Rebuilt, 24 October 2026: the same |
| Synapse | Rebuilt, 24 October 2026: the same |
| Rift Survivor | Rebuilt, 24 October 2026: the same |
| Flux Four and Rift Reversi | Rebuilt, 24 October 2026: play in ARQADE against another person, on ARQADE's server; a run replays in QQ and in Node |
| Ever Rift | QOR OS's own, 30 October 2026: plays in ARQADE, its rules replay a run in QQ and in Node |
| Darkveil | QOR OS's own, 31 October 2026: the same |

## A game's package

```text
orbital/
  scenes/orbital.qml   the scene: sky, light, the core, the orbit's group, the screens and the actions
  logic/orbital.qml    the logic: hands each event to the rules, and draws what they say
  logic/rules.mjs      the rules, and nothing else: the one place they live
  assets/              the sky (an HDR image) and the sounds, written by make_assets.py
  make_assets.py       writes assets/ with the standard library only, the same bytes every time
```

The two arenas are one package, `arenas/`, with a scene for each (`scenes/flux-four.qml`, `scenes/rift-reversi.qml`),
one logic that draws either board (`logic/arena.qml`), and in place of rules their conversation with ARQADE
(`logic/arena.mjs`).

ARQADE carries each package in its executable, at `qrc:/games/<name>/` (`../qt/CMakeLists.txt`), so the scene's paths,
which are relative, mean the same there as on disk. Each package keeps its own rules whole, random numbers and all: a
game reaches nothing outside its package (ADR-087 decision 2), so two games never share a file.

## Rules in one place

A game's rules are a plain JavaScript module, `logic/rules.mjs`, with no QQ or Qt in it. The game's logic hands it each
event as QQ's tick delivers it (a screen button's click, an action's press, a step of 1/60 s) and draws what the rules
say. Nothing the logic draws decides anything.

So a run QQ records (its seed, and what each tick took) is the rules' input exactly. The rules' `replay(run)` plays a
run file of QQ's (`qqRun` 1) to its outcome, in Node or anywhere else that runs JavaScript. That is what ARQADE's
service will run before it confirms a run and pays for it (ADR-102 decision 5; DIRECTION P7.22, not built yet).

The rules use only arithmetic every machine computes alike: adding, subtracting, multiplying, dividing, `Math.floor`
and whole numbers. No sine, cosine, power or logarithm decides anything, because each system's maths library may differ
in its last bits. Their random numbers come from the run's seed through xoshiro128**, in 32-bit whole numbers. A test
fails if the rules' source uses one of those functions. Rift Survivor's rules also take square roots, for distances and
aim: `Math.sqrt` is IEEE 754's correctly rounded square root, which ECMAScript specifies exactly, unlike `Math.hypot`
and the trigonometric functions. Where a shot's spread turns, the cosines and sines of its angles are written in as
numbers.

**Every change to the rules shows.** A small change can leave one run's outcome as it was, so each game's test also pins
the rules' whole state at the end of its kept run, as a SHA-256 of it. Any change to the rules fails that test until the
fixture and the digest are made again. Once ARQADE's service confirms runs (P7.22), such a change is a new version of
the rules (ADR-102 decision 5), and runs kept under the old ones replay under those.

## Orbital

A satellite circles a gilt core. Lock the orbit while it crosses the amber arc: Space, Enter, the pad's A, or the Lock
button. Ten locks complete it, and each lock moves the arc and speeds the satellite up. Three misses lose it. P, or the
pad's Start, pauses.

Rebuilt from the website's 2D game in 3D: physically based gilt and emissive beads under bloom, embers rising from the
core, an HDR sky of dust and 260 stars, and stereo sound: two bell strikes for a lock, a falling tone for a miss, bells
climbing a chord when the orbit completes, three falling tones when it is lost, and a slow looping pad. With reduced
motion the core stops turning and the words do not swell. The satellite still moves, because it is the game.

## Void Runner

A ship runs a track of three lanes into the collapse on the horizon, and what the collapse throws rushes down the lanes
at it. A red hazard that reaches it in its lane breaches its hull; a gold gem is caught, for 100 points. Forty-five
seconds survived completes the run, and three breaches end it; after a breach the hull holds against the next for 0.8
seconds. The rush quickens as the run goes on. A run also scores ten points a second.

Change lane with A and D, the arrows, the pad's d-pad or shoulders, or the Left and Right buttons on the screen. P, or
the pad's Start, pauses.

Rebuilt from the website's 2D game in 3D: a dark track between ember rails whose bars run toward the eye as fast as what
is thrown, the collapse half-risen on the horizon under bloom, red slabs and turning gold, a ship with an engine's wake
of embers, an HDR sky of the collapse's light and 180 stars, and stereo sound: a bell pair for a gem, a thud for a
breach, a glancing tick when the hull holds, a rush of air for each change of lane, and a pulsing loop at 120 beats a
minute. What is thrown is drawn from two pools made as play starts, so nothing is made or removed during a run. With
reduced motion the gems do not turn, the collapse does not pulse, the hull's warning is steady rather than blinking, and
the words do not swell; the track still runs, because it is the game.

## Synapse

Four nodes of a mind, Origin, Echo, Pulse and Aether, light one after another in a signal; then the player repeats it,
node by node. Each signal repeated grows by one node, and a signal of eight repeated masters the game for 1,500 points.
One node out of turn loses it, scored 150 for each whole signal repeated and 20 for each node of the last. While the
signal shows, a press is no answer. The keys 1 to 4, the pad's X, Y, B and A (or its d-pad), or the four buttons along
the foot of the screen press the nodes; P or Start pauses.

The rules count the signal in steps: a lead of 30, then each node lit for 28 steps, 42 apart (the website's 500, 460 and
700 ms). Rebuilt in 3D: four glass nodes in a gentle arc, each on a ring of its own colour, with dendrites reaching out
and gilt filaments joining them, under an HDR sky of faint teal and violet cloud and 160 stars. A node lit, or pressed,
glows, swells and throws sparks, and sounds its note (C, E, G and C, struck glass); a signal repeated, a node out of
turn and the game mastered each have a sound, under a quiet drone. With reduced motion the nodes glow and sound but do
not swell.

## Rift Survivor

One ship in an arena for ninety seconds. Enemies come in from its edges and close on it: drones that one shot destroys,
and later armoured wardens that take three. The ship fires on the nearest by itself; the player moves it. Each kill
scores, more in a chain of kills, and leaves a fragment to collect for 15. After five seconds, rift pulses open where the
ship is, arm 1.35 seconds later and breach the hull of a ship still inside. Every so many kills the signal evolves: the
world holds while the player chooses a perk (Overclock, fire a quarter faster; Fracture, a wider burst; Second wind,
two hull back and fragments drawn from further). Four breaches end the run, ninety seconds survived completes it, and
the score adds five a second. Move with W, A, S and D, the arrows, the pad's d-pad, or the screen's Up, Left, Right and
Down; choose a perk with 1, 2 or 3, the pad's X, Y or B, or its button; P or Start pauses.

The rules are the website's, step by step, in its units (an arena of 1000 by 620): the same speeds, reaches, timings
and scores, with time counted in steps and movement taken from four held actions, pressed and released, so a replay
moves exactly as the run did. Rebuilt in 3D: the arena a dark board tilted toward the eye under a teal grid, teal drones
and ember wardens turning, white shots, gold fragments, rifts that fill as they charge and glow when armed, a ship with
an ember wake, under an HDR sky of teal spiral arms around an ember heart. Everything the rules hold is drawn from pools
made as play starts, and only pieces that stop showing are put away each frame.

**Sound from where it happens.** A sound that happens somewhere is played from there: the logic passes QQ's `play` a
place (`at`), and QQ plays it through Qt's spatial audio, heard through the eye, panned to its side and quieter further
off. Orbital's lock and miss come from the satellite, and Void Runner's from the ship's lane. Synapse's notes come from
their nodes, so a signal moves left to right as it is heard. Rift Survivor's kills come from the enemy, its rifts from
where they open, and its shots, fragments and breaches from the ship. Music and what belongs to the screen (a run's end,
a signal repeated, a perk chosen) are heard everywhere. Qt's spatial sounds take one channel and its ambient sounds two,
each warning of the other, so each `make_assets.py` writes the sounds its game places in mono and the rest in stereo.

**Colour under bloom.** QQ's tonemapping turns a glow much past 1 toward white or yellow: Orbital's amber arc washed out
to the satellite's white, and Void Runner's red hazards to white and its collapse to yellow. Each glow that carries a
meaning is kept near or under 1, which keeps its hue. Synapse's lit node is capped at 0.77 for the same reason.

## Ever Rift

QOR OS's own game, the first of the seven its QOR-0017 sets out: a dungeon hack-and-slash, three floors down into the
rift. Each floor is carved from the run's seed: up to nine rooms that keep apart, each joined to the one before by a
passage two tiles wide, and one more pair joined for a loop. The delver starts in the first room. The room furthest
from it by walking holds the stairs down, or on the third floor the warden.

- **The delver** moves three units a step (a diagonal by the half root), and slips round a corner a passage is cut
  against, as far as the step would have gone. It strikes with a sword: a swing of 18 steps that turns toward the
  nearest enemy within 96 units, plants the delver while it winds up, and strikes on its seventh step everything within
  56 units of reach (and the enemy's own half width) and 60 degrees of where it faces. Held, it swings again. Each hit
  gains fury. A **whirl** costs 35 fury and strikes everything round the delver three times. A **dash** crosses ninety
  units in ten steps, unwounded, every 75 steps. A **potion** heals 45 hundredths of its most health; it starts with
  two and carries five at most. A wound holds off the next for 20 steps.
- **The enemies** sleep until the delver comes within 260 units in their sight, and each that wakes wakes the rest
  within 170. They come straight while they see the delver, and otherwise by the floor's ways, one tile at a time
  nearer it. Each winds up a blow, glowing red as it comes, and the blow lands only on a delver still in reach: a
  **husk**; a **wisp**, fast and weaving; a **brute**, slow and heavy; a **seer**, which keeps its distance and looses
  bolts; and the **warden**, whose slam lands all round it and which calls two husks out of the rift every nine
  seconds. A floor down, they have two fifths more health and a quarter more wound.
- **What they leave**: gold, more often than not; now and then a potion; and a relic, Might (+3 power), Vigor (+20
  most health), Haste (six hundredths faster, to 130 hundredths of its speed) or Wrath (a quarter more fury a hit). Experience brings
  levels at 50, 150, 300, 500 and on, each with 12 more health, 2 more power and three tenths of its health back.
- **The end.** The warden broken clears the rift two seconds later, with a bonus for the time left; the delver's
  health spent ends the run; and the rift closes after ten minutes. Gold, falls, each floor reached (300) and the
  warden (2,500) score.

Move with W, A, S and D, the arrows, the pad's left stick or d-pad, or the screen's Up, Left, Right and Down; strike
with Space, J, the pad's A or the screen's Strike; whirl with K, E, X or Whirl; dash with L, Shift, B or Dash; drink
with Q, H, Y or Drink; P or Start pauses.

**Its rules** are written for replay as the four's are, and more of them: a floor's tiles in whole numbers, collision
against them each axis on its own, sight along a line looked at every ten units, and the way to the delver found by a
breadth-first walk over the tiles, all in the arithmetic above. A swing's arc is a dot product held against cos 60°,
written in as a number. **Rewarded from the start** (QOR-0017): its rules, its Node test and its fixture are shared
with demiurge-chain under Apache-2.0.

**In 3D.** The floor is a board tilted toward the eye that moves under it, so the delver stays in the middle: worn
flagstones, and the rock the floor is cut from built as blocks (each row's runs of rock, carried down while the row
below has the same), in rock veined with the rift's teal. There is no sun underground: a torch goes with the delver and casts the rock's shadows, and beyond it only the cavern's faint light shows the way. A pool's idle pieces are hidden, so they cost nothing to draw.
The delver is gilt with a blade held to one side, swept across the arc in a swing and round it in a whirl. Husks are
bone, wisps teal light, brutes iron with ember, seers violet, the warden crimson and ember, and the warden's slam is a
red ring growing on the floor until it lands. Gold turns, potions glow red, relics in their virtue's colour, and the
stairs are a teal ring with motes rising from it. Sound comes from where it happens: a blow from what it struck, a
fall from the fallen, a bolt from its seer, and the delver's swings, wounds and finds from the delver. With reduced
motion the eye keeps exactly to the delver, nothing on the floor turns, wisps do not bob, and the words do not swell.

## Darkveil

QOR OS's own game, the second of the seven its QOR-0017 sets out: a 3D action RPG in one hand-laid valley of 64 by 64
tiles. The three beacons went dark when the Hollow King rose behind his gate, and a warden stands over each. A hero walks
out of the village of Hearth with a rusted blade, takes errands from the people there, fights through the meadow, the
Mirewood, the Ashen Hold and the High Pass, breaks the wardens and lights the beacons. With all three burning the gate
opens. The king ended wins; a third fall ends the run; and the veil closes over the valley after three quarters of an hour.

- **The hero** moves as the view faces, the view turned an eighth at a time, so a run holds no analog input and replays
  exactly. Mud slows them. A **swing** turns toward the nearest enemy awake within 110 units and strikes within 60 degrees
  of where they face; held, or pressed in time, swings run in a chain of three, the third slow, wide and heavy. A **roll**
  crosses ground unwounded every 55 steps. A **bolt** costs mana and flies at what it can see; a **nova**, from level 3,
  strikes all round. Potions heal half their most health, and a wound holds off the next for half a second. The ward takes
  its part off every wound.
- **The enemies** wait in camps, placed from the seed: wolves, weaving shades, thornlings and bonecasters that shoot from
  afar, revenants, and a region's share more health and wound the further from Hearth. They sleep until a hero comes within
  230 units in their sight, wake their pack, come straight while they see the hero and otherwise by the valley's ways, wind
  up a blow that lands only on a hero still in reach, and go home whole when led too far. Three **wardens** slam all round
  them and call their own every ten seconds. The **great wolf** dens in the west. The **Hollow King**, hurt past half, comes
  on faster, looses a ring of eight shots with each slam, and calls revenants.
- **The people of Hearth.** Talking holds the valley. Elder Maren sets the beacons' errand. Ise rests a hero for nothing,
  sells potions, and wants five moonherbs from the Mirewood. Tolv sells blades and wards, and wants four lumps of ironroot
  from the Hold. Brann wants the great wolf dead. Answer with the keys 1 to 6, by the screen's buttons, or with up, down and
  the strike button on a pad; each errand pays in gold, score and something more.
- **A fall** wakes the hero in Hearth lighter of a quarter of their gold, with what chased them gone home.
- **The score** counts gold, falls of the enemy, each beacon (800), each errand (400), and a bonus for the time left.

Move with W, A, S and D, the arrows, the pad's left stick or d-pad, or the screen's buttons; turn the view with Z and C or
the shoulders; strike with Space, J or the pad's A; roll with L, Shift or B; bolt with K or X; nova with I or RT; drink with
Q, H or Y; talk with E, Return or LT; P or Start pauses.

**World and players apart** (QOR-0017 decision 4). `game.world` holds what is out there, `game.players` each player's own,
and every input names its player, so a server could step one valley for many. QQ plays one, and the Node test steps two: a
beacon one lights burns for both, an errand stays their own, and the valley holds only while every hero talks. The rules
are written for replay as Ever Rift's are, and **rewarded from the start**: its rules, its Node test, its pilot and its
fixture are shared with demiurge-chain under Apache-2.0. `make_world.py` lays the valley out
and writes it between the markers in `logic/rules.mjs`, with how high the land stands in `logic/land.mjs`, which only the
drawing reads, and it checks that every place can be walked to from Hearth and that the Crown cannot until the gate opens.

**In 3D.** `make_assets.py` writes everything with the standard library, the same bytes each time. The land is one mesh under
one painted picture, its rim rising into mountains so its edge is never seen. What stands on it is four flat-shaded quarters
from one small palette: each region's trees, boulders, the Hold's broken walls, Hearth's houses, bridges, the plinths and
the king's dais. People and enemies are rigged models of boxes and tapers with their own clips, played on the game's clock
by QQ's props and sped to last as long as the rules' count of what they show. Everything the rules hold is drawn from pools
made as play starts, only what is near is drawn, and an idle piece is hidden and plays no clip. The valley moves under the
eye and turns with the view, and a tall column of light stands over what to do next. Sound comes from where it happens.
With reduced motion the eye keeps exactly to the hero, the view turns at once, and nothing on the ground spins.

## The arenas: Flux Four and Rift Reversi

Two people play one board on ARQADE's server, which keeps every match and decides every move, as it does for the
website's arcade (`../lib/arena-store.ts`, `../lib/arena-engine.ts`). The arenas have no rules here. Flux Four drops
pulses into seven columns six deep, and four in a row, across, up or diagonally, win. Rift Reversi places discs on six
by six, turning over the opponent's discs each one brackets, and the most discs at the end win. A move has 90 seconds.
The server ranks every match, as it does the website's.

**The conversation.** A QQ game reaches no server of its own (ADR-087), so an arena asks its host, ARQADE's app, through
QQ's host channel (ADR-117): `askHost` to ask, `onTold` to hear. ARQADE's `GameView` answers from the server through
`ArqadeServer`, with the player's session. It answers with the arena's open lobbies from the live arcade, read every 3
seconds while the lobby shows, and with each match opened, joined, moved in, conceded or closed. It asks after the
match being followed every 1.2 seconds, as the website does, and tells the game only what changed, so the run keeps
each answer once. A match that could not be joined or opened is not followed, and the lobby is told again. `arena.mjs` is the game's side:
what it asks for each thing a player does, what it makes of each answer (never an older board than the one drawn, nor
another match's), and what the screens say. It decides no move: a move it asks for is one the server listed as legal,
and the server checks it again. It keeps no clock and draws no chance: the seconds left are counted down in QQ's steps
from the server's answer.

**Playing.** From the lobby a player opens a match, joins the newest open lobby of someone else's, or returns to their
own match. Their own lobby waits for a challenger, and can be closed before one comes. In Flux Four, left and right (A
and D, the arrows, the d-pad) choose a column and Space, Enter or the pad's A drops a pulse; the keys 1 to 7 drop one
straight into a column. In Rift Reversi, the arrows (W, A, S and D, the d-pad) move a cell at a time, Q and E (the pad's
shoulders) jump between the legal moves, and Space, Enter or A places a disc. In both, the pointer over the board aims,
and a click moves. Concede asks once more before it concedes. Lobby leaves a match while it goes on, its clock still
running. A move the server refuses says why for five seconds.

**In 3D.** Flux Four is an upright frame of wells between bronze rails on a plinth. Its pulses drop into their column
and come to rest with a bounce, and four in a row pulse when the match is won. Rift Reversi's board is teal tiles in a
pale frame: a disc set down grows into place, and each disc it turns lifts and flips over to the other colour. Both
stand nearly facing the eye, turned a little so their depth shows, and a lamp beside the eye lights them and casts their
shadows. The player's move waits over the board in their colour, the host's gold or the guest's cyan. In Flux Four it is a
pulse held over the column, bobbing, with a ring where it would land. In Rift Reversi it is a see-through disc,
breathing, with each legal cell marked. The last move holds a spark. A pulse landing, a disc set down and the discs
turning are heard from where they happen. The player's move coming round, a challenger arriving, the match's end and
the music are heard everywhere. With reduced motion, pulses arrive and discs turn at once, and nothing bobs, breathes or
pulses.

**Its run.** An arena's run is what the host told it and what the player did, and its outcome is the server's verdict
on the match: `{ match, seat, result, moves }`. `arena.mjs`'s `replay(run, game)` plays a run in Node to the same state.
Where the pointer was belongs to the window, not the run, so a replay makes nothing of a click on the board. The move it
asked for is in the server's answer, which the run keeps. An arena's run is not rewarded: the server ranks its matches.

## Tests

- `../tests/orbital.test.mjs`, `../tests/void-runner.test.mjs`, `../tests/synapse.test.mjs` and
  `../tests/rift-survivor.test.mjs` (Node, in `npm test` and CI), 9, 9, 10 and 11 tests. Each game's rules are deterministic for a seed and replay a run to the outcome of the same
  events played live. Orbital: a
  lock holds within the arc and across the turn, perfect play completes the orbit, three misses lose it. Void Runner:
  something is thrown every 38 steps and the rush quickens, a hazard in the lane breaches the hull and three end the
  run, a pilot who dodges survives forty-five seconds whole, and the lanes stop at the edges. Synapse: the signal's
  timing to the step, a press while it shows is no answer, eight signals master the game, and one node out of turn loses
  it with its score. Rift Survivor: the same run for a seed, enemies' timing and kinds, the ship's movement and the
  arena's edges, shots, chains and fragments, a still ship breached four times with the hold between, the signal evolving
  and each perk, and a pilot who keeps away lasting far longer than a still one. For each, nothing moves on the title or
  while paused, and the rules' source uses no sine, cosine, power or logarithm.
- `../tests/darkveil.test.mjs` (Node, in QOR OS's lint workflow), 19 tests, with `../tests/darkveil-pilot.js`, a pilot
  it shares with `tst_games`: the same run for a seed; the valley whole, every place walked to, and the Crown only through
  the gate; movement as the view faces, mud, a pause, and nothing walking through what stands; a swing's arc, its turn and
  its chain; a roll, a bolt and a nova; potions, the wound's hold and the ward; talking holding the valley, and nothing sold
  for too little; the errands; enemies waking, missing a hero who steps away, going home and shooting; what falls lying
  where it can be reached; beacons, the gate and the king; falls; a pilot who plays it through winning on three seeds where
  one who goes straight for the wardens falls; a still hero never found, and the veil closing; two heroes in one valley;
  and a run replaying to the outcome of the same events live. Its fixture replays, and not altered; its state is pinned;
  and its source uses none of the forbidden functions, nor a clock.
- `../tests/ever-rift.test.mjs` (Node, in QOR OS's lint workflow), 14 tests: the same floors and run for a seed;
  every floor whole, its stairs in the furthest room and the warden on the last; movement, the corner slip, walls, and
  nothing moving on the title or paused; a swing's arc and reach, fury, and a held swing again; a whirl's three blows
  and a dash's ninety units unwounded; potions; enemies asleep until they see, a blow wound up and missing a delver who
  steps away, and a seer's bolt; levels and finds; a delver with its skills clearing the rift where one that only
  swings falls; and a still delver never found until the rift closes. Its fixture replays, and not altered; its state
  is pinned; and its source uses none of the forbidden functions, nor a clock.
- Each also replays its run kept in `../tests/fixtures/`, a run QQ recorded as ARQADE's games suite played the game, to
  the outcome QQ reached; the same run with another seed, or altered (Orbital's first lock 40 ticks later, Void Runner's
  lanes never changed, Synapse's node out of turn taken away, Rift Survivor's ship never moved), reaches another. And each pins the rules' whole state at
  the end of that run.
- `../qt/tests/tst_games.cpp` (Qt Test, on the owner's PC): each card opens its game in ARQADE's window, from the
  keyboard. Orbital is played by mouse, keyboard and pad to a miss, a pause and ten locks. Void Runner is steered
  away from hazards and toward gems, changing lane by the keys, the arrows, the pad and the screen's buttons in turn,
  until each has been used and a gem caught, then steered into three hazards. Synapse is pressed while its signal shows
  (no answer), then three signals are repeated, each node pressed by the keys, the pad and the buttons in turn, then a
  node out of turn loses it for 450. Rift Survivor is moved each way there is (D, the left arrow, the d-pad's down and
  the screen's Up button), steered clear by the keys until ten kills evolve the signal, a perk chosen by its button while
  the world holds, then left standing until four breaches end it, a perk chosen by its key each time the signal evolves
  meanwhile. Ever Rift is moved each way there is (D, the left arrow, the left stick down and the screen's Up
  button), played through its first floor and down its stairs by `../qt/tests/ever-rift-pilot.js` (the Node test's pilot,
  run in the game's own engine) by the keys, the pad and the screen's Drink button, then walked among the second
  floor's enemies without a blow until the delver falls. Each game's placed sounds are checked where they happen: a lock within a unit of the satellite, a change
  of lane from a placed voice, each Synapse note at its own node, a kill from a place in the arena. The run ARQADE kept
  of each replays in QQ
  to its outcome, and so does each fixture. The launcher's reduced motion reaches the game, and Esc or Back returns to
  the home.
- `../tests/arenas.test.mjs` (Node, in `npm test` and CI), 17 tests of the arenas' conversation, against boards made
  by the server's own rules (`../lib/arena-engine.ts`). It covers the lobby kept to the arena's own matches, signed out
  saying why and asking nothing, a match opened, closed and joined, a move as the cursor and the board last seen, an
  older board or another match's never drawn, the cursor through a full column and onto the nearest legal move, a
  concession asked twice, a Space that pressed a button not moving, a refusal fading, the clock counting down, each
  result's words and outcome, and what is told read and never trusted. Both runs kept in `../tests/fixtures/` replay to
  QQ's outcomes, and not without the server's last answer. The whole state at the end of each is pinned, and
  `arena.mjs` uses no clock or chance of its own.
- `../qt/tests/tst_games.cpp` plays Darkveil too: moved each way there is and the view turned; the Elder answered by the
  screen's button; fought to its first beacon by the shared pilot, by the keys, the pad and the screen's buttons; then
  walked among what is left without a blow until a third fall ends the run. Its run replays in QQ to its outcome, and so
  does its fixture.
- `../qt/tests/tst_games.cpp` plays both arenas against a stand-in ARQADE that signs in as the website does, keeps
  matches as `arena-store.ts` keeps them and moves for the opponent. Signed out, an arena says why and offers nothing.
  Flux Four opens a match, a challenger joins, and four pulses go down one column by Space, the key 5, the pad's A and
  the pointer, each heard where it lands, to win. Then a rematch's lobby is closed before anyone joins. In Rift
  Reversi, a lobby someone else takes just before the player joins it is said to be no longer available, and the lobby
  then shows none open. Then Rift Reversi joins someone's open lobby. Its legal moves are marked, the arrows and E move the cursor, Space places a disc whose
  sound comes from where it lands, and the discs it brackets turn to the guest's colour. A move the server refuses says
  why. A disc is placed by the pointer, and the match is conceded only after Concede is asked twice. Each arena's kept
  run, and its fixture, replays in QQ to its outcome.

Faults planted to check these tests catch a change:

- Orbital's speed changed by 0.01: the fixture's replay failed in Node and in QQ (7 locks and 3 misses in place of 10
  and 1).
- Orbital's game not given the record: the run test failed.
- Void Runner's acceleration changed from 0.004 to 0.0041: the fixture's outcome did not move, which is why the state
  is pinned. The pinned state then failed, as did Orbital's for its speed changed by 0.0001.
- Void Runner's logic not handing presses to the rules: the pilot's first change of lane failed.
- Synapse's gap between nodes changed from 42 steps to 41: the pinned state failed. Its time lit changed from 28 to 27,
  which only the drawing reads: the timing test failed. Its logic not handing presses to the rules: the second signal
  never came.
- Rift Survivor's hold after a breach changed from 96 steps to 95: the pinned state failed. Its logic not handing
  releases to the rules: the left arrow did not move the ship (right was still held).
- The arenas' conversation drawing an older board, or another match's; a Space that pressed a button also moving; the
  cursor not set on a legal move; the clock at half speed; a refusal leaving the ask waiting; a call to `Date`: each
  failed `arenas.test.mjs`.
- QQ handing a host's message on through the signal's own argument, which makes its lists Qt's rather than arrays:
  QQ's play test failed (and the arenas' lobby never read as signed in). ARQADE's server posting a move without its
  origin: the server's own suite failed. ARQADE's window never asking after the followed match: the Flux Four test
  never saw its challenger join.

## Change a game

- Run it on its own: `qq-player products/arqade/games/<name>/scenes/<name>.qml` (QQ's README says how to build the
  Player), or open the folder in QQ Studio.
- Rewrite the assets: `python products/arqade/games/<name>/make_assets.py`.
- **A change to `rules.mjs` changes what every kept run replays to.** Make new fixtures with ARQADE's games suite:
  `ARQADE_RUN=<folder>` writes each run it played there, as `<name>-run.json`. Copy the game's into `../tests/fixtures/`,
  set the outcome and the digest its test expects, and say why in the commit. The same goes for the arenas'
  `arena.mjs`, whose replayed state is pinned as the rules' is.
