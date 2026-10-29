# QOR-0012: GitHub stays the source of truth; Entire mirrors QOR OS once it hosts private repositories

- **Status:** Superseded by [QOR-0014](QOR-0014-qor-os-does-not-use-entire.md)
  (decision 2 first by [QOR-0013](QOR-0013-entire-may-reach-every-repository.md)), all on 2026-10-29
- **Date:** 2026-10-29
- **Deciders:** the owner
- **Relates to:** Demiurge ADR-124 (the same choice for demiurge-chain);
  [docs/CI.md](../CI.md) (the self-hosted runners);
  [QOR-0008](QOR-0008-before-a-public-release.md) (nothing public before the release gates)

## Context

The owner suggested moving QOR OS and Demiurge to **Entire**, the Git forge
that GitHub's former chief executive launched on 26 October 2026, "if at all
possible". Entire hosts code, reviews changes, runs CI through partners
(Buildkite, which can run on one's own machines, and Depot, which runs existing
workflows in its cloud) and keeps a memory of the agents that worked on the
code. It can mirror a GitHub repository, with GitHub staying the source of
truth, or take one over with its `entire-migrate` plug-in. What it said on
29 October 2026 did not cover private repositories, organisation permissions
or pricing.

QOR OS relies on GitHub for more than its code:

- CI runs GitHub Actions on the workstation's self-hosted runners, because the
  commercial Qt kit is licensed to the owner and installed there
  ([CI.md](../CI.md));
- the QOR core is fetched from demiurge-chain on GitHub at a pinned commit
  ([QOR-0003](QOR-0003-the-qor-core-at-a-pinned-commit.md));
- the carried products move by `git subtree` from demiurge-chain
  ([UPSTREAM.md](../UPSTREAM.md));
- the repository is private, and `docs/release/` holds research that stays out
  of public view.

Asked to choose between mirroring with GitHub as the source of truth, a full
migration, or waiting, the owner chose to mirror.

Entire's GitHub app (`entire`) was already installed on the QOR-MATRIX
organisation, on 28 October 2026, for **all** its repositories, this one
included, with read and write access to their contents, pull requests and
workflows. Its "Entire Gates" check appeared on demiurge-chain's #110 on
29 October.

## Decision

1. **GitHub stays the source of truth** for QOR OS's code, CI, its runners and
   its history.
2. **Entire mirrors `QOR-MATRIX/qor-os` only once Entire documents private
   repositories** that only the organisation's members can read. Until then
   nothing of QOR OS goes to Entire.
3. **Nothing here depends on the mirror**: no build, script or document reads
   from Entire.
4. **A move to Entire is a later record**, made when Entire documents private
   repositories, permissions, pricing and a way to run CI on the workstation's
   licensed kit.

## Not decided here

- **Recording agents' sessions with Entire's command-line tool.** Sessions
  carry what `AGENTS.md` §6 keeps out of logs and commits, so recording them
  needs a record of its own.

## Consequences

- Nothing changes for CI or for anyone working here.
- **Decision 2 is not met today**: the app installed on 28 October can read
  this repository. Meeting it is the owner's act, in the organisation's GitHub
  settings: Entire's app limited to the repositories it may mirror
  (demiurge-chain, ADR-124), until Entire documents private repositories.
