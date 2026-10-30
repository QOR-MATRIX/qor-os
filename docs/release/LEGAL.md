# Draft for counsel, not legal advice: terms, privacy and the legal opinion

*Prepared on 27 October 2026 by an assistant, from the research listed under
[Sources](#sources), for the owner and their advisers. It is research, not
legal advice.*

**Draft for counsel, not legal advice.** This brief states facts from the
code and records of QOR OS and demiurge-chain as they stood on 27 October
2026. It draws no legal conclusion. Where a fact is unknown it says so, or
leaves a [placeholder].

**The gate** ([ROADMAP.md](../ROADMAP.md#release-gates)): "Terms of Service, a
privacy policy, and a legal opinion on rewards, prizes, identity checks,
sanctions and age limits".

## What this covers

[QOR-0008](../decisions/QOR-0008-before-a-public-release.md) adopts
"Demiurge's legal list". Its context names four items: Terms of Service and a
privacy policy; a legal opinion on rewards, prizes, identity checks,
sanctions and age limits; code-signing certificates; an external security
review. The last two have files of their own ([SIGNING.md](SIGNING.md),
[SECURITY-REVIEW.md](SECURITY-REVIEW.md)), and so do the 5 October key and QOR
ID's second region ([SIGN-IN.md](SIGN-IN.md)). This file covers the first two.

Demiurge's `OWNER.md` (origin/main `e99c7ce`, dated 26 October 2026), "What
only you can do", item 6, verbatim: "**Terms of Service and a privacy policy, with a lawyer,** and
**the legal opinion** on rewards, prizes and **KYC** — *identity checks* —
before anything carries value."

The same list appears in demiurge-chain's `HANDOFF.md` §4.0 (rows "Terms of
Service, a privacy policy, deleting and exporting an account" and "Legal
review", both open, the owner's) and `docs/audit/RECONCILIATION.md` §8 ("Open,
the owner's and a lawyer's").

[QOR-0006](../decisions/QOR-0006-qor-os-1-0-moves-no-money.md) makes QOR OS's
paid items wait for this list.

**The operator** of every service below is [Legal entity, its country and
registered address]. It is not established; see
[README.md](README.md#one-question-first-which-legal-entity).

---

## Part 1. Terms of Service and a privacy policy

### Current state

- Neither document exists for any service. A search of demiurge-chain's
  origin/main for terms, privacy, legal or policy files found none.
  `site/.well-known/security.txt` points its `Policy:` line at `SECURITY.md`,
  which is a vulnerability policy.
- ARQADE shows a stopgap notice (`products/arqade/app/systems.tsx`): "This is
  an informational preview notice, not final Terms of Service or sweepstakes
  rules. Final terms will need to identify the operator, jurisdiction,
  eligibility, odds, entry methods, prizes, redemption and privacy practices."
- ARQADE's `IMPLEMENTATION.md` lists as not implemented: "Final Terms of
  Service, official promotion rules, operator identity, jurisdiction/age
  eligibility, purchase-free entry, and approved prize economy."
- **No route lets a person delete or export their own account.** Only an
  administrator's ban exists (`HANDOFF.md` §4.0). That row says the documents
  are "Needed before inviting more people, and before any public beta."
- QOR OS has no hosted service of its own today. Its first public build will
  also need an end-user licence: Qt's published terms appear to require one
  (§5.4.1(ii) and (x); the letter's question 6 asks Qt, see [QT.md](QT.md)).

### Services and what each holds

| Service | Live since | Where it runs | What it holds | Source |
|---------|-----------|---------------|---------------|--------|
| **QOR ID** (`https://id.qorsync.dev`) | 1 October 2026 | Railway, project `demiurge`, region `iad`: the service, its Postgres and Redis | See the inventory below | `docs/SYSTEMS.md`; `services/qor-auth/` |
| **Social** | 16 October 2026, for the owner's own use until the content policy is decided (L8.7) | Railway, its own Postgres | Profiles, rooms, members, invites, messages, reactions, blocks, reports, rate limits, mutes, friend requests, friendships, circles, "elites" | `services/social/migrations/001` to `003` |
| **World** (in Social) | 17 October 2026 | As Social | One room for everyone who opens Social, "kept in full" | `SECURITY.md`; ADR-092 |
| **ARQADE's website** | [date] | Vercel, project `qor-arqade` (Hobby plan), Postgres on Neon (ADR-074). ADR-074: the Hobby plan is for non-commercial use; the plan changes before any real payment | Players, matches, standings, messages and reports, QOR ID sign-in tables, tips, and whether a player's first match was reported. It signs in through QOR ID (OAuth 2.1, ADR-073) | `docs/SYSTEMS.md`; `products/arqade/IMPLEMENTATION.md` |
| **The homepage** (`qorsync.dev`) | 18 October 2026 | Vercel, project `demiurge-site` | [whether any analytics or logs: check] | ADR-100 |
| **The devnet** (`wss://rpc.qorsync.dev`) | 3 October 2026 | Railway: two validators and an RPC node (ADR-068) | A public ledger: addresses, test balances, assets, transactions. A record cannot be removed from it individually; the devnet as a whole can be reset (`site/index.html`) | `SECURITY.md`; `site/index.html` |
| **QOR Launcher** (Windows, tester builds) | 17 October 2026 (0.1.10) | The user's PC | Kept locally. It downloads translation models from Mozilla (ADR-092) and checks GitHub for updates at start and every six hours unless turned off (ADR-111) | `SECURITY.md`; ADR-111 |
| **QOR OS** | Not released | The user's machine | Local-first: "QOR ID connects people; it is never required to use the machine" ([VISION.md](../../VISION.md), P6). The Store's stage 2 plans opt-in, aggregated analytics ([STORE.md](../STORE.md)) | QOR OS tree |

### QOR ID's data, from its migrations

From `services/qor-auth/migrations/001` to `025` and `SECURITY.md`:

| Data | Where | Notes |
|------|-------|-------|
| Usernames, emails, pending email changes | `users` (001, 016, 019, 023) | Usernames unique; a chosen name for automatic `key_…` accounts |
| Password hashes | `users` | Argon2 |
| Email verification and password-reset links | `email_verifications`, `password_resets` | Sent through Resend from `noreply@demiurge.cloud` |
| Backup codes | `backup_codes` (014) | Only a SHA-256 of each code; each works once |
| Suppressed email addresses | `email_suppressions` (017) | Only a SHA-256 of the address; from Resend's bounce and complaint reports |
| Resend webhook deliveries | `resend_webhook_deliveries` (017) | [fields: check] |
| Sessions | Redis | The IP address stored is a placeholder, `0.0.0.0`; `last_used_at` is kept |
| Linked public keys and sign-in challenges | `linked_wallets`, `auth_challenges` (006) | Key sign-in |
| Agents | `users` (007, 011, 025) | Registered with the controller's key (ADR-014, ADR-112). A spending limit per agent is "recorded and not enforced" (025) |
| Progress, XP and tasks | `progress_events` (020) | Written only by QOR ID or a registered app's server |
| Welcome grants owed | `welcome_grants` (020) | "One per account, per email address and per chain account (the unique keys), whatever happens to the account later" |
| The sign-up network address | `signup_addresses` (020) | "a keyed hash of the network address an account was created from, never the address, kept 30 days" |
| Avatars and their reports | `avatars`, `avatar_reports`, `avatar_removals` (022) | Re-encoded at 256 × 256 and public at `/avatars/{hash}` |
| Audit rows | `audit_log` (001) | Columns `user_id`, `action`, `details`, `ip_address` and `user_agent`. At `e99c7ce` the service writes only `user_id`, `action` and `details`. [Whether older rows hold IP addresses: check] |
| Attestations | `attestations` (001) | Typed for `age_verification`, `region_verification`, `kyc_complete` and `reputation_threshold`, with a proof hash and public inputs. No code reads or writes it at `e99c7ce`; its routes answer that attestations are not implemented. [Rows: check] |
| Music tables | `music_tracks`, `playlists`, `playlist_tracks`, `track_likes` (004) | [whether used today: check] |
| Retired donation tables | schema `retired` (024) | Migration 024 moved `donations`, `donor_profiles` and `subscription_events` there whole, unread by the service, so as not to delete what "might be a record of a payment". [Whether any rows exist: check] |

No migration of QOR ID (001 to 025) or Social (001 to 003) stores a date of
birth, a country or a jurisdiction.

### Subprocessors

| Who | What for | Data it sees |
|-----|----------|--------------|
| Railway | QOR ID, its Postgres and Redis; Social and its Postgres; the devnet's nodes (region `iad`) | Everything those services hold |
| Vercel | ARQADE's website; the homepage | Requests; ARQADE's app. ARQADE is on the Hobby plan, which Vercel limits to non-commercial use; demiurge-chain plans a commercial plan before real payments (`HANDOFF.md` §4.0, ADR-074) |
| Neon | ARQADE's Postgres | ARQADE's data |
| Resend | QOR ID's email | Email addresses and message content; bounce and complaint reports |
| GitHub | Source, releases, the launcher's update channel | Downloaders' and updaters' requests |
| Mozilla | Translation models the launcher downloads | Download requests (no message text is sent) |
| Model providers, on the user's own account | QQ's Agent panel (ADR-085; `OPEN_QUESTIONS.md` U-19) | What the user sends them |

Each one's processing terms and data locations: [check each vendor's current
data processing agreement].

### Deleting and exporting an account: what the routes must settle

Engineering builds these once the policy says what they must do. Facts that
bear on the policy:

- The chain is public. A single record on it cannot be deleted; the devnet
  as a whole can be reset.
- World keeps its messages in full. Social can read the messages it holds
  (ADR-091 decision 4).
- `welcome_grants` keeps its unique keys (account, email, chain account)
  "whatever happens to the account later".
- `signup_addresses` keeps a keyed hash for 30 days. `email_suppressions` keeps
  a hash of addresses with no account.
- Avatars are public at a URL until removed.
- `audit_log` and the `retired` schema hold rows the service keeps for
  records.
- Backups: [where, how long; Railway's backups: check].
- Demiurge's rule that nothing secret reaches a log (its `AGENTS.md` §9;
  QOR OS's [`AGENTS.md`](../../AGENTS.md) §6) applies to both routes: no email
  address, token or link in any log.

### Questions for counsel: terms and privacy

1. Who must be named as operator, and where? Which law governs, and which
   courts or arbitration?
2. Which privacy laws apply, given an operator in [country] and users
   anywhere? Which lawful bases fit each item in the inventory?
3. What retention period fits each item, including the hashes kept after an
   account is gone, the welcome-grant keys, audit rows and backups?
4. How does a right to erasure meet data on a public chain, and World's full
   history?
5. What must be disclosed about Social reading the messages it holds, about
   moderation and reports, and about World?
6. Children: what minimum age, and what follows from not collecting a date of
   birth today?
7. International transfers: the services run in Railway's `iad` region. What
   is needed for users outside the US?
8. Which agreements are needed with each subprocessor?
9. What must the terms say about content people make and publish: QQ games,
   QML written by an AI assistant, Store items, avatars? A licence to host and
   show it; notice and takedown; developer terms for the Store.
10. Agents act for a person (ADR-014, ADR-112). What must the terms say about
    an agent's actions and its spending limit?
11. What must QOR OS's end-user licence contain, beside what Qt requires?
12. Cookies or similar on ARQADE's website and the homepage: what consent is
    needed?
13. Breach notification: what duties apply, and to whom?
14. Hosting plans: which providers' terms (for example Vercel's Hobby plan)
    limit commercial use, and when must they change?
15. QQ, ARQADE's app and games, and QOR Media came from demiurge-chain under
    Apache-2.0 and are developed in QOR OS since 29 October 2026
    ([QOR-0015](../decisions/QOR-0015-the-products-are-developed-here.md)).
    [`LICENSE`](../../LICENSE) keeps the 374 files that came
    ([`LICENSES/from-demiurge-chain.txt`](../../LICENSES/from-demiurge-chain.txt))
    under Apache-2.0, and puts later changes to them and new files under QOR
    OS's proprietary licence, relying on Apache-2.0 §4. Is that wording
    sound? What must a distributed build carry for them? Do ARQADE's five rule
    modules, kept identical and Apache-2.0 in both repositories, need anything
    more?

---

## Part 2. The legal opinion

Demiurge's `HANDOFF.md` §4.0, "Legal review" row: "A legal opinion on
backing, paid games with prizes, the welcome grant, task rewards, ADR-102's
Spark rewards for play, and making CGT exchangeable, before any of them goes
live (U-15 to U-18). With it: which KYC, AML, sanctions screening, geofencing
and age gating apply, and a public statement that devnet balances never
carry into a network with value."

### Everything today is test CGT on a devnet

- The chain is "an unaudited devnet with test value only" (QOR-0006).
  Demiurge's `SECURITY.md`: "Do not hold real value on it."
- The homepage says "Test CGT has no value, and the test network can be
  reset" (`site/index.html`).
- QOR OS 1.0 moves no money: free items and test CGT only
  ([QOR-0006](../decisions/QOR-0006-qor-os-1-0-moves-no-money.md)).
- No identity checks, sanctions screening, geofencing or age gating exist.

### The mechanics, as built or designed

Demiurge's records are in `docs/decisions/` of
[demiurge-chain](https://github.com/QOR-MATRIX/demiurge-chain/tree/main/docs/decisions);
open questions in `docs/economics/OPEN_QUESTIONS.md`.

| # | Mechanic | As built or designed | Record and state | What moves today |
|---|----------|----------------------|------------------|------------------|
| 1 | **Tips** by a signed `qor://pay` request | ARQADE's server signs a payment request. The launcher checks the signature, the genesis hash ("devnet only") and a cap of 100,000 CGT per request, then asks in its host dialog | ADR-076, ADR-077: accepted; built | Test CGT. "The only CGT that moves is a tip, signed in the player's launcher and confirmed from finalised blocks"; tips are "Devnet test CGT only" (ARQADE's `IMPLEMENTATION.md`) |
| 2 | **ARQ Wallets** | One keyless payout account per published game, derived from its DRC-369 Cartridge and governed by its holder; a payout authority bounded by an on-chain policy; each outcome paid once; loosening and withdrawals delayed; prizes reserved; small payouts accrued | ADR-070: accepted; `pallet-arq-wallet` built, with prizes held before a paid round (decision 8) | Test CGT only |
| 3 | **The platform reward pool** for free play | Players rewarded in Sparks from one pool, ARQADE's own ARQ Wallet, funded by the owner, its policy enforced on chain, paid only for runs the ARQADE service has replayed. Play is free, so no entry leads to a prize | ADR-102: accepted. Confirming a run by replay: ADR-116, **proposed**. The pool's numbers (largest payout, budget per epoch, most per player) are not set | Production payouts wait on U-16 and the legal review (`docs/DIRECTION.md`) |
| 4 | **The welcome grant** and task rewards | 100 CGT once the tutorial, email and key are confirmed, from a Welcome account with no daily cap (the owner's choice; its balance is the only cap), devnet first; three accounts per network address; an abuse watcher. Levels and XP touch no CGT | ADR-078: accepted, grant at 100 CGT. U-18 open | Nothing: "QOR ID records each owed grant and pays nothing" |
| 5 | **Paid games** | Games free or up to 10,000 CGT, bought as DRC-369 licences; non-fungible in-game items | ADR-071: accepted | [whether a paid game exists on the devnet: check] |
| 6 | **Paid play and prizes** | ARQADE removed Energy, the entry cost and the random draw (P7.23). Whether a paid entry may lead to a prize, skill only or chance too, is open | U-16 open | Nothing paid in production "until this item is decided" |
| 7 | **Backing and memberships** | Backers receive the work (DRC-369 rewards), never its proceeds. Threshold for review, missed milestones and holding periods are open | ADR-071; U-17 open. In ARQADE only tips are built (`IMPLEMENTATION.md`) | Nothing beyond tips |
| 8 | **Asset rental** | A holder offers an asset for a price per period of blocks (zero lends it); the renter pays up front; the rent is divided as a sale is, so the creator's royalty is paid on rent | ADR-119: accepted under the owner's delegation of engineering choices; built and merged (#106); the devnet runs it (`spec_version` 11) | Test CGT |
| 9 | **Fractional ownership** | Not built. It "waits for the legal review" (`docs/DIRECTION.md` M4.3) | — | Nothing |
| 10 | **Royalties** | A creator's share of each sale (and rent), settled on chain in `pallet-drc369-royalties` | ADR-061, ADR-062: accepted; built | Test CGT |
| 11 | **The Mesh** | Seeding and fetching games and creations, paid from hosting escrow; the node signs its own work | ADR-095, ADR-097. In the core QOR OS builds; QOR OS keeps it off until turned on ([ROADMAP.md](../ROADMAP.md), Phase 4) | Test CGT |
| 12 | **A marketplace settled in dollars** | The owner proposed a marketplace that settles in USD with a fixed share; the share and the rail are open. U-15 notes that a fiat rail "can make a product a money transmitter in some jurisdictions" | U-15 open. The chain settles sales in CGT and takes no share (ADR-061) | Nothing |
| 13 | **Making CGT exchangeable** | Not designed | `HANDOFF.md` §4.0 | Nothing |
| 14 | **Emulated play** of a person's own game files | Off until the legal opinion approves it | ADR-110: **deferred** | Nothing |
| 15 | **QOR OS Store, paid stage** | Designed, not built: a prepaid balance in dollars, card and other payment methods through a processor, a commission on paid items, payouts to developers. Stage 2's developer account fee is a recommendation | [STORE.md](../STORE.md); QOR-0006 decision 3: waits for U-15, a QOR record choosing the model, this list and Qt's answer | Nothing; stage 1 has free items only |

Undecided shares and fees are deliberately not repeated here as numbers;
they are open (Demiurge's `AGENTS.md` §5, QOR OS's §4).

### Questions matrix

● marks a question counsel is asked to answer for that mechanic. Counsel may
add others.

| Mechanic | Lawful where, and on what terms? | Skill, chance or neither? | KYC and AML | Sanctions screening | Geofencing | Minimum age | Payments, virtual-currency or money-transmission rules | Consumer and tax rules |
|----------|---|---|---|---|---|---|---|---|
| 1. Tips | ● | | ● | ● | ● | ● | ● | ● |
| 2. ARQ Wallet payouts and prizes | ● | ● | ● | ● | ● | ● | ● | ● |
| 3. Reward pool for free play | ● | ● | ● | ● | ● | ● | ● | ● |
| 4. Welcome grant, task rewards | ● | | ● | ● | ● | ● | ● | ● |
| 5. Paid games | ● | | ● | ● | ● | ● | ● | ● |
| 6. Paid play and prizes | ● | ● | ● | ● | ● | ● | ● | ● |
| 7. Backing and memberships | ● | | ● | ● | ● | ● | ● | ● |
| 8. Rental | ● | | ● | ● | ● | ● | ● | ● |
| 9. Fractional ownership | ● | | ● | ● | ● | ● | ● | ● |
| 10. Royalties | ● | | ● | ● | ● | | ● | ● |
| 11. The Mesh's paid hosting | ● | | ● | ● | ● | ● | ● | ● |
| 12. Marketplace in dollars | ● | | ● | ● | ● | ● | ● | ● |
| 13. CGT exchangeable | ● | | ● | ● | ● | ● | ● | ● |
| 14. Own game files in an emulator | ● (copyright) | | | | ● | | | |
| 15. Store's paid stage | ● | | ● | ● | ● | ● | ● | ● |

**Questions that cut across:**

1. Does it matter that everything is test CGT on a devnet today, and what
   must be true before any mechanic carries value?
2. Does an on-chain mechanic (a pallet anyone can call) need the same gating
   as one behind a service? Where can gating sit, given that the chain cannot
   check identity or location?
3. Agents can act for a person, with a recorded spending limit (QOR ID
   migration 025). Does that change any answer?
4. Which mechanics need terms of their own (official rules for promotions,
   backing terms, developer terms)?

### A public statement for counsel to approve

`HANDOFF.md` §4.0 asks for "a public statement that devnet balances never
carry into a network with value". The homepage's line is close but not
explicit. A draft, for counsel, and for the owner, because it is a
commitment:

> Demiurge's devnet is a test network. Its CGT is test CGT: it has no value,
> and the network can be reset. No balance, asset or record on the devnet
> will carry into any other network.

It should not appear on `qorsync.dev` until counsel and the owner approve it.
Check it against [`AGENTS.md`](../../AGENTS.md) §5 (no statement about CGT's
price or future value) as well as with counsel.

### What age gating and geofencing would need

Nothing is collected today that would allow either. QOR ID's `attestations`
table, typed for age verification, region verification and KYC, exists from
migration 001 but no code uses it. Each would add personal data to Part 1's
inventory:

- **Age:** a date of birth or an age statement at sign-up in QOR ID, and a
  check wherever a gated mechanic starts.
- **Location:** a declared country, a check of the network address at the
  moment of use, or both.
- **Sanctions and KYC:** a screening or verification provider (a new
  subprocessor) and the name and address data it needs.
- **Where to enforce:** in services (QOR ID, the ARQADE service that confirms
  runs and signs `qor://pay`), since the chain cannot.

### What the answers unblock

U-15 to U-18 (Demiurge); ADR-116's run confirmation in production; ADR-110's
own-files mode; fractional ownership; QOR OS's paid Store stage (QOR-0006);
and real CGT anywhere.

## Who to engage

Counsel for: privacy and online terms; gaming, sweepstakes and promotions;
payments and virtual-currency regulation; sanctions and export; minors. One
firm or several: [the owner's choice].

## Sources

| What | Where |
|------|-------|
| The owner's list, verbatim (dated 26 October 2026) | demiurge-chain origin/main `e99c7ce`: `OWNER.md` |
| §4.0 rows: Legal review; ToS, privacy, delete and export | `HANDOFF.md` |
| Open items for the owner and a lawyer | `docs/audit/RECONCILIATION.md` §8 |
| U-15 to U-19 | `docs/economics/OPEN_QUESTIONS.md` |
| Unaudited status; live surfaces; Social; Mozilla; sessions | `SECURITY.md` |
| Hosting | `docs/SYSTEMS.md`; `services/qor-auth/DEPLOY-RAILWAY.md`; ADR-015, ADR-063, ADR-068, ADR-074, ADR-100 |
| QOR ID's and Social's schemas | `services/qor-auth/migrations/001` to `025`; `services/social/migrations/001` to `003` |
| ARQADE's notice and what is not implemented | `products/arqade/app/systems.tsx`; `products/arqade/IMPLEMENTATION.md` |
| Mechanics | ADR-061, ADR-062, ADR-070, ADR-071, ADR-076, ADR-077, ADR-078, ADR-085, ADR-091, ADR-092, ADR-095, ADR-097, ADR-102, ADR-110, ADR-111, ADR-112, ADR-116, ADR-119 (indexed in `docs/decisions/README.md`); `docs/DIRECTION.md` (M4.3; production payouts) |
| The homepage's test-CGT line; `security.txt` | `site/index.html`; `site/.well-known/security.txt` |
| QOR OS's records | [QOR-0006](../decisions/QOR-0006-qor-os-1-0-moves-no-money.md), [QOR-0008](../decisions/QOR-0008-before-a-public-release.md), [STORE.md](../STORE.md), [VISION.md](../../VISION.md) |
