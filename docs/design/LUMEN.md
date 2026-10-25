# Lumen design language

Lumen is the QTOS design language: physically inspired materials,
spring-driven motion, continuous-curvature shapes, perceptual color, and calm
typography. This specification describes the system as implemented in
`QTOS.Kit` (`kit/`) and the `Lumen` Qt Quick Controls style (`kit/style/`).

Exact values live in the tokens. See the generated
[token reference](tokens.md) and the source
[`kit/tokens/lumen.tokens.json`](../../kit/tokens/lumen.tokens.json).

## Principles

1. **Depth tells you where you are.** Chrome floats on glass above content.
   Focused windows sit higher than unfocused ones.
2. **Everything moves physically.** Motion is spring-based and interruptible.
   Things come from somewhere and go somewhere.
3. **Color is perceptual.** Every color is defined in OKLCH, so lightness steps
   look even and themes morph without muddy midpoints.
4. **Content first.** Materials recede. Text and media stay crisp and solid.
5. **Accessible by construction.** Reduce Motion, Reduce Transparency and
   Increase Contrast are global switches, not per-component work.

## Color

### Model

Each color role is `{ light: [L, C, H, A], dark: [L, C, H, A], followAccent }`:
OKLCH lightness (0–1), chroma, hue (degrees) and alpha. With `followAccent`,
the hue is an offset from the live accent hue.

`Theme` resolves roles with `Oklch.themed(token, darkness, hue)`, where:

- `darkness` is a spring between 0 (light) and 1 (dark), preset `gentle`.
- `hue` is a spring towards `accentHue`, preset `smooth`, always taking the
  short way around the hue circle.

Interpolation happens in **OKLab**. Out-of-gamut colors are mapped by reducing
chroma at constant lightness and hue (binary search), never by clipping
channels.

### Roles

| Group | Roles |
|-------|-------|
| Backgrounds | `background`, `surface`, `surfaceRaised`, `surfaceSunken` |
| Materials | `glassTint`, `glassStroke`, `frostTint`, `windowStroke` |
| Text | `textPrimary`, `textSecondary`, `textTertiary`, `textDisabled` |
| Controls | `fillControl`, `fillControlHover`, `fillControlPressed`, `fillSelected`, `separator`, `focusRing` |
| Accent | `accent`, `accentHover`, `accentText`, `onAccent` |
| Status | `positive`, `warning`, `critical` |
| Effects | `shadow`, `scrim` |

**Accent presets** (hue °): 274 Indigo (default), 248 Blue, 205 Teal, 162
Green, 95 Amber, 58 Orange, 25 Red, 338 Pink. Any hue 0–360 is valid.

Rules:

- Body text uses `textPrimary` on `surface`, `surfaceRaised` or glass.
  Secondary information uses `textSecondary`. Metadata uses `textTertiary`.
- `accent` fills primary actions and selection indicators. Text in the accent
  color uses `accentText` (lighter in dark appearance for contrast).
- Translucent fills (`fill*`) are composed over whatever is below, so they
  work on any surface and on glass.

## Materials

| Material | Component | Backdrop | Use |
|----------|-----------|----------|-----|
| **Glass** | `Glass` | `Materials.scene` (wallpaper + windows) | Shell chrome: Rail, Shelf, Beam, Control Center, banners, lock-screen fields |
| **Frost** | `Glass { tint: Theme.frostTint }` | `Materials.desktop` (wallpaper only) | In-window sidebars and inspectors (`FrostSidebar`) |
| **Surface** | `Surface` | none | Opaque panels, cards, controls |
| **Solid** | `Rectangle { color: Theme.surface }` | none | Content areas |

Glass recipe (`kit/shaders/glass.frag`):

1. Sample the shared backdrop: the scene at ¼ resolution, `MultiEffect` blur
   (`blurMax` 40 texture px ≈ 160 px on screen), saturation +0.45.
2. Mix with `glassTint` by its alpha.
3. Add an inner top sheen (`highlight`) and blue-noise grain (`grain` 0.022) against banding.
4. Draw a hairline (`glassStroke`) that is brighter at the top and fades to
   35% at the bottom: a specular edge.
5. Draw an analytic soft shadow outside the shape.

With **Reduce Transparency** or **Increase Contrast** (`Theme.solidMaterials`),
`Backdrop` pauses and `Glass` renders an opaque tinted `Surface` with
identical geometry.

## Shape

