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

The **simulator** (`qor-shell`, `qor-simd`) is a development tool. It
exposes simulation controls (`SimControl`) on a local socket with no
authentication and must never be shipped on a device.

Before anything holds value, the QOR core and the vault service get an
external security review, and the key Demiurge's records list as exposed on
5 October 2026 is replaced
([QOR-0008](docs/decisions/QOR-0008-before-a-public-release.md)).
