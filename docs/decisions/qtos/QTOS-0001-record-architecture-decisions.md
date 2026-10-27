# ADR-0001: Record architecture decisions

- **Status:** Accepted; superseded by [QOR-0001](../QOR-0001-one-rulebook-and-one-decision-log.md) on 2026-10-26
- **Date:** 2026-10-23

## Context

QTOS spans a compositor, a design system, services, apps, platform plugins
and an OS image. Decisions in one area constrain the others, and the reasons
behind them are easily lost.

## Decision

Record significant decisions as Architecture Decision Records in `docs/adr/`,
numbered sequentially, using [`TEMPLATE.md`](../TEMPLATE.md). An ADR is
required for changes to module boundaries, service contracts, the rendering
model, licensing, toolchain baselines and anything listed in
[ROADMAP.md](../../ROADMAP.md) as a decision gate.

## Consequences

Pull requests that change architecture include an ADR. Superseded ADRs stay
in place with their status updated.
