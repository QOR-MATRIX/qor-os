# QOR OS documentation

The current documents. A document not listed here (or a carried product's
own README) is not current. [`../AGENTS.md`](../AGENTS.md) says how they are
kept true.

| Document | Read it to… |
|----------|-------------|
| [../OWNER.md](../OWNER.md) | see where QOR OS stands, on one page |
| [../VISION.md](../VISION.md) | understand what QOR OS is and where it is going |
| [ROADMAP.md](ROADMAP.md) | see what is done, what is next, and the release gates |
| [decisions/](decisions/README.md) | read why decisions were made (QOR, QTOS and Demiurge records); propose new ones |
| [UPSTREAM.md](UPSTREAM.md) | know what is carried from demiurge-chain, how to pull it, and how the QOR core's pin moves |
| [ARCHITECTURE.md](ARCHITECTURE.md) | understand how the code is organised and how the pieces talk |
| [BUILDING.md](BUILDING.md) | set up the toolchain, build, run, deploy and troubleshoot |
| [DEVELOPMENT.md](DEVELOPMENT.md) | follow conventions, use recipes (add a service, app or control), debug |
| [CI.md](CI.md) | configure CI with the commercial Qt kit and the shared runners |
| [design/LUMEN.md](design/LUMEN.md) | apply the Lumen design language correctly |
| [design/tokens.md](design/tokens.md) | look up exact Lumen token values (generated) |
| [design/SHELL.md](design/SHELL.md) | know exactly how each shell surface behaves; keyboard reference |
| [SERVICES.md](SERVICES.md) | use or change service contracts; scenarios; `qorctl` |
| [AUDIO.md](AUDIO.md) | use the audio engine and the audio-reactive interface |
| [CONTROLLERS.md](CONTROLLERS.md) | use game controllers and the controller interface |
| [STORE.md](STORE.md) | publish to and verify the signed Store |
| [QT-MODULES.md](QT-MODULES.md) | see every Qt module and tool and its role in QOR OS |
| [PERFORMANCE.md](PERFORMANCE.md) | check budgets, measure frame pacing, keep the shell fast |

The carried products document themselves in their own READMEs:
[QQ](../products/qq/README.md), [ARQADE](../products/arqade/qt/README.md) and
[QOR Media](../tools/qor-media/README.md). Their links to `docs/decisions/ADR-*`,
`docs/blueprints/` and `HANDOFF.md` point at demiurge-chain's documents, which
live upstream ([UPSTREAM.md](UPSTREAM.md)).

`design/tokens.json` is Demiurge's one look, which the carried products' tests
read. Lumen's own tokens are `kit/tokens/lumen.tokens.json`, documented in
`design/tokens.md`.

Screenshots in [`images/`](images/) are produced by
`tools/screenshots/capture.ps1`. Regenerate them when the UI changes.
