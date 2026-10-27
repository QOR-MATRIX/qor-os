# Sign-in: QOR ID's second region, offline login and the 5 October key

*Prepared on 27 October 2026 by an assistant, from the research listed under
[Sources](#sources), for the owner and their advisers. It is research, not
legal advice.*

**The gate** ([ROADMAP.md](../ROADMAP.md#release-gates)): "QOR ID in a second
region with offline login; the key exposed on 5 October 2026 replaced".
[QOR-0008](../decisions/QOR-0008-before-a-public-release.md): "QOR ID, Social
and the devnet run on one hosting provider in one region. An operating system
that signs people in through QOR ID needs redundancy and offline login first."

This gate has three parts with different owners. The key is the owner's
alone and is blocked by nothing else, so it comes first here.

## The 5 October key

### Current state

Demiurge's `SECURITY.md` (origin/main `e99c7ce`), verbatim:

> "A recovery phrase was pasted into a chat on 5 October 2026. Treat that key
> as exposed and replace it. If it is the devnet's sudo key, replace it with
> `sudo.setKey` from the current sudo account, signed in the owner's wallet,
> then move anything the old account holds. The devnet holds nothing of value
> (ADR-037), but whoever has its sudo key can replace its runtime."

- `HANDOFF.md` §4 item 53 records that a 24-word phrase was pasted and that
  "whatever account it controls must be treated as exposed and replaced". Its
  §4.0 row "The exposed key" is open, the owner's.
- No current document says **which account** the phrase controls, or that it
  was replaced.
- The devnet's sudo key has signed runtime upgrades since 5 October (to
  `spec_version` 9 on 17 October, 10 on 23 October and 11 on 26 October;
  `HANDOFF.md` §4 items 88, 136 and 160). If the exposed phrase is the sudo
  key and no `sudo.setKey` has been made since (none is recorded), it is
  still the live one; step 2 checks.
- `SECURITY.md` separately lists nine credential-shaped values from a private
  archive's history. That is a different item, not this gate.

### Rules for this runbook

- **It contains no key material**, and none is ever added to it: no phrase,
  no private key, no seed, no password.
- **Every step that touches the phrase or the wallet is the owner's.** An
  assistant never sees, handles, types, stores or asks for the phrase, in a
  chat, a file, a terminal or a screenshot.
- **Never paste the phrase anywhere**, including into an AI tool, a search
  box, a document or a message.
- Records name only **dates and public addresses**.

### The runbook

| Step | Who | What | Record |
|------|-----|------|--------|
| 1 | **Owner** | On your own machine, in the wallet that holds the exposed phrase, find which account (or accounts) it restores. Write down **only the public address** of each. If the phrase is not in a wallet now, restore it into a wallet on a machine that is offline, read the address, and remove it again | The public address(es) |
| 2 | Owner, or an assistant | Read the devnet's current sudo key, which is public: the Sudo pallet's `key` storage at `wss://rpc.qorsync.dev` (for example in Polkadot-JS Apps, Developer › Chain state › `sudo.key()`). Compare it with step 1's address in the same address format (or compare the public keys in hex). `HANDOFF.md` §4 item 136 recorded the sudo key on 23 October 2026 as `5HN6PZA4…Zadn` | Whether they match |
| 3 | **Owner** | If they match: create a **new** account in your wallet, on your own machine. Write its recovery phrase on paper and store it offline, as you keep your other keys. Never type it into a chat | The new public address |
| 4 | Owner, or an assistant | Rehearse on a local development chain (`--dev`, as `chain/README.md` describes, and as was done for the runtime upgrades): call `sudo.setKey` from the development sudo account to a development account, then read `sudo.key()` back. Development accounts' keys are public test keys, so an assistant may run this | That the rehearsal passed |
| 5 | **Owner** | Submit `sudo.setKey(<new address>)` on the devnet, from the current sudo account, signed in your wallet. Do it promptly once started: anyone else with the phrase could also call it | The block number |
| 6 | Owner, or an assistant | Read `sudo.key()` from `wss://rpc.qorsync.dev` again. It must name the new address | The read-back |
| 7 | **Owner** | Move anything the old account holds (test CGT, DRC-369 assets, any role it has, such as an ARQ Wallet's payout authority) to accounts you control | What moved |
| 8 | **Owner** | Check whether the same phrase backs any other account: other derivation paths; a launcher vault profile; a key linked to a QOR ID account for key sign-in. Replace each: unlink the old key from QOR ID and link a new one; retire the old profile | Each public address replaced |
| 9 | **Owner** | If the address in step 1 is **not** the sudo key: do steps 7 and 8 for it, and record that it was not the sudo key | — |
| 10 | Assistant, when asked | Update demiurge-chain's `SECURITY.md`, `HANDOFF.md` §4.0 and `OWNER.md`, and QOR OS's `OWNER.md`, with the date and the **public** addresses only | A records change |

Afterwards, the new sudo key belongs in the key-custody inventory
([SIGNING.md](SIGNING.md#key-custody-a-runbook-outline)).

---

## QOR ID in a second region

### Current state

- QOR ID with its Postgres and Redis, Social with its Postgres, and the
  devnet's two validators and RPC node all run on **Railway, project
  `demiurge`, region `iad`** (Demiurge's `docs/SYSTEMS.md`;
  `services/qor-auth/DEPLOY-RAILWAY.md`; `chain/DEPLOY-RAILWAY.md`; ADR-063,
  ADR-068).
- ADR-015 accepted this concentration "for the stages before mainnet", with a
  revisit before mainnet.
- `docs/GATES.toml`'s `public-release.single-provider-revisited` is unmet. It
  requires backups held off the hosting provider and validators on more than
  one provider. `docs/architecture/HOSTING.md` §4 says a single-provider
  validator set is not acceptable.
- No record covers a second region for QOR ID. Railway's usage limit is also
  still to be agreed.

### What a redundancy record in demiurge-chain must decide

The record is a Demiurge ADR (QOR ID lives there), filed as **Proposed** for
the owner. It should decide:

1. **The goal.** How long QOR ID may be down, and how much data may be lost
   in a failure (recovery time and recovery point objectives).
2. **Where.** A second Railway region, a second provider, or both. [Check
   Railway's current page for regions and multi-region features.]
3. **Postgres.** A replica in the second region; how failover happens and who
   triggers it; how the old primary is fenced off so there are never two
   writers.
4. **Backups off the provider.** Where, how often, encrypted with which key,
   how long kept, and a restore tested on a schedule. This also serves
   `public-release.single-provider-revisited`.
5. **Redis and sessions.** Replicate sessions, or accept that people sign in
   again after a failover. Refresh tokens rotate on every use (ADR-073), so a
   lost write matters.
6. **Secrets.** How the signing secrets for tokens, the Resend key and OAuth
   client settings reach the second region without entering a log or the
   repository.
7. **Traffic.** How `id.qorsync.dev` moves to the second region (DNS, health
   checks), and what OAuth clients (ARQADE) and launchers see during it.
8. **Social and the devnet.** Whether they follow the same plan, and how
   validators move to more than one provider.
9. **Cost.** A monthly figure from the providers' current pages, for the
   owner to approve.
10. **Proof.** A failover drill, run and recorded, before the gate is ticked.

## Offline login

### Current state

- QOR OS's Phase 2 item "One QOR ID sign-in at the shell, with offline login"
  is unchecked ([ROADMAP.md](../ROADMAP.md#phase-2--identity-and-keys)). No
  offline-login code exists; only [VISION.md](../../VISION.md) describes it.
- VISION's principle P6: "Playing, making and the whole OS work offline. QOR
  ID connects people; it is never required to use the machine." Its risk
  table: "Offline login, then a second region, before QOR ID signs people in
  to the OS".
- Upstream, the launcher requires an "Offline first run" (its README). L1.4
  still needs "a key sign-in after QOR ID was unreachable at unlock" tried in
  a running launcher. Demiurge's `docs/blueprints/ECOSYSTEM.md` G-5 records
  "No entitlement format, and no offline proof".
- The QOR core QOR OS builds has QOR ID's client (`identity` feature), but
  its bridge still offers four commands. The vault service is not built.

### Design questions for a QOR record in Phase 2

1. **What "offline login" means.** Signing in to the machine with QOR ID
   unreachable; using QOR ID-backed features offline; or both?
2. **The local account.** How a QOR ID binds to a local user (greetd and PAM
   with a Prism greeter, VISION §6), and whether a machine can have a user
   with no QOR ID at all (P6 says it can).
3. **The first sign-in.** Must it be online? What is cached after it: a
   session, a statement signed by QOR ID, a local verifier?
4. **Verification.** How the vault service proves the cached sign-in with the
   person's own key, and what Prism's trusted layer shows.
5. **Expiry and revocation.** How long an offline sign-in lasts; what happens
   on reconnecting; how a lost or stolen machine is cut off; whether a
   revoked session can still unlock the machine.
6. **Secrets at rest.** How the cache is protected: LUKS2 with TPM2 and a PIN
   (VISION §3), the vault, both. Limits on PIN guesses.
7. **Clocks.** Expiry depends on the machine's clock, which an attacker
   offline may control. What is safe to trust?
8. **Entitlements offline.** Which games and items work offline, and how that
   is proven without the network (ECOSYSTEM G-5 has no format yet).
9. **Several people on one machine,** and moving to a new machine.
10. **Recovery.** What happens if the local key is lost, or QOR ID's account
    is recovered online while the machine is offline.
11. **Logs.** No token, key or email address in any log (QOR OS
    [`AGENTS.md`](../../AGENTS.md) §6).
12. **Tests.** Including L1.4's missing check, scripted where possible: key
    sign-in with QOR ID unreachable.

Changes the core needs go upstream first, under demiurge-chain's rules, and
reach QOR OS by moving the pin ([UPSTREAM.md](../UPSTREAM.md)).

## Sources

| What | Where |
|------|-------|
| The gate and its reason | [QOR-0008](../decisions/QOR-0008-before-a-public-release.md); [ROADMAP.md](../ROADMAP.md) (Phase 2, Release gates); [VISION.md](../../VISION.md) (P6, §3, §6, risks) |
| The exposure and how to replace the key | demiurge-chain origin/main `e99c7ce`: `SECURITY.md` ("Status"); `HANDOFF.md` §3, §4.0 ("The exposed key"), §4 items 53, 88, 136 and 160; `OWNER.md` item 9; `docs/decisions/ADR-037-sudo-on-development-and-test-networks-only.md` |
| Hosting and its risk | `docs/SYSTEMS.md` (operations stack); `services/qor-auth/DEPLOY-RAILWAY.md`; `chain/DEPLOY-RAILWAY.md`; ADR-015, ADR-063, ADR-068; `docs/architecture/HOSTING.md` §4; `docs/GATES.toml` (`public-release.single-provider-revisited`) |
| Offline first run; L1.4; G-5 | `tools/qor-launcher/README.md`; `docs/DIRECTION.md` (L1.4); `docs/blueprints/ECOSYSTEM.md` |
| OAuth and refresh-token rotation | `SECURITY.md` ("New live surfaces"); ADR-073 |
| The core's bridge and features | [UPSTREAM.md](../UPSTREAM.md); [`core/CMakeLists.txt`](../../core/CMakeLists.txt) |