- Every rounded shape is a **continuous-curvature rectangle**: the corner is
  an Lᵖ-norm curve with exponent `squircleExponent` = 4. Circles use exponent
  2. App icons use 4.6.
- Radii (px): `xs` 4 · `s` 6 · `m` 9 · `l` 13 · `xl` 18 · `xxl` 26 · `window` 14.
  Nested shapes use `inner = outer − padding`.
- All shapes are signed distance fields (`Surface`, `Glass`, `RoundMask`):
  anti-aliased with `fwidth`, resolution-independent, one draw each.

## Elevation

| Level | Shadow blur | Offset Y | Opacity | Use |
|-------|-------------|----------|---------|-----|
| 0 | 0 | 0 | 0 | Flat |
| 1 | 8 | 1 | 0.45 | Controls, cards, primary buttons |
| 2 | 18 | 4 | 0.60 | Rail, app icons |
| 3 | 34 | 10 | 0.75 | Unfocused windows, tooltips, banners on the lock screen |
| 4 | 56 | 18 | 0.90 | Shelf, popovers, notification banners |
| 5 | 84 | 28 | 1.0 | Focused windows, Beam, Control Center |

Shadow color is `Theme.shadow` (tinted in light, near-black in dark).
Artwork may use a colored shadow in its own hue.

## Motion

`Spring` integrates a damped harmonic oscillator in closed form:

- `response` (s): the period of the undamped oscillation, which is the
  perceived duration. A critically damped spring is within 2% of its target
  after one response.
- `damping`: the damping ratio. 1 is critical (no overshoot), below 1 overshoots.

| Preset | Response | Damping | Use |
|--------|----------|---------|-----|
| `snappy` | 0.28 s | 0.90 | Controls, hover, press, small panels (Beam, Control Center) |
| `smooth` | 0.42 s | 1.00 | Windows, Overview, Space travel, sheets |
| `bouncy` | 0.50 s | 0.72 | Rare, playful moments: Shelf launch hop, checkbox pop |
| `gentle` | 0.80 s | 1.00 | Ambient changes: appearance, lock and unlock depth |

Rules:

- Use springs for anything that moves through space. Short `ColorAnimation`
  (110–160 ms) is fine for hover color.
- Retargeting keeps velocity. Gestures hand velocity to the spring
  (`spring.velocity = v`) on release.
- Lists enter with a stagger of `Motion.staggerMs` (18 ms) per item.
- Open from the origin: windows grow out of their Shelf icon. Closing shrinks
  in place. Minimise returns to the icon.
- **Reduce Motion** sets `Kit.reducedMotion`, and every spring jumps to its
  target. No component needs special handling.

```qml
Item {
    y: reveal.value
    Spring {
        id: reveal
        target: open ? 0 : 40
        preset: Motion.snappy
    }
}
```

## Typography

**Inter** (variable, `wght` and `opsz` axes) for UI, **JetBrains Mono**
(variable) for code. Both are bundled and registered at startup. Weights are
continuous axis values, not just named instances.

| Style | Size px | Weight | Tracking | Use |
|-------|---------|--------|----------|-----|
| `caption` | 11 | 520 | +0.15 | Metadata, section headers |
| `footnote` | 12 | 460 | +0.05 | Secondary lines, hints |
| `body` | 13 | 430 | 0 | Default text |
| `bodyStrong` | 13 | 600 | 0 | Emphasis, buttons, titles in rows |
| `callout` | 15 | 450 | −0.05 | Editor text, lock-screen field |
| `headline` | 15 | 630 | −0.1 | Card and inspector titles |
| `title3` | 17 | 630 | −0.15 | Small headings |
| `title2` | 22 | 660 | −0.3 | App section titles, Beam field |
| `title1` | 28 | 690 | −0.5 | Page titles |
| `display` | 36 | 720 | −0.8 | Hero headings |
| `hero` | 120 | 230 | −3.5 | Lock-screen clock (curve-rendered) |

Use `font: Theme.<style>`. Monospace: `Theme.mono`. Static text uses native
rendering for crispness. Text that scales during animation (the lock clock)
uses `Text.CurveRendering`.

## Iconography

- A curated Lucide set (113 symbols, `kit/icons/icons.txt`) at a 1.75 px
  stroke on a 24-unit grid, recoloured white and tinted on the GPU by `Icon`.
- Sizes: 13–16 px in controls, 16–18 px in the Rail and sidebars, 20–24 px in
  Control Center tiles.
