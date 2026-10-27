# QOR-0003: The QOR core, built from demiurge-chain at a pinned commit

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** QOR-0002; Demiurge
  [ADR-103](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-103-the-launcher-moves-to-qt-in-stages.md),
  [ADR-104](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-104-the-launcher-on-android-on-qt.md)

## Context

The QOR Launcher's Rust core (`tools/qor-launcher/core` in demiurge-chain)
holds what handles keys. That includes the vault (sealed with Argon2 and
ChaCha20-Poly1305 and kept in the operating system's keychain), the profiles,
and the confirmations they ask through. Amounts are CGT in integer embers.

ADR-104 gave the core a C bridge for Qt. It takes one zero-terminated JSON
request and returns one JSON answer, and no command answers with a key.
Demiurge's roadmap item L13.2 moves the rest of the launcher's Rust host into
the core, module by module with its tests: QOR ID's client, the chain client,
Social's client, the Library's index and the Mesh. The owner's plan adds
Qontrol.

Copying that code into QOR OS would give it two sources. Every fix to key
handling would then have to be made, reviewed and tested twice, under two
sets of rules.

## Decision

1. **QOR OS builds the core from demiurge-chain at a pinned commit and never
   copies it.**
   - [`core/CMakeLists.txt`](../../core/CMakeLists.txt) fetches the repository
     at `QOR_CORE_COMMIT` (a full hash, which git checks).
   - It builds the `qor-launcher-core` crate as a static library through
     Corrosion, with its `Cargo.lock` (`LOCKED`), as Demiurge's own Qt shell
     does.
   - The target `qor::core` carries the library and the bridge's header.
2. **QOR OS reaches the core only through its bridge.** It calls
   `qor_core_call` with JSON in and JSON out, as
   [`tests/core/tst_qorcore.cpp`](../../tests/core/tst_qorcore.cpp) does.
   Amounts cross as decimal strings of embers, so a `u128` is never rounded.
3. **The core changes upstream.** A change to it is made in demiurge-chain,
   under that repository's rules (its §9 log-hygiene checks included), and
   reaches QOR OS when the pin moves.
4. **The pin moves by a commit here.** That commit names the new upstream
   commit and what changed, and must pass CI.
   `QOR_CORE_SOURCE_DIR=<a demiurge-chain checkout>` builds against local
   changes while both sides are being worked on. A pin to anything but a
   commit on demiurge-chain's `main` never lands on `main` here.
5. **On the device the core runs as its own service, never inside Prism or
   an app.** The vault service (Phase 2) is a separate, sandboxed process built
   from the core. Apps and the shell reach it over a narrow interface, and the
   approvals it asks for are drawn by Prism in the trusted layer (VISION §3).

## Consequences

- Every machine that builds QOR OS needs Rust: 1.91 or later, the core's
  `rust-version`. CI uses 1.98.1 on Windows and Linux. Linux also needs
  `libdbus-1-dev`, because the core's keychain uses the Secret Service.
- `QOR_BUILD_CORE` is on wherever `cargo` is found. The CI presets require it,
  so a runner without Rust fails instead of skipping the core.
- The first pin is `28d9e97` (demiurge-chain `main`, 26 October 2026). That
  core has Sparks, errors, the vault, profiles and four bridge commands. The
  pin moves to L13.2's merge commit once that change is merged upstream, and
  [`docs/UPSTREAM.md`](../UPSTREAM.md) records each move.
- The bridge grows from four commands to the launcher's full set, with events,
  as Phase 2 makes the vault, sign-in and Social system services. Each command
  is added upstream with its tests.
- The decision is wrong if QOR OS keeps needing changes that upstream cannot
  take. If that happens, a later record moves the core's home.
