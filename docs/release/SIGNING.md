# Code signing and the Secure Boot key

*Prepared on 27 October 2026 by an assistant, from the research listed under
[Sources](#sources), for the owner and their advisers. It is research, not
legal advice.*

**The gate** ([QOR-0008](../decisions/QOR-0008-before-a-public-release.md)):
"code-signing and Secure Boot keys exist (QOR-0004)".
[QOR-0004](../decisions/QOR-0004-the-base-image-is-fedora-atomic.md): "Secure
Boot needs QOR OS's own key: kernel modules built for the image are signed
with it, and a machine enrols it once."

> **Prices and vendor terms.** This file states none as fact. Wherever a
> vendor, price, eligibility rule or validation requirement is mentioned:
> **check the vendor's current page.** The vendor and standards pages cited
> under [Sources](#sources) were not read for this pack; what is said about
> them is background for the owner to confirm.

## What exists today

No key's content was read for this file. Only names and places are given.

| What | Kind | Where it is held | State |
|------|------|------------------|-------|
| Demiurge's Windows launcher installers, `launcher-v0.1.10` to `launcher-v0.1.17` | GitHub pre-releases | — | **Unsigned.** Demiurge's L6.1, "Signed installers for Windows, macOS and Linux", is unchecked. Its `HANDOFF.md` says signing "needs a Windows code-signing identity and an Apple Developer ID, which the owner was obtaining on 2026-09-17". No later record says either was obtained |
| The launcher's **update key** (Demiurge ADR-111) | A self-made Tauri updater key, not a CA certificate | The owner's PC, `%APPDATA%\Demiurge\updater\`, encrypted with a password Windows seals for that user | Signs every installer the update channel offers. ADR-111 decision 7: Windows and Apple code signing are not covered by it. Demiurge's `OWNER.md` ("What only you can do", item 3) asks the owner to back it up (`updater-key.ps1 -Backup`); no record says it was done |
| The **Store's keys** (QOR OS) | Self-made ECDSA P-256 keys | `~/.qor-os/store-signing/`, moved from `~/.qtos/store-signing/` on 27 October 2026 (commit `9849f57`). A dated backup beside them ([`AGENTS.md`](../../AGENTS.md) §6), the folder `~/.qor-os/store-signing-backup-2026-10-27` (its name seen on disk on 27 October 2026; the repository does not record it), is **on the same disk** | `store.pem` is the root every installation trusts (its public half is `store/keys/store.pub`). [STORE.md](../STORE.md): before the Store opens, "generate the production store key offline or in a hardware security module, sign catalogues on a machine that holds nothing else". `developer-org.qtos.pem` is no longer used |
| The Android launcher's test signing key (Demiurge ADR-104) | APK signing key | "The test signing key is made on the owner's PC and kept out of the repository." For Google Play, later: "The key for Google Play, later, is the owner's to hold" | No Android release yet |
| QOR OS's Windows build | CI artifact `qor-os-windows-x64`, on tags and manual runs, kept 7 days | — | Unsigned. Not a public release |
| QOR OS's **Secure Boot key** | — | — | **Does not exist.** The image is Phase 3 work ([image/README.md](../../image/README.md)) |
| Linux package or image signing | — | — | Nothing exists; no image is built yet |

**Do now, whatever else is decided:** move the Store keys' backup off this
PC, to offline storage the owner controls. Only the owner should move it
(QOR OS [`AGENTS.md`](../../AGENTS.md) §6).

## What will need signing

| Artefact | Platform | Signed with | Needed for |
|----------|----------|-------------|------------|
| The QOR OS image (OCI) and its installer ISO and disk images | Linux | An image-signing key; signed checksums | Phase 3; the public download |
| Kernel modules built for the image (NVIDIA's open modules) | Linux, UEFI | QOR OS's Secure Boot key | Phase 3; booting with Secure Boot on |
| The QOR OS layer's packages, if they are RPMs | Linux | A package-signing key | Phase 3, [if the layer is packaged as RPMs] |
| Store catalogues | Every QOR OS | The production store key | Before the Store opens |
| The SDK and its Windows simulator | Windows, Linux | Windows code signing; Linux signing | Phase 4 |
| Demiurge's launchers (upstream) | Windows, macOS, Android | Windows code signing; Apple Developer ID; APK key | Demiurge's L6.1 and L13 |

QOR OS has no macOS build. Apple matters only for Demiurge's launcher (L6.1)
or a later macOS SDK.

## Options to compare

### Windows code signing

Background to confirm: since 1 June 2023 the CA/Browser Forum's Code Signing
Baseline Requirements have required the private key of a publicly trusted
code-signing certificate to be generated and kept in hardware (a token or an
HSM, including a cloud HSM). [Check the current requirements.]

| Route | Where the key lives | Fits CI? | Validation | Notes to check |
|-------|---------------------|----------|------------|----------------|
| **OV certificate on a hardware token** | A USB token the CA ships or the owner supplies | Poorly: signing happens where the token is plugged in, so on the owner's PC, after CI builds | Organisation validation of [Legal entity] | Price, token cost, validity: check the vendor's current page. SmartScreen reputation builds with downloads |
| **EV certificate on a hardware token** | As above | As above | Extended validation (stricter) | Microsoft changed how SmartScreen treats EV certificates in 2024; check Microsoft's current guidance before paying for EV. Price: check the vendor's current page |
| **A CA's cloud signing service** (for example DigiCert KeyLocker, SSL.com eSigner; others exist) | The CA's HSM | Yes: CI calls the service with a credential, and the key never leaves the HSM | OV or EV, as above | Per-signature or subscription pricing, and how CI authenticates: check the vendor's current page |
| **Microsoft's managed signing service on Azure** (Trusted Signing, as last known; check its current name) | Microsoft's HSM; short-lived certificates | Yes, including from GitHub Actions through OIDC | Microsoft validates the organisation or individual | Eligibility (countries, organisation age, individuals) and price: check Microsoft's current page |

### Apple: Developer ID and notarisation

Only for macOS builds (Demiurge's launcher). Background to confirm:

- Membership of the Apple Developer Program, as an individual or an
  organisation. An organisation needs a D-U-N-S number. Fee: check Apple's
  current page.
- A **Developer ID Application** certificate (apps) and a **Developer ID
  Installer** certificate (installer packages).
- Notarisation: the hardened runtime, submission with `notarytool`, then
  stapling the ticket.

### Linux packages and images

Background to confirm against each project's documentation:

- **The OCI image.** bootc images are container images. They can be signed
  with sigstore (`cosign`), keyed or keyless. Clients enforce signatures
  through `containers-policy.json`, and bootc can be told to enforce the
  policy when switching images [check the bootc docs for the current option].
  Keyless signing records the signer's identity in a public transparency
  log; with a private repository, decide whether that is acceptable.
- **RPMs**, if the QOR OS layer ships as packages: a GPG key with `rpmsign`,
  and signed repository metadata.
- **A Flatpak remote**, if QOR OS hosts one: a GPG-signed repository.
- **Installer ISO and disk images**: publish SHA-256 checksums and sign the
  checksum file (GPG or `cosign sign-blob`).

### Secure Boot for a Fedora Atomic bootc image

Background to confirm: Fedora ships `shim` signed by Microsoft's UEFI CA, and
`shim` trusts Fedora's key, which signs Fedora's kernel. Modules QOR OS builds
itself, such as NVIDIA's open modules, are not signed by Fedora. With Secure
Boot on, a Fedora kernel refuses unsigned modules [verify on the reference
laptop].

| Option | How it works | For | Against |
|--------|--------------|-----|---------|
| **A. Fedora's shim and kernel, plus a QOR OS Machine Owner Key (MOK)** | QOR OS signs its own modules with its key. Each machine enrols the public key once at first boot (`mokutil`, then a password typed into MokManager on the next boot) | What QOR-0004 and [image/README.md](../../image/README.md) describe ("enrolment on first boot"). Universal Blue's images work this way [check their docs]. Fedora's signed boot chain is unchanged | One extra step for every user at first boot. The key signs every image build, so it must be reachable by the build |
| **B. QOR OS's own shim, signed by Microsoft** | QOR OS submits its own `shim` for review (`rhboot/shim-review`) and has Microsoft sign it; QOR OS then signs its own kernel and modules | No enrolment step for users | Heavy: a review, Microsoft's signing process (which has its own account and certificate requirements: check Microsoft's current page), and QOR OS maintaining kernel signing for every kernel |
| **C. QOR OS's own keys in the firmware** (PK, KEK, db) | The user puts the firmware in setup mode and enrols QOR OS's keys | Full control | Replaces Microsoft's keys: can stop Windows and devices whose option ROMs Microsoft signed from booting. Reasonable only on hardware QOR OS or a partner controls (preinstalled handhelds) |

Points for the record that chooses:

- Microsoft's 2011 UEFI CA certificates were due to expire in 2026, with 2023
  replacements. Check which CA signs the `shim` Fedora ships and what the
  target machines trust.
- Changing Secure Boot keys changes TPM2 PCR 7. QOR OS plans LUKS2 unlocked by
  TPM2 and a PIN ([VISION.md](../../VISION.md) §3), so the enrolment and the
  TPM sealing must be designed together.
- The kernel's `sign-file` can use a key in a PKCS#11 token or HSM rather than
  a key file [check the kernel's module-signing documentation].

A QOR record should decide A, B or C before Phase 3's image work starts.

## The documents validation asks for

Typical items; **check each CA's and Apple's current list**:

- [Legal entity]'s exact registered name, registration number, jurisdiction
  and registered address;
- a telephone number listed in a public directory, for a verification call;
- a D-U-N-S number (Apple organisation accounts; some CAs use it too);
- proof of a domain the entity controls, if asked (`qorsync.dev`: [who
  registered it]);
- for EV: evidence of legal and operational existence, a named and verified
  officer, and signed subscriber agreements;
- the applicant's government ID;
- for an individual certificate instead: the owner's government ID, and
  whatever identity check the vendor runs.

The name on the certificate is what users see as the publisher. It should
match the cleared product names and the entity in the terms.

## Key custody: a runbook outline

To become a runbook once the keys exist. **No key, password, PIN or phrase is
ever written into it.**

1. **Inventory.** One table, kept by the owner: each key's purpose, holder,
   where it lives, where its backups are, its expiry or rotation date, and
   what to do if it leaks. Rows: Windows code signing; Apple Developer ID;
   the Secure Boot MOK; image signing; package signing; the production store
   key; first-party Store developer keys; Demiurge's update key and APK key.
2. **Generation.** On an offline machine, or inside the token, HSM or cloud
   service, so the private key is never on a networked disk. Commit only the
   public half or its fingerprint.
3. **Backups.** Two copies in separate places, encrypted, with the passphrase
   stored apart from them. Restore one copy once a year to prove it works.
   Keys in non-exportable hardware have no backup: plan their replacement
   instead.
4. **Access.** Who may sign, and with whose approval. A signing log records
   what was signed, when, by whom and the artefact's SHA-256, never a secret.
5. **Use.** Sign only artefacts CI built, after checking their SHA-256 against
   CI's record. Timestamp every Windows signature, so it outlives the
   certificate.
6. **Calendar.** Renewal dates for every certificate, with reminders well
   ahead.
7. **Compromise.** Revoke (with the CA, giving the date of compromise); make a
   new key; ship the new public key (the Store's needs a QOR OS update; a new
   MOK needs re-enrolment); say publicly what happened.
8. **Continuity.** What happens if the owner is unavailable: [named person or
   arrangement].

## Where signing joins the release scripts and CI

The keys never enter the repository, a log, CI output or a screenshot (QOR OS
[`AGENTS.md`](../../AGENTS.md) §6).

**QOR OS today.** `build.yml` packages a Windows build on `v*` tags and manual
runs ("Package", then `upload-artifact` as `qor-os-windows-x64`). Signing would
be added as follows:

1. A **separate signing job** that runs only on tags, never on
   `pull_request`, in a GitHub Environment that requires the owner's approval
   before it starts.
2. With **cloud signing**: the job authenticates by OIDC (no stored
   long-lived secret where the service supports it), signs, verifies
   (`signtool verify /pa` on Windows), and uploads the signed artefact.
3. With a **hardware token**: CI uploads the unsigned artefact and its
   SHA-256; the owner runs a script on the PC with the token
   ([`tools/release/`, to be written]) that downloads it, checks the hash,
   signs with a timestamp, verifies, and uploads the signed copy. The token is
   never plugged into a CI runner.
4. **Self-hosted runners** never hold a signing key. The signing job, if it
   runs on one, uses a runner label kept for it.

**Phase 3, the image.**

- The image build signs NVIDIA's modules with the MOK during the build. The
  key comes in as a build secret (for example `podman build --secret`), so it
  never lands in an image layer, or better from an HSM through PKCS#11.
- After pushing, the job signs the image by digest and verifies the
  signature.
- Installer ISOs and disk images get signed checksums.

**The Store.** `tools/store/build_store.py` signs on the owner's machine with
the keys in `~/.qor-os/store-signing/`. For the public Store, catalogues are
signed on an offline machine with the production key; only the signed
catalogue (public data) is committed.

**Upstream.** Demiurge's `tools/qor-launcher/scripts/release.mjs` (L6.1) gains a
signing step under demiurge-chain's own rules. Tauri can call a custom
signing command on Windows [check Tauri's current bundler docs].

## Decisions for the owner

1. Which legal entity is the signing subscriber (the README's question).
2. The Windows route: token or cloud, OV or EV.
3. Whether an Apple Developer account is needed now (only for Demiurge's
   macOS launcher).
4. Secure Boot: option A, B or C, recorded in a QOR record before Phase 3.
5. Where the offline keys and backups live.
6. When to move the Store keys' backup off this PC (now is suggested).

## Sources

| What | Where |
|------|-------|
| The gate; Secure Boot's own key | [QOR-0008](../decisions/QOR-0008-before-a-public-release.md); [QOR-0004](../decisions/QOR-0004-the-base-image-is-fedora-atomic.md); [image/README.md](../../image/README.md); [VISION.md](../../VISION.md) §3 |
| Store keys and the production key plan | [STORE.md](../STORE.md) ("Trust" and the signing-keys paragraph); QOR OS commit `9849f57`; [OWNER.md](../../OWNER.md) |
| QOR OS's CI packaging step | [`.github/workflows/build.yml`](../../.github/workflows/build.yml); [CI.md](../CI.md) |
| Unsigned launcher releases; L6.1; the update key; the APK key | demiurge-chain origin/main `e99c7ce`: `docs/DIRECTION.md` (L6), `HANDOFF.md` §4.0 ("L6, distribution"), `SECURITY.md`, `OWNER.md` ("What only you can do", item 3), `docs/decisions/ADR-111-the-launchers-update-channel.md`, `docs/decisions/ADR-104-the-launcher-on-android-on-qt.md`; `gh release list -R QOR-MATRIX/demiurge-chain` (27 October 2026) |
| Background, not read for this pack (confirm) | CA/Browser Forum Code Signing documents, <https://cabforum.org/working-groups/code-signing/documents/>; Microsoft's signing service, <https://learn.microsoft.com/azure/trusted-signing/>; Apple Developer ID, <https://developer.apple.com/developer-id/>; Apple notarisation, <https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution>; shim review, <https://github.com/rhboot/shim-review>; bootc, <https://bootc-dev.github.io/bootc/>; sigstore cosign, <https://docs.sigstore.dev/>; kernel module signing, <https://www.kernel.org/doc/html/latest/admin-guide/module-signing.html> |
