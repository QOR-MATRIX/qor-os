# QOR-0013: Entire may reach every QOR-MATRIX repository, QOR OS included

- **Status:** Accepted
- **Date:** 2026-10-29
- **Deciders:** the owner
- **Supersedes:** [QOR-0012](QOR-0012-github-stays-the-source-of-truth-entire-mirrors-it.md)
  decision 2 (QOR OS mirrored only once Entire documents private repositories)
- **Relates to:** Demiurge ADR-124 (Entire mirrors demiurge-chain); `AGENTS.md` §6

## Context

QOR-0012 kept GitHub as the source of truth and Entire as a mirror. Its
decision 2 kept QOR OS off Entire until Entire documents private
repositories. But Entire's GitHub app had been installed on the QOR-MATRIX
organisation on 28 October 2026 for all its repositories, this one included,
with read and write access to their contents, pull requests and workflows.
Told so, and how to limit the app to demiurge-chain, the owner answered:
"Allow Entire in full."

## Decision

1. **Entire's GitHub app keeps its access to every QOR-MATRIX repository**,
   QOR OS included, as it was installed on 28 October 2026. This supersedes
   QOR-0012 decision 2.
2. **Entire mirrors QOR OS**, as it mirrors demiurge-chain (ADR-124).
   QOR-0012's other decisions stand: GitHub stays the source of truth for the
   code, CI on the licensed kit and the history; nothing here depends on the
   mirror; a move to Entire is a later record.
3. **Its "Entire Gates" check is advice.** QOR OS's `main` requires no check.

## Not decided here

- **Recording agents' sessions with Entire's command-line tool.** It would
  write session transcripts beside the commits, and a transcript carries what
  `AGENTS.md` §6 keeps out of logs and commits. That needs a record of its own.

## Consequences

- Entire can read everything in this repository, `docs/release/` included,
  under Entire's own terms, which on 29 October 2026 said nothing about
  private repositories.
- Narrowing the app later is the owner's act, in the organisation's GitHub
  settings, and a later record.
