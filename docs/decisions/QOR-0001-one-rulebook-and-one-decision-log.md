# QOR-0001: One rulebook and one decision log

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** supersedes [QTOS-0001](qtos/QTOS-0001-record-architecture-decisions.md);
  cites Demiurge's [decision records](https://github.com/QOR-MATRIX/demiurge-chain/tree/main/docs/decisions)

## Context

QOR OS joins two projects, and each has its own decisions and rules:

- **QTOS** recorded ten decisions, ADR-0001 to ADR-0010, in `docs/adr/`. Its
  working practice (CMake presets, warnings as errors, zero-warning
  `qmllint`, golden images, self-hosted CI with the commercial Qt kit) lived
  in `CONTRIBUTING.md` and `docs/DEVELOPMENT.md`.
- **Demiurge** (`QOR-MATRIX/demiurge-chain`, public) records ADR-001 onward in
  `docs/decisions/`, indexed in `docs/DECISIONS.md`. Its `AGENTS.md` binds
  every assistant: the code is the truth, money is integer embers, no
  investment language, nothing secret reaches a log.

Two numbering schemes that both say "ADR" would make every citation
ambiguous, and two rulebooks would drift apart.

## Decision

1. **Three series, each named by its prefix.**
   - `QOR-NNNN`: QOR OS's own records, in `docs/decisions/`, from QOR-0001.
   - `QTOS-NNNN`: QTOS's records, moved unchanged to `docs/decisions/qtos/`
     (QTOS-0001 to QTOS-0010). They stay in force unless a QOR record
     supersedes or amends them, and then both say so.
   - `ADR-NNN`: Demiurge's records. They stay in demiurge-chain and are cited
     by number with a link, never copied or restated here. When QOR OS departs
     from one on the device, a QOR record says so.
2. **One log.** [`docs/decisions/README.md`](README.md) is the only index of
   decisions for QOR OS. It lists every QOR and QTOS record and the Demiurge
   records QOR OS depends on.
3. **A record is never edited to change its decision.** A later record
   supersedes it, and both say so. Fixing a link or adding a "superseded by"
   line is not a change of decision.
4. **One rulebook.** [`AGENTS.md`](../../AGENTS.md) holds the rules for every
   contributor, human or assistant, and `CLAUDE.md` points to it. It merges
   Demiurge's rules on checking claims against the code, money, language,
   secrets and scope with QTOS's build and CI practice.
   [`CONTRIBUTING.md`](../../CONTRIBUTING.md) is the human workflow.
5. **What needs a record.** A QOR record is required for module boundaries,
   service contracts, the rendering model, licensing, toolchain baselines,
   the base image, anything that moves money or keys, a change to what is
   carried from demiurge-chain, and anything [`docs/ROADMAP.md`](../ROADMAP.md)
   calls a decision.

## Consequences

- Citations are unambiguous: "QTOS-0003", "QOR-0005", "Demiurge ADR-104".
- QTOS's records keep their history: `git log --follow` shows them back to
  QTOS's first commit.
- The template is [`TEMPLATE.md`](TEMPLATE.md).
- Rules that exist only upstream, such as the chain's toolchain pin, are not
  copied. QOR OS follows them by working on that code upstream, in
  demiurge-chain (QOR-0002, QOR-0003).
