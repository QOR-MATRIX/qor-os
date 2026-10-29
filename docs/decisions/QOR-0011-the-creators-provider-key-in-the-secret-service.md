# QOR-0011: The creator's provider key on Linux is kept in the Secret Service

- **Status:** Accepted
- **Date:** 2026-10-28
- **Deciders:** the owner
- **Relates to:** `AGENTS.md` §6 (keys and secrets); Demiurge ADR-083
  decision 4 (the provider's key in the operating system's keychain) and
  ADR-086 (generation is paid for by whoever uses it);
  [QOR-0003](QOR-0003-the-qor-core-at-a-pinned-commit.md) (the QOR core)

## Context

QQ Studio's agent works through an outside provider (Anthropic) with the
creator's own API key. Demiurge ADR-086 makes the creator pay for their own
generation, so Demiurge holds no provider key. ADR-083 decision 4 keeps the
creator's key in the operating system's keychain. On Windows that is the
Credential Manager (`products/qq/agent/keychain.cpp`). Elsewhere the code had
only a stub that stored nothing. On Linux five of QQ's agent cases failed, and
the agent could not be used on QOR OS.

`AGENTS.md` §6 says code that handles keys lives only in the QOR core. That
rule is about QOR's own secrets: the vault's identity and signing keys,
recovery phrases, what the core keeps and approves. A creator's key for
someone else's service is none of those. It signs nothing in QOR, moves no
money in QOR, and belongs to the creator's account with the provider.

Alternatives considered:

- **The QOR core's vault.** That would put a third party's credential among
  QOR's keys, change the core upstream for something that is not QOR's, and
  make the vault answer with a key, which §6 forbids.
- **libsecret**, the usual C library for the Secret Service. The Qt kit
  does not have it and the build machines do not either. Qt D-Bus, in every
  kit, speaks the same protocol.
- **No key on Linux.** The agent would not work on QOR OS.

## Decision

1. **A creator's own key for an outside provider is not a QOR key.** It is
   kept in the platform's keychain, by the product that uses it, as Demiurge
   ADR-083 decision 4 already does on Windows. `AGENTS.md` §6 says so.
2. **On Linux that keychain is the Secret Service** (freedesktop.org's
   specification, served by GNOME Keyring, KWallet and others). QQ talks to it
   over D-Bus with Qt D-Bus:
   - the key is an item in the default collection, found by its attributes
     (`application`, `target`);
   - the keyring's own prompt unlocks it when it is locked;
   - it crosses the session bus in a plain session.
3. Everything else §6 says still holds. The key is never written to a file, a
   setting or a log, and is held in memory only while a request is made. The
   suites never reach a creator's keyring: on Linux they run with a session
   bus and a keyring of their own, `tools/dev/secret-service.py`.

## Consequences

- QQ Studio's agent works on Linux wherever a Secret Service runs. Where none
  does, keeping the key says so, as the stub did.
- **The QOR OS image must ship a Secret Service provider**, unlocked at
  sign-in, which Phase 3's image work decides. Until then QOR OS's own CI
  tests QQ only against the run's keyring, not a real one.
- A plain session sends the secret over the session bus unencrypted. The bus
  is the user's own, and any program of the same user can already ask the
  keyring for it. An encrypted session (`dh-ietf1024-sha256-aes128-cbc-pkcs7`)
  can be added later without changing this decision.
- This would be wrong if QOR OS moved the creator's provider keys into a
  service of its own, for example to give the agent a key the creator never
  sees. A later record would supersede this one, and §6 would change with it.
