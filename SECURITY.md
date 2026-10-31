# Security policy

## Reporting a vulnerability

Please **do not** open a public issue for security problems.

Report privately through GitHub's
[security advisory form](https://github.com/QOR-MATRIX/qor-os/security/advisories/new).
Include:

- the affected component: the compositor, a service, an app, QQ, ARQADE, QOR
  Media, the QOR core and vault, the image or updates;
- the commit SHA or version;
- steps to reproduce, or a proof of concept;
- the impact you expect: for example privilege escalation, sandbox escape,
  a key or token exposed, or a spoofed trusted-UI prompt.

Please do not test against anyone else's device or account.

A problem in the QOR core, or in the network services (the chain, QOR ID,
Social, ARQADE's website), belongs to demiurge-chain, where that code lives. Report it as
[its SECURITY.md](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/SECURITY.md)
says, or here, and we will move it.

You will receive an acknowledgement within **3 business days**. We aim to
deliver a fix or mitigation within **30 days** for high-severity issues and
will coordinate disclosure with you.

## Supported versions

QOR OS is pre-release. Only the latest commit on `main` receives security
fixes.

| Version | Supported |
|---------|-----------|
| `main`  | Yes |
| older   | No |

## Security model

The intended device security model is described in
[`VISION.md` §3](VISION.md#3-foundation-image-boot-updates-security). In
summary:

- a signed boot chain, and whole-image updates with rollback;
- sandboxed games and apps that reach the system only through portals and
  service contracts;
- a vault service, built from the QOR core, that keeps keys outside the
  compositor and every app, and never answers with a key;
- system prompts and key approvals drawn by Prism in a trusted layer that
  clients can never cover.

Secrets never reach a log, test output, screenshot or commit
([`AGENTS.md`](AGENTS.md) §6).

The **simulator** (`qor-shell`, `qor-simd`, and Prism's embedded services) is a
development tool. It exposes simulation controls (`SimControl`) on a local socket
with no authentication, its lock screen takes any password, and it decodes media
inside the shell. A device's image is built without it
(`-DQOR_BUILD_SIMULATOR=OFF`), which removes it from Prism altogether.

## What is checked by machines

[QOR-0018](docs/decisions/QOR-0018-the-security-and-quality-bar.md): exploit
mitigations on every program, read back in CI; the suites under AddressSanitizer
and UBSan; everything CI runs pinned by content; secrets looked for at the door
and through the whole history; the QOR core's dependencies audited weekly.

## What a game or an app can and cannot do

- **Prism treats every Wayland client as hostile.** Its name is a claim: a client
  calling itself Settings or ARQADE is kept apart from them. It takes the keyboard
  only when it can be seen and it steals it from nobody, and never on the lock
  screen. Its title is text, never markup.
- **The QQ Player keeps a game to its package** by two walls: QQ's own checks
  (`products/qq/runtime/confinement.h`), and on Linux the kernel's
  (`products/qq/player/sandbox.h`): Landlock for files and programs, seccomp for
  the network.
- **QQ Studio runs a project's code with the person's own reach**, since making a
  game means running it. So a project made elsewhere runs nothing until they say
  they trust it, nothing in it can bring in native code, and logic an AI agent
  wrote runs only once the creator allows it.

## Known limits

Say so if you find a way past one; these are the walls we know are low.

- **On Windows the Player has only QQ's own checks.** Windows is a development
  host, not the device. Qt Quick's `Canvas` (reads any picture) and `grabToImage`
  (writes over any picture) and Qt Multimedia's camera, microphone, screen capture
  and recorder types cannot be refused from inside the process. A stranger's game
  plays only in the browser's sandbox (Demiurge's ADR-087).
- **The bus trusts anyone who reaches it**, which is the person's own processes.
  Until games and apps are sandboxed (Phase 3), a process running as the person can
  crash whatever hosts the bus: Prism, with the simulator. Qt Remote Objects'
  decoder trusts what it is sent. So: nothing sandboxed is ever given the bus.
- **Prism's clipboard is Qt's**: any client may set the selection. Reading it takes
  the keyboard, which a client no longer gets unseen.
- **The Store's catalogue has no expiry or sequence yet.** A replayed older one can
  hide updates, though it cannot downgrade what is installed. It is reachable only
  through `QOR_STORE_URL` until there is a server (Store stage 2).
- **A same-user process can rewrite a widget** after it is checked and before Qt
  reads it, in a small window. A read-only image closes that (Phase 3).

Before anything holds value, the QOR core and the vault service get an
external security review, and the key Demiurge's records list as exposed on
5 October 2026 is replaced
([QOR-0008](docs/decisions/QOR-0008-before-a-public-release.md)).
