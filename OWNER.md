# Where QOR OS stands

For Andrew, 27 October 2026 (evening). One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
— *the compositor and desktop in one* — and Lumen design, joined with
Demiurge's QQ engine, ARQADE and QOR Media, on a Linux base. Code:
`github.com/QOR-MATRIX/qor-os` (private), on this PC at `X:\Repoz\QOR-OS`.

**Phase 0 is complete.** Next is Phase 1: QQ, ARQADE and QOR Media on Linux,
inside Prism, driven by a controller.

## This session

1. **The core** — *the Rust code that holds keys* — is built at your merge of
   #107, with all its modules. One name, **QOR OS**, runs through the code
   ([QOR-0009](docs/decisions/QOR-0009-the-name-is-qor-os.md)). CI passed.
2. **The Store's signing keys moved** to `C:\Users\Godmode\.qor-os\store-signing`,
   checked byte for byte, with a backup beside them
   (`store-signing-backup-2026-10-27`). The Store tool uses the new place.
3. **demiurge-chain #108**, records only, waits for your merge:
   - **ADR-121 accepted** at your "proceed with", logged as a retroactive
     approval because the merge came first.
   - **ADR-122 proposed:** should the six active Rust crates say Apache-2.0,
     like the repository, instead of MIT? I recommend yes, but it is yours:
     my own list told you it was yours to decide, and changing a licence is
     the copyright holder's act.
   - A fault found in Demiurge's gates page, reported, not fixed.
4. **The release gates have a pack:** [`docs/release/`](docs/release/README.md)
   holds, for each gate, the facts, drafts, questions and next step, checked
   by two reviewers each. It found things you should know (below).

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
4. **Merge #108**, and say "accept ADR-122" (or not).
5. **Now, needing nobody else:** find which account the phrase pasted on
   5 October controls and replace it
   ([SIGN-IN.md](docs/release/SIGN-IN.md)), and move the Store keys' backup
   off this PC.

## Next

Phase 1: QQ, ARQADE and QOR Media built on Linux and running inside Prism,
driven by a DualSense.
