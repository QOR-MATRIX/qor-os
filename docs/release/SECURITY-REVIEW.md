# External security review: RFP and scope

*Prepared on 27 October 2026 by an assistant, from the research listed under
[Sources](#sources), for the owner and their advisers. It is research, not
legal advice.*

**The gate** ([ROADMAP.md](../ROADMAP.md#release-gates)): "An external
security review of the QOR core and the vault service".

**Demiurge's matching criteria** (`docs/GATES.toml` on demiurge-chain's
origin/main, both unmet):

- `beta.external-review`: "An external security review of the runtime, the
  launcher vault and QOR ID is complete, and every finding it rates high or
  critical is closed."
- `public-release.review-closed`: "Every finding of the external security
  reviews, at any severity, is closed or accepted in writing by the owner."

Demiurge's `SECURITY.md` opens: "**Demiurge has not been audited.** It is
pre-release software. Do not hold real value on it." Its roadmap item M7.3,
"An external security review of the runtime, the launcher vault and QOR ID",
is unchecked. No reviewer, RFP or scope exists.

## First decision: one review or two

QOR OS's vault service does not exist yet (Phase 2). Everything else in scope
exists.

- **Two rounds.** Round 1 now: the QOR core, the launcher vault and host, the
  chain runtime and pallets, and QOR ID. This serves Demiurge's Beta gate and
  most of QOR OS's. Round 2 after Phase 2: QOR OS's vault service, the core
  at its new pin, and Prism's trusted layer for approvals.
- **One round** after Phase 2, covering everything. Fewer engagements; the
  Beta gate waits for it.

The owner chooses. The RFP below is written for round 1, with round 2 as an
option firms should price separately.

---

## The RFP

### 1. Who we are

[Legal entity] ([country]) builds QOR OS, an operating system for players and
game makers (private source), and Demiurge, an open network: a Substrate
chain, an identity service (QOR ID) and launchers that hold people's keys
(public source, Apache-2.0, at
[github.com/QOR-MATRIX/demiurge-chain](https://github.com/QOR-MATRIX/demiurge-chain)).
Contact: [name, email].

### 2. Purpose

An independent review of the code that holds keys, signs, and decides who
someone is, before anything holds value. The report is evidence for our
release gates (section 7).

### 3. Scope, pinned

All public components at **demiurge-chain `e99c7ce62045da08701a2ad6f7769a1a1d87afe6`**
(the merge of #107, 27 October 2026), unless the firm and we agree a later
commit before work starts.

| # | Component | Path | Language | Live? | Notes |
|---|-----------|------|----------|-------|-------|
| 1 | **The QOR core** | `tools/qor-launcher/core/` with the `desktop` feature (`identity`, `social`, `chain`, `library`, `mesh`, `qontrol`); `chain/primitives/mesh-proof/` | Rust | In the Windows launcher; in QOR OS (private) at `6a4822d`, with this pin's core code | Holds keys. `src/vault/`, `profiles.rs`, `confirm.rs`, `pay.rs`, `agents.rs`, `bridge.rs` and the C bridge header `include/qor_core.h`; the chain client (subxt), QOR ID's and Social's clients, the Library index, the Mesh (iroh) |
| 2 | **The Windows launcher host** | `tools/qor-launcher/src-tauri/` | Rust (Tauri 2) | Tester pre-releases; latest `launcher-v0.1.17` (`5db8349cc1e9f5c968c65174f98df27977abadeb`) | Draws the host dialogs before signing; keychain use (ADR-056); the update channel (ADR-111); `qor://pay` handling (ADR-076, ADR-077) |
| 3 | **The chain runtime and pallets** | `chain/runtime/`, `chain/node/`, `chain/pallets/arq-wallet`, `drc369`, `drc369-royalties`, `mesh`, `validator-set`, `chain/primitives/` | Rust (Polkadot SDK `polkadot-stable2606-1`) | The devnet at `wss://rpc.qorsync.dev`, runtime `spec_version` 11 [confirm the deployed commit] | Money paths in integer embers (`u128`); ADR-035's arithmetic rules |
| 4 | **QOR ID** | `services/qor-auth/` | Rust, Postgres, Redis | `https://id.qorsync.dev` [confirm the deployed commit] | Password and key sign-in, sessions, backup codes, email flows (Resend), OAuth 2.1 with PKCE (ADR-073), agents (ADR-014, ADR-112), avatars (ADR-079) |
| 5 | *Optional:* **Social** | `services/social/` | Rust, Postgres | Live since 16 October 2026 | Messages sealed at rest, readable by the service (ADR-091) |
| 6 | *Optional:* **ARQADE's payment signer** | `products/arqade/lib/pay.ts` and related | TypeScript | ARQADE's website | Signs `qor://pay` requests |
| 7 | *Round 2:* **QOR OS's vault service** | QOR-MATRIX/qor-os (private) [commit, once built] | C++ and the core through its bridge | Not built (Phase 2) | The vault as its own service outside Prism and every app; approvals drawn by Prism in a trusted layer ([VISION.md](../../VISION.md) §3, [QOR-0003](../decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md)) |

**Out of scope:** frozen code (`apps/`, `cli/`, `sdk/`, `packages/`,
`client/`, including the wallet extension) and dead code (`aeons/`,
`archons/`, `syzygies/`); Demiurge's QQ, ARQADE's Qt app and QOR Media, unless
the firm proposes otherwise.

**Build and test.** Each component builds and tests as demiurge-chain's CI
does (`.github/workflows/ci.yml`; `chain/README.md`; each service's
`Cargo.toml` and README). QOR OS builds the core through `core/CMakeLists.txt`
and tests it with `tst_qorcore` ([BUILDING.md](../BUILDING.md)). [We will give
the firm exact commands and a working build on request.]

### 4. Threat model, in outline

We will give the firm a fuller model before work starts. In outline:

- **Assets.** Recovery phrases and wallet keys; profile PINs; QOR ID
  passwords, sessions, refresh tokens, backup codes and reset links; agent
  authorisations; test CGT and DRC-369 assets (and later real ones); the
  devnet's sudo key; the launcher's update key; the Store's signing root.
- **Actors.** A remote attacker on the internet; a malicious app, QQ game,
  Store widget or AI agent on the same machine; a malicious `qor://` link or
  website; a compromised or curious service operator; a network attacker; a
  stolen or lost device.
- **Trust boundaries.** The core's C bridge (one JSON request in, one JSON
  answer out; no command answers with a key); the host dialog before any
  signature; QOR ID's OAuth flows and token introspection; the chain's
  extrinsics and their origins; `qor://pay` and `qor://agent` links; the
  update channel; email links; the vault service and Prism's trusted layer
  (round 2).
- **Questions we want answered.** Can anything obtain or use a key without
  the person's approval? Can an approval be spoofed or misread? Can an
  amount be changed, rounded or truncated between display and signature? Can
  an account be taken over through recovery, email or OAuth? Can a pallet be
  made to pay twice, pay the wrong account or create CGT? Can an agent exceed
  its authorisation?

### 5. Known issues and accepted risks

Do not re-report these as new findings. Comment on them if the firm disagrees
with how they are handled. All are in demiurge-chain's
[`SECURITY.md`](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/SECURITY.md)
except the core's `ruzstd` ignore, which is in
`tools/qor-launcher/core/.cargo/audit.toml` and `docs/GATES.toml`'s change
log:

- Ed25519 accepts a forged signature for a small-order public key, by the
  SDK's ZIP-215 rule (ADR-038). Account keys are Sr25519.
- Social can read the messages it holds (ADR-091 decision 4).
- The launcher trusts Mozilla's list of translation records as served over
  TLS (ADR-092 decision 3).
- QOR ID's sign-up limit trusts `X-Real-IP` from Railway's edge (ADR-078).
- No limit on concurrent QOR ID sessions; sessions store a placeholder IP.
- "QOR ID weaknesses that remain" and "Accepted lockouts in QOR ID".
- Dependency advisories accepted with conditions, each ignored only while a CI
  step proves it does not apply: RUSTSEC-2023-0071 (`rsa`), RUSTSEC-2026-0344
  (`ruzstd`: ADR-089 for the launcher host; for the core, ADR-121: Proposed
  at `e99c7ce`, accepted on 27 October 2026, on demiurge-chain's `main` since
  `694e5d8`), RUSTSEC-2026-0383 (ADR-107), RUSTSEC-2026-0387 (ADR-115).
  The log is `docs/GATES.toml` `[[change_log]]`.
- `chain/`'s advisories: measured on 20 September 2026 (10 vulnerabilities,
  13 warnings); "Not re-measured since"; CI's later reports list 20
  vulnerabilities and 14 warnings (20 October) and 20 and 15 (24 October).
- The recovery phrase exposed on 5 October 2026 ([SIGN-IN.md](SIGN-IN.md)),
  to be replaced before the review starts.
- Nine credential-shaped values in a private archive's history, not in the
  public repository.

### 6. How we expect the work done

- Source review and testing of the pinned commits, with our test suites.
- Live testing only against the devnet or a staging copy we provide, within
  written rules of engagement: [addresses, accounts, hours, contacts]. No
  testing against real people's accounts.
- Findings shared as they are found if high or critical, not held for the
  report.

### 7. What the report must contain

- For each finding: a severity on a stated scale (Critical, High, Medium,
  Low, Informational) with its definition; the commit and file it applies
  to; how to reproduce it; its impact; advice to fix it.
- A status for each finding that maps onto our gates:
  - **Demiurge's Beta gate** (`beta.external-review`) needs every Critical and
    High finding **closed**, confirmed by retest;
  - **public release** (`public-release.review-closed`, and QOR OS's gate)
    needs **every** finding closed, or accepted in writing by the owner, with
    the reason.
- What was in scope, what was not reached, and what assumptions were made.
- A summary we may publish, separate from the full report [if the firm
  agrees].
- No secrets in the report: no key, token, password or personal data, even
  from test accounts.

### 8. Retest

The price includes a retest of every fix made within [N] weeks of the final
report, ending in a letter that names the commit retested and the status of
each finding. Further retests: [priced per day].

### 9. Confidentiality

QOR OS's source is private and proprietary. The firm signs [a
non-disclosure agreement] before receiving it, keeps it only for the review,
and deletes it after. demiurge-chain is public.

### 10. Timeline, budget and proposals

- Start: [date]. Report: [date]. Retest window: [dates].
- Budget: [the owner's figure, or "please quote"].
- Proposals should give: the team and their relevant experience; method;
  days per component; price for round 1 and, separately, round 2; sample
  reports; references; availability; insurance.

---

## Choosing a firm

- **Relevant experience:** Substrate and the Polkadot SDK (pallets, runtime
  upgrades, origins, weights); Rust; key management and wallets; OAuth 2.1
  and identity services; for round 2, C++ and Qt, and desktop trusted-UI
  design.
- **Independence:** no stake in Demiurge or QOR OS and no conflict with the
  findings.
- **Published work:** public reports for comparable systems.
- **Retest included,** and a final letter suitable as gate evidence.
- **Handling private code** and data responsibly.
- **References** from teams of similar size.

## Groundwork before the review

Doing these first saves paid days and keeps the report about real issues.

1. **Replace the 5 October key** ([SIGN-IN.md](SIGN-IN.md#the-5-october-key)).
2. **Re-measure `chain/`'s advisories by exact version**, the task
   `SECURITY.md` says is outstanding, and update what a patch release fixes.
3. **Close the engineering items** in demiurge-chain's
   `docs/audit/RECONCILIATION.md` §8: the welcome grant's hash verification,
   ARQ Wallet saturation, the `qor://pay` double-pay guard, and the Mesh's
   amount parser.
4. **Freeze the scope**: name the commit, and give the deployed commits of
   QOR ID and the devnet's runtime.
5. **Write the fuller threat model** from section 4.
6. **Prepare build and test instructions** for each component, run once from
   a clean machine.
7. **Produce a dependency inventory** for the core and the launcher (the core
   compiles 497 crates for QOR OS's build).
8. **For round 2:** build QOR OS's vault service (Phase 2) and pin it.

## Sources

| What | Where |
|------|-------|
| QOR OS's gate; vault service plan | [ROADMAP.md](../ROADMAP.md); [QOR-0003](../decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md); [QOR-0008](../decisions/QOR-0008-before-a-public-release.md); [VISION.md](../../VISION.md) §3; [UPSTREAM.md](../UPSTREAM.md) (the pin) |
| The pin | [`core/CMakeLists.txt`](../../core/CMakeLists.txt): `QOR_CORE_COMMIT 6a4822d10c93694d6d12abf33a75b4df6cac918a`, features `desktop`; the core's code as at `e99c7ce`, its manifest's licence and two comments aside |
| Gate criteria | demiurge-chain origin/main `e99c7ce`: `docs/GATES.toml` (`beta.external-review`, `public-release.review-closed`) |
| Unaudited status, known issues, advisories, live surfaces | `SECURITY.md` |
| Roadmap item M7.3 | `docs/DIRECTION.md` |
| Engineering items before the review | `docs/audit/RECONCILIATION.md` §8 |
| Components and features | `tools/qor-launcher/core/Cargo.toml` (`[features]`); `chain/` tree; `services/qor-auth/`; `docs/SYSTEMS.md` |
| Releases | `gh release list -R QOR-MATRIX/demiurge-chain`, 27 October 2026; `git rev-list -n1 launcher-v0.1.17` |
| The core's `ruzstd` ignore; ADR-121's state | `tools/qor-launcher/core/.cargo/audit.toml` and `docs/GATES.toml` `[[change_log]]` at `e99c7ce`; ADR-121, accepted, on demiurge-chain's `main` at `694e5d8` (#108) |
| Dependency count of the core | `cargo tree` of the core at `e99c7ce`, `desktop`, Linux target: 497 crates (research of 27 October 2026) |
