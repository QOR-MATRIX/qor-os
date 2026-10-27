# Release gates: the owner's pack

*Prepared on 27 October 2026 by an assistant, from the research listed in each
file, for the owner and their advisers. It is research, not legal advice.*

[QOR-0008](../decisions/QOR-0008-before-a-public-release.md) decision 1 says
there is no public image, SDK or download until the conditions it lists hold.
[ROADMAP.md](../ROADMAP.md#release-gates) tracks them as six gates. On 27
October 2026 the owner answered "proceed with" to a list, written by the
assistant, that included the release gates.

An assistant cannot obtain any of the six. Each needs the owner, a vendor or
an adviser. This folder holds what they need to act: the facts as the trees
show them, the drafts, the questions and the next steps. Nothing here ticks a
gate.

| Gate | State on 27 October 2026 | Prepared |
|------|--------------------------|----------|
| 1. Qt's written confirmation | Not started | [QT.md](QT.md) |
| 2. Trademark clearance | Preliminary US search only | [TRADEMARKS.md](TRADEMARKS.md) |
| 3. Terms of Service, privacy policy, legal opinion | Nothing exists | [LEGAL.md](LEGAL.md) |
| 4. Code signing and the Secure Boot key | No certificate, no Secure Boot key | [SIGNING.md](SIGNING.md) |
| 5. External security review | None | [SECURITY-REVIEW.md](SECURITY-REVIEW.md) |
| 6. QOR ID's second region, offline login, the 5 October key | One region; not built; key not replaced | [SIGN-IN.md](SIGN-IN.md) |

ROADMAP.md's release gates are the only status; this table and the dated
lines below are a snapshot of 27 October 2026.

## One question first: which legal entity?

Every gate needs a legal person. The same answer decides:

- who holds the Qt licence, and who writes to The Qt Company;
- who owns the copyright in QOR OS and in demiurge-chain;
- who applies for the trademarks;
- who operates QOR ID, Social, ARQADE's website, the homepage and the Store,
  and is named in the Terms of Service and the privacy policy;
- who is validated for code-signing certificates and an Apple organisation
  account;
- who commissions the security review and signs the hosting contracts.

The trees name several different holders. None of them was verified as a
registered legal entity.

| Name | Where it appears |
|------|------------------|
| "QOR-MATRIX" | The GitHub organisation. QOR OS's [`LICENSE`](../../LICENSE) ("Copyright (c) 2026 QOR-MATRIX"). On the Qt licence: `LICENSE` and [`THIRD_PARTY_NOTICES.md`](../../THIRD_PARTY_NOTICES.md) say it is "held by QOR-MATRIX"; [`VISION.md`](../../VISION.md) §14 and the context of the accepted record [QTOS-0004](../decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md) say "QOR-MATRIX holds a **Qt Enterprise** commercial license". None of it was checked against the Qt License Certificate |
| "the owner" (personally) | demiurge-chain's ADR-083 ("The owner holds a commercial Qt licence"; "The owner's commercial licence covers the owner's builds and distribution") and `README.md` ("QQ is built with Qt under the owner's commercial licence") |
| "Demiurge" | demiurge-chain's `LICENSE` ("Copyright 2026 Demiurge"), which QOR OS ships as [`LICENSES/Apache-2.0.txt`](../../LICENSES/Apache-2.0.txt) |
| "Laustrup Group" | demiurge-chain `tools/qor-launcher/src-tauri/Cargo.toml`, `authors` |
| "Astra Matrix, Inc" | demiurge-chain `tools/qor-launcher/src-tauri/tauri.conf.json`, `bundle.copyright`: "Astra Matrix, Inc & Laustrup Group" |
| An individual (the name matches the owner's GitHub login) | demiurge-chain `services/qor-auth/Cargo.toml` (QOR ID), `authors`, with a `demiurge.io` address |

The Qt licensee may be a company or the owner personally. The assistant's
notes of what the owner said record the licence but not the licensee. Until
the owner answers, every file here writes **[Legal entity]**.

**Questions for the owner** (and an adviser, if no entity exists yet):

1. For each role above: which entity, with its full registered name, country,
   registration number and address?
2. If there is no entity yet: should one be formed before the gates, where,
   and of what kind?
3. Must the Qt licence or the copyright move to it? Ask The Qt Company how a
   licence transfers ([QT.md](QT.md)). Ask a lawyer about the copyright. Most
   commits in both repositories carry a `Co-Authored-By` line naming an AI
   assistant (counted on 27 October 2026; demiurge-chain's ADR-122, accepted
   by the owner the same day, notes the same); ask counsel how that bears on
   ownership.
4. Afterwards, one records change makes the names in `LICENSE` files and
   metadata agree. That waits for the answer.

## The gates

### 1. The Qt Company's written confirmation

- **State on 27 October 2026.** No letter has been sent. The repository calls
  the licence "Qt Enterprise", which is not one of Qt's current product names.
  The product, agreement version, Deployment Platforms and licensee are not
  recorded. Qt is already distributed in public: Demiurge's Windows launcher
  pre-releases bundle QOR Media with Qt's libraries, and from
  `launcher-v0.1.14` ARQADE too (ADR-083 left this "for the owner to confirm
  with Qt").
- **Prepared.** [QT.md](QT.md): what QOR OS uses, Qt's published terms quoted
  with dates, the open question, the contact route and the complete letter.
- **Who must act.** The owner, as or for the licensee. The Qt Company answers.
- **Next step.** Open the License Certificate at
  [account.qt.io](https://account.qt.io/login). Fill the letter's placeholders
  from it. Before sending, have counsel (or, if none is engaged yet, the
  owner, deliberately) read the letter: it describes builds already
  distributed (question 13a) and how development is done (question 15). Send
  it through the Qt Account portal or to the account manager, with a copy to
  `license-management@qt.io`.
- **Blocked on.** The entity question, and the owner reading the certificate.

### 2. Trademark clearance for "QOR OS", QQ and ARQADE

- **State on 27 October 2026.** A preliminary knockout search of the US
  register was made that day. It found live marks for review: Tencent's QQ
  (Reg. 4578256), Arqiva's ARQADE (Reg. 7781722), and COREOS (Reg. 4792179),
  which the research rated as sounding like "QOR OS". COREOS's owner of record
  (TSDR, 27 October 2026) is Red Hat, LLC (Delaware), after assignment updates
  of 9 and 16 September 2026; the registrant was CoreOS, Inc. No attorney has
  been engaged. No other register was searched systematically.
- **Prepared.** [TRADEMARKS.md](TRADEMARKS.md): a knockout brief for an
  attorney, per mark, with numbers, sources, the research's preliminary
  ratings, alternatives and questions.
- **Who must act.** The owner engages a trademark attorney. The attorney
  searches and advises.
- **Next step.** Engage an attorney and send TRADEMARKS.md. Ask first whether
  public use of QQ and ARQADE should pause.
- **Blocked on.** The applicant (the entity question), and the owner's choice
  of attorney and budget.

### 3. Terms of Service, a privacy policy and the legal opinion

- **State on 27 October 2026.** Neither document exists for any live
  service. QOR ID, Social and ARQADE are live and hold emails, messages and
  sessions. No route lets a person delete or export their own account. No
  legal opinion exists on tips, rewards, prizes, the welcome grant, backing,
  rental, identity checks, sanctions or age. Demiurge's U-15 to U-18 are
  open.
- **Prepared.** [LEGAL.md](LEGAL.md): the data inventory, subprocessors and
  questions for counsel; a factual brief of each mechanic with its record;
  the questions matrix; a draft public statement for counsel to approve.
- **Who must act.** The owner, with counsel: privacy and online terms;
  gaming and promotions; payments and virtual currency; sanctions; minors.
  Engineering builds the delete and export routes once the policy says what
  they must do.
- **Next step.** Engage counsel, send LEGAL.md, and name the operator and its
  jurisdiction.
- **Blocked on.** The entity question and counsel. Parts of the opinion also
  wait on the owner's economic decisions (U-15 to U-18, ADR-116).

### 4. Code-signing certificates and the Secure Boot key

- **State on 27 October 2026.** No code-signing certificate exists.
  Demiurge's launcher pre-releases are unsigned. Its update key is self-made
  (ADR-111). The Store's keys are self-made, in `~/.qor-os/store-signing/`,
  with a dated backup on the same disk. There is no Secure Boot key, and the
  image is not built yet (Phase 3).
- **Prepared.** [SIGNING.md](SIGNING.md): what exists, the options compared,
  the documents validation asks for, a key-custody runbook outline, and where
  signing joins the release scripts and CI.
- **Who must act.** The owner: validation, purchase and holding the keys.
  Vendors issue the credentials.
- **Next step.** Choose the Windows route and the Secure Boot approach. Move
  the Store keys' backup off this PC now.
- **Blocked on.** The entity question (organisation validation). The Secure
  Boot key also waits on Phase 3's image.

### 5. An external security review of the QOR core and the vault service

- **State on 27 October 2026.** No review has been commissioned. Demiurge's
  `SECURITY.md` opens: "Demiurge has not been audited." Its `docs/GATES.toml`
  criteria `beta.external-review` and `public-release.review-closed` are
  unmet. QOR OS's vault service is not built (Phase 2).
- **Prepared.** [SECURITY-REVIEW.md](SECURITY-REVIEW.md): an RFP with
  components pinned to commits, a threat model outline, the known-issues
  annex, report rules mapped to the gates, a retest clause, criteria for a
  firm, and the groundwork to do first.
- **Who must act.** The owner commissions and pays. A firm reviews.
  Engineering fixes. The owner accepts anything left open, in writing.
- **Next step.** The owner decides one review or two (the parts that exist
  now; the vault service after Phase 2) and shortlists firms; engineering
  does the groundwork in SECURITY-REVIEW.md (items 2 to 7) in
  demiurge-chain's sessions.
- **Blocked on.** Budget, a stable scope, and the vault service.

### 6. QOR ID in a second region with offline login; the 5 October key replaced

- **State on 27 October 2026.** QOR ID, Social and the devnet run on
  Railway, project `demiurge`, region `iad`. No record covers a second
  region. Offline login is not built. A recovery phrase pasted into a chat on
  5 October 2026 is treated as exposed, and no record says which account it
  controls or that it was replaced.
- **Prepared.** [SIGN-IN.md](SIGN-IN.md): the current state, a key-replacement
  runbook with no key material, what a QOR ID redundancy record must decide,
  and the design questions for offline login.
- **Who must act.** The owner alone replaces the key. The owner decides
  hosting and budget. Engineering drafts the records and builds offline login
  in Phase 2.
- **Next step.** The owner runs the runbook's first step: find, in the wallet,
  which public address the phrase controls. Nothing else blocks it.
- **Blocked on.** The key: only the owner. Redundancy: the owner's hosting
  decision. Offline login: Phase 2's vault service.

## Already public

These are public today. Several gates touch them.

- **demiurge-chain** is public under Apache-2.0 (the repository's licence; at
  `e99c7ce` six active crates, the core, the launcher, QOR ID and Social among
  them, still declare MIT in their manifests; ADR-122, accepted by the owner on
  27 October 2026, moves them to Apache-2.0 when pull request #109 merges),
  with QQ, ARQADE and QOR Media in it, and names QQ and ARQADE throughout.
- **Demiurge's Windows launcher**: tester pre-releases `launcher-v0.1.10` (17
  October 2026) to `launcher-v0.1.17` (25 October 2026) on GitHub (there is
  no 0.1.13), all unsigned. Every one bundles QOR Media, with Qt Multimedia
  and its FFmpeg backend (from `launcher-v0.1.17`, also Qt PDF and PDFium).
  From `launcher-v0.1.14` (18 October 2026) they also bundle ARQADE, which
  links Qt Quick 3D (and from `launcher-v0.1.17` carries QQ's runtime). Both
  are deployed with `windeployqt` and Qt's Windows libraries
  (`tools/qor-media/build.ps1`, `products/arqade/qt/build.ps1`,
  `tools/qor-launcher/scripts/stage-arqade.mjs`; each tag's
  `tauri.bundle.conf.json` and `CMakeLists.txt`). The installer's resources
  are `licenses/libgit2-COPYING.txt` and those folders. Whether the
  installers carry Qt's and FFmpeg's notices was not checked.
- **Live services without terms or a privacy policy:** QOR ID at
  `https://id.qorsync.dev` (since 1 October 2026), the devnet's RPC node at
  `wss://rpc.qorsync.dev` (since 3 October), Social (since 16 October),
  ARQADE's website, and the homepage at `qorsync.dev` (since 18 October).
- **QOR OS** is private. Nothing of it is public.

## Suggested order of work

Names and licences shape everything else, so they come first.

1. **Now, and blocked by nothing: the 5 October key.** Only the owner can do
   it, and it needs no adviser ([SIGN-IN.md](SIGN-IN.md#the-5-october-key)).
   Moving the Store keys' backup offline is the same kind of task.
2. **The entity.** Answer the question above.
3. **Then, in parallel:**
   - **a trademark attorney**, because the product names appear in the letter
     to Qt, the terms, the certificates and the store listings;
   - **the letter to Qt**, because its answer decides whether the image is an
     Application or a Device, what that costs, and what the SDK may contain.
4. **Counsel for the terms, the privacy policy and the opinion.** This can
   start as soon as the operator is named; it does not need the names cleared.
5. **Code signing.** After the entity, because validation is of the entity,
   and ideally after the names, because the publisher name is shown to users.
6. **The security review.** Groundwork now; commission when the scope is
   stable. QOR OS's part needs the vault service (Phase 2).
7. **QOR ID's redundancy record** in demiurge-chain whenever the owner wants
   it drafted; **offline login** in Phase 2.

## Keeping the evidence

- Keep the documents themselves (Qt's letter, the attorney's opinion,
  counsel's opinion, the review report, certificates) with the owner's
  agreement records, outside this repository. Never commit a private key, a
  token's PIN, a recovery phrase or a credential.
- When a gate holds, tick it in [ROADMAP.md](../ROADMAP.md#release-gates) with
  the date and what was obtained, and say so in `CHANGELOG.md` and `OWNER.md`.
  Cite no confidential terms.
- If an answer changes a decision (a name, the Qt product, the image's
  contents), a new QOR record says so.
