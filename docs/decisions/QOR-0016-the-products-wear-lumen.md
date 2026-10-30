# QOR-0016: The products wear Lumen

- **Status:** Accepted
- **Date:** 2026-10-30
- **Deciders:** the owner, who answered "Proceed." to the assistant's
  recommendation to give the products "the Lumen look" as Phase 1's next step;
  the mapping below is the assistant's
- **Amends:** [QOR-0015](QOR-0015-the-products-are-developed-here.md) decision 2,
  on `docs/design/tokens.json`: it stays copied (decision 5 below)
- **Relates to:** [QTOS-0009](qtos/QTOS-0009-oklch-tokens-single-source.md)
  (Lumen's tokens, one source), [QOR-0010](QOR-0010-on-wayland-prism-places-the-products-windows.md)
  (Prism hosts the products), [QOR-0015](QOR-0015-the-products-are-developed-here.md)
  (which keeps copying `docs/design/tokens.json` "until Phase 1 gives the
  products' look one source in Lumen"); Demiurge
  [ADR-108](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-108-one-look-obsidian-and-gilt-in-two-registers.md)
  (Demiurge's one look)

## Context

- **Two looks.** QOR OS's shell and apps wear Lumen
  (`kit/tokens/lumen.tokens.json`): light or dark, an accent the person
  chooses, a tint from the Store's themes, and Increase Contrast. QQ, ARQADE
  and QOR Media wear Demiurge's one look (`docs/design/tokens.json`): obsidian
  grounds, parchment inks, an ember accent and gilt. VISION and the roadmap
  say Phase 1 maps the second onto the first's roles, so the system has one
  source.
- **What the products can take.** They are themed by their host already.
  - ARQADE and QOR Media take a `theme` command over the launcher's host
    protocol, by the names of Demiurge's look ("void", "ink", "accent"...).
    Prism listed `theme` among the commands they understand, but sent none.
  - QQ has no channel in Prism. Its `Look` holds the look as constants.
- **Contrast.** Demiurge's look holds every ink to 4.5:1 on every ground.
  - Lumen's three readable text roles give that, except its tertiary text on
    its raised surface: 4.08:1 at worst.
  - Demiurge's ink on the accent is dark; Lumen's is white. A primary button
    that brightened its fill when lit would leave white ink unreadable on
    Lumen's light accent text.
- **The stage.** ARQADE's portal and emblems are lit for a dark sky, and QOR
  Media plays films and shows pages. Lumen's light appearance would give them
  light grounds.

## Decision

1. **One mapping, in Lumen's source.** `kit/tokens/lumen.tokens.json` gains
   `products.colors`: each of the 21 colours of Demiurge's look, by its name,
   and the Lumen role it takes. `tools/tokens/generate_tokens.py` writes it
   into `Tokens.products` and into `docs/design/tokens.md`.

   | Demiurge's colours | Lumen role |
   |--------------------|------------|
   | `void` | `background` |
   | `base`, `well` | `surfaceSunken` |
   | `surface` | `surface` |
   | `raised` | `surfaceRaised` |
   | `edge` / `edge-soft` | `windowStroke` / `separator` |
   | `ink` | `textPrimary` |
   | `ink-body`, `ink-muted` | `textSecondary` |
   | `ink-faint`, `gilt-dim` | `textTertiary` |
   | `accent` / `accent-dim` / `on-accent` | `accent` / `accentHover` / `onAccent` |
   | `accent-bright`, `gilt`, `gilt-bright` | `accentText` |
   | `ok` / `warn` / `bad` | `positive` / `warning` / `critical` |

   The gilt has no role of its own in Lumen, so it takes the accent's text.

2. **Dark, with the person's choices.** The products' colours are those roles
   resolved dark, whatever the shell's appearance, with the person's accent,
   tint and contrast as set, not as they animate (`Theme.productColors`).

3. **Prism gives them.**
   - Every product is given the colours as it starts, as `QOR_THEME_COLORS`, a
     JSON object, beside `QOR_REDUCED_MOTION`.
   - A product that follows `theme` (ARQADE, QOR Media) is sent them as it
     connects and whenever they change.
   - ARQADE takes every colour, the gilt and the status colours included.
     QOR Media takes its on-accent ink too.
   - QQ's `Look` takes them as the runtime starts. That covers QQ Studio, the
     QQ Player, and ARQADE's games' screens.

4. **Contrast is tested.** In `tests/tools/test_generate_tokens.py`, at every
   accent preset, with and without Increase Contrast:
   - every ink, the bright accent, the gilt and the status colours read at
     4.5:1 on every ground;
   - the only exception is the faintest ink on the raised ground, held to
     4.0:1, as Lumen's own tertiary text is there;
   - the on-accent ink reads at 4.5:1 on the accent and its dim.

   A lit primary button in ARQADE and QOR Media keeps its fill and lights its
   edge instead.

5. **What stays.**
   - Outside QOR OS (the Windows launcher's frozen copies, a product started
     by hand), the products keep Demiurge's look, from `docs/design/tokens.json`.
     It stays copied for those defaults and for the products' tests.
   - The faces stay Cinzel and Barlow until a later record.

## Consequences

- In Prism the products follow the person's accent, and their gilt with it.
  With the orange preset (hue 58), ARQADE's accent is `#b15f00`, darker than
  its ember `#ff6a00` so that white reads on it, and its bright accent is
  `#f8aa6d`, near its own `#ff9142`. With the default (274), both are blue.
- A colour the products add to their look must be mapped here, or the token
  test fails.
- A product that changes its look's names breaks the mapping: the test
  compares the mapping with `docs/design/tokens.json`'s colours.
- QQ and ARQADE's games take a change of theme only when next started; ARQADE
  and QOR Media take it at once.
- **Left:** Lumen's faces and type scale for the products, and Lumen's light
  appearance, should either be wanted.
- **It would be wrong** if the products' text read worse than the shell's, or
  if a theme the person chose left a product unreadable. The contrast test is
  how that would show first.
