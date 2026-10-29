# QOR-0014: QOR OS does not use Entire

- **Status:** Accepted
- **Date:** 2026-10-29
- **Deciders:** the owner, on the assistant's recommendation
- **Supersedes:** [QOR-0012](QOR-0012-github-stays-the-source-of-truth-entire-mirrors-it.md)
  and [QOR-0013](QOR-0013-entire-may-reach-every-repository.md)
- **Relates to:** Demiurge ADR-125 (the same for demiurge-chain); `AGENTS.md` §6

## Context

The owner had asked about moving the projects to Entire, the Git forge from
GitHub's former chief executive, and then allowed its GitHub app on every
repository (QOR-0013). The same day the owner asked for the assistant's best
recommendation: "If you are not seeing any real positive benefit to using
Entire, then let's forget about it and keep on trekking the way we already
were."

What Entire offered QOR OS on 29 October 2026, against what it cost:

- **Hosting and review** duplicate GitHub, which stays the source of truth
  (QOR-0012 decision 1) and holds the history, the pull requests and the
  workflows.
- **CI** runs through partners, Buildkite on one's own machines or Depot in
  its cloud. QOR OS's CI needs the owner's licensed Qt kit on the workstation,
  which GitHub's self-hosted runners already reach ([CI.md](../CI.md)).
- **Agents' sessions recorded beside the commits** would write transcripts
  into the repositories, and a transcript carries what `AGENTS.md` §6 keeps
  out of commits. The decision records, `OWNER.md` and the commit messages
  already say what was done and why.
- **The app** had read and write access to every repository, this private one
  and its `docs/release/` included, under terms that said nothing about
  private repositories. Its "Entire Gates" check needed approvals on Entire,
  so it marked every pull request neutral or failed.

## Decision

1. **QOR OS does not use Entire.** GitHub alone holds QOR OS, its CI and its
   history, as before 28 October 2026. This supersedes QOR-0012 and QOR-0013.
2. **Entire's GitHub app is removed from the QOR-MATRIX organisation.** GitHub
   lets an organisation owner do that only from the organisation's settings,
   not through the API.
3. **Entire is looked at again only in a later record**, if it documents
   private repositories and a way to run CI on the licensed kit that GitHub
   lacks.

## Consequences

- Nothing in QOR OS read from or depended on Entire, so nothing else changes.
- Until the app is removed it can still read this repository.