- **App icons** (`AppIcon`): a continuous-curvature tile (radius 27% of size),
  a two-stop OKLCH gradient from one hue (lighter and +14° at the top, darker
  and −14° at the bottom), a specular rim, an inner sheen, and a white symbol
  at 50% of the tile.

## Wallpaper: Aurora

`Wallpaper` renders a procedural field: domain-warped fractal noise,
palette built and blended in OKLab from the accent hue, two aurora ribbons
with bloom, silk ridges and a perceptual vignette. Light and dark are
different palettes of the same field.

- Rendered once into a cached layer with 14% overscan. Zoom (lock,
  Overview) and parallax (Spaces) are transforms of that texture.
- **Living wallpaper** (off by default) animates the field slowly.
- **Audio-reactive light.** `energy` and `pulse` (from `Reactive` in the
  shell) drive a separate glow shader over the cached field: light rising
  from the bottom corners in the accent's neighbourhood, and a bloom that
  flares on beats. Additive in dark mode, a soft tint in light mode. It is
  drawn only while there is something to react to. See
  [AUDIO.md](../AUDIO.md).

## Components

| Component | Purpose |
|-----------|---------|
| `Surface` | Base SDF shape: fill, gradient, hairline, sheen, shadow |
| `Glass` | Translucent material over a `Backdrop` |
| `Backdrop` | Shared blurred texture of a subtree |
| `RoundMask` | Layer effect clipping to a continuous-curvature shape |
| `Wallpaper` | Aurora |
| `Icon`, `AppIcon`, `Artwork`, `QtosMark` | Symbols, app tiles, generated media art, brand mark |
| `IconButton`, `SidebarItem`, `SectionHeader`, `Card`, `SegmentedControl`, `SearchField`, `Kbd`, `Divider` | App building blocks |
| `Spectrum` | Audio visualiser: any number of 0..1 bands drawn as rounded bars, optionally mirrored, with a highlighted sub-range |
| `Sounds` | Interface sounds (`Sounds.play("notification")`); the shell binds `enabled`, `volume` and `device` |
| `Theme`, `Tokens`, `Motion`, `Materials`, `Oklch`, `Kit`, `Sounds` | Singletons |
| `Spring`, `SceneRect`, `FrameStats` | C++ behaviour types |

### Lumen Qt Quick Controls style

Selected with `QQuickStyle::setStyle("Lumen")`. It implements Button,
ToolButton, Switch, CheckBox, RadioButton, Slider, RangeSlider, TextField, TextArea,
ItemDelegate, ScrollBar, ScrollIndicator, ScrollView, ProgressBar, ToolTip,
Label, Pane, Frame, TabBar, TabButton, Popup, Menu, MenuItem and
MenuSeparator. Other controls fall back to Basic. `icon.name` takes Lumen
symbol names. `Button.highlighted` is the primary accent button.

### Qt Widgets

`platform/style` (the Lumen `QStyle`) and `platform/theme` (the `qtos` QPA
theme) apply the same tokens to Widgets apps: palette, Inter, rounded
controls, accent default buttons, focus rings and thin scroll bars.

## Accessibility

- Text roles meet contrast targets on their intended backgrounds in both
  appearances. Verify new roles with the token reference hex values.
- Text on accent fills (`onAccent` on `accent`/`accentHover`) meets WCAG AA
  (4.5:1) at **every** accent hue. OKLCH lightness alone cannot promise this,
  because luminance at a given lightness varies with hue, so these roles carry
  a `maxLuminance` cap. `Oklch.themed` (and the Widgets palette) lowers
  lightness until the cap holds.
- **Increase Contrast** blends every role that defines `highContrast` values
  toward them on a spring (`Theme.contrast`, 0–1). The high-contrast set
  targets 7:1 for primary, secondary and accent text, 4.5:1 for tertiary text
  and status colors, and 3:1 for focus rings. Separators, strokes and control
  fills strengthen, and glass becomes solid.
- `tests/tools/test_generate_tokens.py` enforces these ratios, so a token
  change that breaks them fails CI.
- Every interactive Kit component exposes `Accessible.role`/`name`, keyboard
  activation and a visible focus ring (`focusRing`, 2 px).
- Global switches: Reduce Motion (springs snap), Reduce Transparency (glass
  becomes opaque, backdrops pause), Increase Contrast. Planned: larger text.
