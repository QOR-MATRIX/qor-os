# QOR-0006: QOR OS 1.0 moves no money

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** Demiurge
  [ADR-002](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-002-value-from-spending.md),
  [ADR-008](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-008-language-discipline.md),
  [ADR-035](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-035-eighteen-decimals.md),
  [ADR-101](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-101-the-spark-is-a-thousandth-of-a-cgt.md);
  [STORE.md](../STORE.md)

## Context

QOR OS inherits two economies that conflict.

- **QTOS's Store plan is in dollars** ([STORE.md](../STORE.md#roadmap)). Stage
  2 adds a one-time $13 developer account. Stage 3 adds paid items, an 8%
  commission and a prepaid balance. Stage 1, which exists, has free items only.
- **Demiurge's economy is CGT**, in integer embers (1 CGT = 10^18 embers, a
  Spark is 10^15). CGT is ARQADE's and the Market's only currency, and the
  chain takes no platform share. Demiurge's open question U-15 records a 15%
  proposal for a marketplace that settles in fiat. Its rules forbid a dollar
  balance that turns into CGT.

Neither economy can carry value yet:

- The chain is an unaudited devnet with test value only.
- Every economic number is listed as open.
- Demiurge's owner's list still waits on Terms of Service, a privacy policy, a
  legal opinion, code-signing certificates and an external security review.

## Decision

1. **QOR OS 1.0 ships free items only** in the Store, and **test CGT only**,
   on the devnet, in ARQADE and the Market.
2. **No code that moves money ships.** That means no dollar price, balance,
   commission or payout, and nothing that turns dollars into CGT or CGT into
   dollars. Design and private prototypes may exist; they do not reach a
   build anyone installs.
3. **Paid items wait for written decisions:**
   - U-15, decided in demiurge-chain;
   - a QOR record choosing QOR OS's model: dollars, CGT, or both kept apart,
     and the platform's share;
   - the legal list above;
   - The Qt Company's confirmation for the SDK (QOR-0008).

   QTOS's Store stage 2 (the developer program, without payments) may proceed.
4. **Demiurge's money rules apply across QOR OS:**
   - amounts are integer embers in `u128`, never floating point;
   - excess precision is refused, never truncated;
   - scaled arithmetic uses the SDK's helpers;
   - no value Demiurge lists as undecided is invented, in code or in a
     document;
   - no investment language appears anywhere (ADR-008).

## Consequences

- QOR OS can show balances and prices in test CGT, read through the QOR core,
  with the devnet named wherever they appear.
- The Store's catalogue keeps `"price": 0` for every item, and the Store app
  shows no purchase flow (it offers Install only for free items).
- The catalogue's `price` is a floating-point number of dollars today
  (`store/src/catalog.h`). It only ever holds 0. Before any paid item, it is
  replaced by an integer amount in the chosen currency's smallest unit, as the
  money rules above require.
- When a model is chosen, its record supersedes this one, and Store stage 3 is
  rewritten to match.
