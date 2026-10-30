# QOR OS documentation

The current documents. A document not listed here (or a product's own
README) is not current. [`../AGENTS.md`](../AGENTS.md) says how they are
kept true.

| Document | Read it to… |
|----------|-------------|
| [../OWNER.md](../OWNER.md) | see where QOR OS stands, on one page |
| [../VISION.md](../VISION.md) | understand what QOR OS is and where it is going |
| [ROADMAP.md](ROADMAP.md) | see what is done, what is next, and the release gates |
| [release/](release/README.md) | act on each release gate: who must act, the next step, and the drafts for the owner's advisers (Qt, trademarks, terms and privacy, signing, the security review, sign-in) |
| [decisions/](decisions/README.md) | read why decisions were made (QOR, QTOS and Demiurge records); propose new ones |
| [UPSTREAM.md](UPSTREAM.md) | know what QOR OS shares with demiurge-chain (ARQADE's rules and runs, the one look, the QOR core), how it moves, and where the products came from |
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

The products document themselves in their own READMEs:
[QQ](../products/qq/README.md), [ARQADE](../products/arqade/qt/README.md) and
[QOR Media](../tools/qor-media/README.md). They came from demiurge-chain
([QOR-0015](decisions/QOR-0015-the-products-are-developed-here.md)), so their
links to `docs/decisions/ADR-*`, `docs/blueprints/`, `DIRECTION` and
`HANDOFF.md` point at demiurge-chain's documents, which stay there. Where a
README and this repository disagree on what is planned, [ROADMAP.md](ROADMAP.md)
wins.

`design/tokens.json` is Demiurge's one look, which the products' tests read. Lumen's own tokens are `kit/tokens/lumen.tokens.json`, documented in
`design/tokens.md`.

Screenshots in [`images/`](images/) are produced by
`tools/screenshots/capture.ps1`. Regenerate them when the UI changes.
