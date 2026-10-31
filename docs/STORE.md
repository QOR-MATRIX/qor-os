# The QOR OS Store

The Store is where people get themes, widgets and, later, apps, games, tools
and music for QOR OS. Stage 1, in this repository today, is a complete,
verifying client with QOR OS's own themes and widgets. There are no payments
yet. Third-party publishing arrives with the developer program in stages 2
and 3 ([Roadmap](#roadmap)).

## What people see

- **Store app** (`apps/StoreApp.qml`): Discover, Themes, Widgets, Apps, Games,
  Tools and Music, search, item pages, and a Library with updates. Apps lists
  what comes with QOR OS. Games, Tools and Music say plainly that they arrive
  with the developer program.
- **Themes** apply from the Store, from Settings › Appearance (Aurora plus
  installed themes) or from Beam ("Theme: Ember"). A theme sets the accent, a
  preferred appearance, a tint for every neutral surface and the Aurora
  wallpaper's character. The accent and appearance stay the user's to change
  afterwards.
- **Widgets** live on the desktop, over the wallpaper and under the windows,
  on a grid of 152 px cells: small (1×1), medium (2×1) and large (2×2). Add
  them from the Store ("Add to Desktop") or from the gallery: right-click the
  desktop and choose Edit Widgets, or use Beam. While editing, the windows
  step aside, widgets drag to a free cell, and × removes one. Escape or Done
  finishes. The layout persists in `Settings.widgets`.

First-party content: eight themes (Ember, Tidepool, Moss, Dusk, Paper,
Graphite, Saffron, Glacier) and six widgets (Clock, Calendar, Now Playing,
System, Focus, Visualizer), all by QOR-MATRIX.

## How it works

`QOR.Store` (`store/`) is the client:

| Part | Role |
|------|------|
| `StoreEngine` | Fetches the catalogue; downloads, verifies, installs, updates and removes packages per user |
| `Store` (QML singleton) | The catalogue and the user's library as plain maps for QML; installed themes and widgets |
| `Themes` (QML singleton) | Aurora plus installed themes; the active one; `apply(id)` |
| `ThemePreview` | A theme in miniature: its wallpaper and a window in its colours |

The catalogue comes from `QOR_STORE_URL` (the URL of a `catalog.json`; its
signature is the same URL with `.sig` appended) or, by default, the signed
first-party catalogue compiled into QOR OS. `qrc:` and `file:` URLs are read
directly; anything else goes through Qt Network (redirects only to equally
safe URLs, a 30 s transfer timeout), so a store server needs no client
change. Stage 1 has no server, so that path has not yet met one. Packages install under
`QOR_STORE_DIR`, by default the app data folder's `store/`
(`items/<id>/` and `installed.json`). Automated runs (`--script`,
`--screenshot`, `--benchmark`) and the tests use a temporary folder.

The shell binds the active theme into `Theme` (`neutralHue`, `neutralTint`,
`wallpaperSeed`, `wallpaperIntensity`). Neutral roles resolve through
`Oklch.themed(token, …, neutralHue, neutralChroma)`. With Aurora's values the
colours are identical to Lumen's own, which the golden-image tests guard.

## Trust

Nothing installs unless all of these hold:

1. The catalogue carries the **store's signature** over its exact bytes. The
   store's public key is compiled into QOR OS (`store/keys/store.pub`) and is the
   root of trust. Nothing in an unsigned or altered catalogue is believed, not
   even its developer keys.
2. The downloaded package's **SHA-256** matches the catalogue entry.
3. The package's manifest carries its **developer's signature**, made with
   the key the signed catalogue lists for that developer.
4. Every file matches the **manifest's SHA-256**, and the package holds
   exactly the listed files: no extra or missing files, no symbolic links, no
   paths that climb out of the install folder, and at most 32 MB.
5. The manifest's id, version, kind and developer match the catalogue's.

Signatures are ECDSA P-256 over SHA-256 (r ‖ s, 64 bytes), checked by the
platform's own crypto: CNG on Windows, OpenSSL's libcrypto elsewhere. Files
are written to a staging folder that replaces the installed version only
once every file is in place, so a failed install leaves nothing behind and an
update never leaves a half-written widget.

**Widgets are code.** A widget is QML that runs in the shell, so stage 1
installs only QOR-MATRIX widgets. Third-party widgets and apps wait for the
Phase 3 sandbox, which runs them out of process. Themes are data and can open
to third parties sooner.

## Formats

**Catalogue** (`catalog.json`, signed as `catalog.json.sig`):

```json
{
  "format": 1,
  "store": "QOR OS Store",
  "developers": { "dev.qorsync": { "name": "QOR-MATRIX", "verified": true, "key": "<base64 P-256 point>" } },
  "items": [{
    "id": "dev.qorsync.theme.ember", "kind": "theme", "name": "Ember", "developer": "dev.qorsync",
    "version": "1.0.0", "summary": "…", "description": "…", "whatsNew": "…", "released": "2026-10-26",
    "tags": ["warm"], "featured": true, "price": 0,
    "preview": { "accentHue": 38, "appearance": "dark", "neutralHue": 40, "neutralTint": 1.5,
                 "wallpaperSeed": 11.7, "wallpaperIntensity": 1.15 },
    "package": { "file": "packages/dev.qorsync.theme.ember-1.0.0.qorpkg", "size": 806, "sha256": "…" }
  }]
}
```

**Package** (`.qorpkg`): a zip archive of `manifest.json` (`format`, `id`,
`kind`, `version`, `name`, `developer`, and `files`: path → SHA-256),
`manifest.sig` (the developer's signature over `manifest.json`) and the
files.

**Theme** (`theme.json`):

```json
{ "accentHue": 38, "appearance": "dark",
  "neutral": { "hue": 40, "tint": 1.5 },
  "wallpaper": { "style": "aurora", "seed": 11.7, "intensity": 1.15 } }
```

`appearance` is `dark`, `light` or `any`. `tint` scales the neutrals' chroma
(0 to 2.5; 1 is Lumen's). Seeds run 0 to 1000 and intensity 0.3 to 1.6.

**Widget** (`widget.json` and the QML it names):

```json
{ "entry": "Clock.qml", "size": "medium", "glyph": "clock", "hue": 228 }
```

The entry's root `Item` fills a glass card the desktop draws. It may import
`QtQuick`, `QtQuick.Shapes`, `QOR.Kit` and `QOR.Services`, reads the system
through `Services` and asks for actions through `Intents`. It never needs
input while the desktop is being edited: the card disables it then.

## Building the catalogue

The sources are in `store/content/`: `developers.json`, then one folder per
item under `themes/` or `widgets/`, each with a `listing.json` (the
catalogue text) and the files its package carries.

```bash
python tools/store/build_store.py build      # sign changed packages and the catalogue
python tools/store/build_store.py check      # verify store/catalog against store/content (CI runs this)
python tools/store/build_store.py fixtures   # regenerate tests/store/fixtures (throwaway keys)
```

Unchanged items keep their signed packages, so rebuilding changes nothing.
`git` treats packages and signatures as binary and leaves the catalogue's
bytes alone (`.gitattributes`).

**Signing keys never enter the repository.** `build` keeps them in
`~/.qor-os/store-signing/` (moved there from QTOS's `~/.qtos/store-signing/`
on 27 October 2026). If a machine still has only the old folder, `build`
uses it, so a missing folder never creates a new store key:
`store.pem` signs the catalogue and
`developer-<id>.pem` signs that developer's packages. The first build creates
them and writes the store's public key to `store/keys/store.pub`. Back these
keys up: whoever holds `store.pem` decides what every QOR OS installation
trusts. Before the Store opens to the public, generate the production store
key offline or in a hardware security module, sign catalogues on a machine
that holds nothing else, and ship its public key in a release. Rotating the
store key needs a QOR OS update.

Tests: `tst_store` (signatures, the first-party catalogue, refused tampering,
forgery and unsigned catalogues, install, update and remove), a qmllint pass
over every widget (`store_widgets_qmllint`) and the Store section of
`shell_e2e`.

## What runs, and when it is checked

A package is verified as it is installed (the catalogue's digest, the developer's signature, every file). Its manifest
and signature are kept beside its files, and `Store` verifies them again each time it loads what is installed: every
listed file against the manifest and nothing added, since anything running as the person can write there. What fails is
not loaded. A widget's QML runs inside the shell with all it can do, so only QOR OS's own (`dev.qorsync`) are loaded,
whoever a catalogue lists. A version older than the one installed is refused ([QOR-0018](decisions/QOR-0018-the-security-and-quality-bar.md)).

## Roadmap

**QOR OS 1.0 moves no money**
([QOR-0006](decisions/QOR-0006-qor-os-1-0-moves-no-money.md)). The Store
ships free items only.

- Stage 2 may proceed without its fee, which is a payment.
- Stage 3 below is QTOS's proposal, kept as the starting point. It is not
  decided. It waits for:
  - Demiurge's U-15 decided in writing;
  - a QOR record choosing the model: dollars, CGT, or both kept apart, and
    the platform's share;
  - the legal list and The Qt Company's confirmation.

  Until then no price, balance, commission or payout code reaches a build.

**Stage 2: the developer program.** A developer portal and an SDK preview
(Phase 4's installable SDK). Developers register once (the recommendation is
a one-time $13 per developer account rather than per item, with free items
always free to publish, waived for a founding group), sign their packages
with their own keys, and see privacy-preserving analytics: aggregated,
opt-in, with no tracking of individual users. Every submission is reviewed,
automatically and by people, before the store signs a catalogue that lists
it. Third-party themes first; widgets and apps once the Phase 3 sandbox
exists.

**Stage 3: paid items.** An 8% commission on paid purchases, made to work by
a prepaid QOR OS balance: people top up once (cards and Cash App Pay through
Stripe; Bitcoin only for top-ups and fees, through a processor that settles
in dollars) and purchases come out of the balance, so card fees do not eat
small sales. Stripe Connect onboards and pays developers. Stripe Tax collects
sales tax and VAT. Refunds follow local law (the EU's 14-day right on digital
content included).

**Before either stage.** Confirm with The Qt Company how Qt's commercial
licence applies to shipping QOR OS on devices and to an SDK that third parties
build on ([QTOS-0004](decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md),
[QOR-0008](decisions/QOR-0008-before-a-public-release.md)). Write the developer agreement, terms, privacy policy and
content policy, including age ratings for games. Keep installs from outside
the Store (Flatpak, sideloading) open: that is what lets people trust the
Store.
