# Security policy

## Reporting a vulnerability

Please **do not** open a public issue for security problems.

Report privately through GitHub's
[security advisory form](https://github.com/QOR-MATRIX/QTOS/security/advisories/new).
Include:

- the affected component (compositor, a service, an app, the image or updates)
- the commit SHA or version
- steps to reproduce, or a proof of concept
- the impact you expect (for example, privilege escalation, sandbox escape, or
  a spoofed trusted-UI prompt)

You will receive an acknowledgement within **3 business days**. We aim to
deliver a fix or mitigation within **30 days** for high-severity issues and
will coordinate disclosure with you.

## Supported versions

QTOS is pre-release. Only the latest commit on `main` receives security fixes.

| Version | Supported |
|---------|-----------|
| `main`  | Yes |
| older   | No |

## Security model

The intended device security model is described in
[`VISION.md` §3](VISION.md#3-foundation-image-boot-updates-security). In
summary: signed boot chain, read-only verified root, A/B updates with
rollback, sandboxed apps that reach the system only through portals and
service contracts, and system prompts drawn by Prism in a trusted layer that
clients can never cover.

The **simulator** (`qtos-shell`, `qtos-simd`) is a development tool. It
exposes simulation controls (`SimControl`) on a local socket with no
authentication and must never be shipped on a device.
