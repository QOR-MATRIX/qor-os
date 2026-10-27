# Where QOR OS stands

For Andrew, 27 October 2026 (late evening). One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
— *the compositor and desktop in one* — and Lumen design, joined with
Demiurge's QQ engine, ARQADE and QOR Media, on a Linux base. Code:
`github.com/QOR-MATRIX/qor-os` (private), on this PC at `X:\Repoz\QOR-OS`.

**Phase 0 is complete.** Next is Phase 1: QQ, ARQADE and QOR Media on Linux,
inside Prism, driven by a controller.

## This session

1. **You merged demiurge-chain #108**, and approved my decisions. So **ADR-122 is
   accepted**: the six active Rust crates, the **core** — *the Rust code that holds
   keys* — among them, now say **Apache-2.0**, like the repository. That change
   is in [demiurge-chain #109](https://github.com/QOR-MATRIX/demiurge-chain/pull/109),
   which waits for your merge.
2. **Demiurge's gates page now counts all its decisions.** It read a bold
   "Accepted" as not accepted; the records lost the bold, its rules are
   unchanged, and a test keeps it so. Also in #109.
3. **Nothing changed in QOR OS's code.** Its core stays at #107's merge until
   #109 merges; then the pin moves and the notices say Apache-2.0.
4. **Earlier today:** the core built at #107's merge, the name QOR OS through
   the code ([QOR-0009](docs/decisions/QOR-0009-the-name-is-qor-os.md)), the
   Store's signing keys moved to `C:\Users\Godmode\.qor-os\store-signing`
   with a backup beside them, and the release-gate pack in
   [`docs/release/`](docs/release/README.md).

## Needs you

1. **Merge demiurge-chain #109.** Then tell me, and I'll move QOR OS's core to it.
2. **Which legal entity?** Four names appear as the copyright holder or Qt
   licensee: QOR-MATRIX, Demiurge, Laustrup Group, Astra Matrix, Inc. The Qt
   letter, trademark filings, the terms and code signing all need one real
   entity. See [the pack's first question](docs/release/README.md#one-question-first-which-legal-entity).
3. **Trademarks, before more public use.** The preliminary search — *not
   legal advice* — found close live marks: Tencent's **QQ** (game software),
   Arqiva's **ARQADE**, and Stack Exchange's "Arqade" gaming site; "QOR OS"
   sounds like Red Hat's **COREOS**. Take
   [TRADEMARKS.md](docs/release/TRADEMARKS.md) to an attorney soon.
4. **Qt:** Qt's published terms do not say whether a free OS image is an
   *application* (no fee) or a *device* (a fee per copy). The letter asking
   them is ready in [QT.md](docs/release/QT.md), with blanks only you can
   fill from your Qt account.
5. **Now, needing nobody else:** find which account the phrase pasted on
   5 October controls and replace it
   ([SIGN-IN.md](docs/release/SIGN-IN.md)), and move the Store keys' backup
   off this PC.

## Next

Phase 1: QQ, ARQADE and QOR Media built on Linux and running inside Prism,
driven by a DualSense.
