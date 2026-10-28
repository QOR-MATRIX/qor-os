# Where QOR OS stands

For Andrew, 28 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
— *the compositor and desktop in one* — and Lumen design, joined with
Demiurge's QQ engine, ARQADE and QOR Media, on a Linux base. Code:
`github.com/QOR-MATRIX/qor-os` (private), on this PC at `X:\Repoz\QOR-OS`.

**Phase 0 is complete.** Next is Phase 1: QQ, ARQADE and QOR Media on Linux,
inside Prism, driven by a controller.

## This session

1. **You merged demiurge-chain #109, and QOR OS's core moved to it**
   (`6a4822d`). The **core** — *the Rust code that holds keys* — is the same
   code as before; it now declares **Apache-2.0**, like the rest of
   demiurge-chain, and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) says
   so. Its tests pass on this PC.
2. **Nothing is waiting upstream.** Both Demiurge decisions you accepted on
   27 October, the core's audit exception (ADR-121) and the crates' licence
   (ADR-122), are merged.

## Needs you

1. **Which legal entity?** Four names appear as the copyright holder or Qt
   licensee: QOR-MATRIX, Demiurge, Laustrup Group, Astra Matrix, Inc. The Qt
   letter, trademark filings, the terms and code signing all need one real
   entity. See [the pack's first question](docs/release/README.md#one-question-first-which-legal-entity).
2. **Trademarks, before more public use.** The preliminary search — *not
   legal advice* — found close live marks: Tencent's **QQ** (game software),
   Arqiva's **ARQADE**, and Stack Exchange's "Arqade" gaming site; "QOR OS"
   sounds like Red Hat's **COREOS**. Take
   [TRADEMARKS.md](docs/release/TRADEMARKS.md) to an attorney soon.
3. **Qt:** Qt's published terms do not say whether a free OS image is an
   *application* (no fee) or a *device* (a fee per copy). The letter asking
   them is ready in [QT.md](docs/release/QT.md), with blanks only you can
   fill from your Qt account.
4. **Now, needing nobody else:** find which account the phrase pasted on
   5 October controls and replace it
   ([SIGN-IN.md](docs/release/SIGN-IN.md)), and move the Store keys' backup
   off this PC.

## Next

Phase 1: QQ, ARQADE and QOR Media built on Linux and running inside Prism,
driven by a DualSense.
