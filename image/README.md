# image — the QOR OS image (Phase 3)

> **Status:** planned. The image is built on **Fedora Atomic with bootc**
> ([QOR-0004](../docs/decisions/QOR-0004-the-base-image-is-fedora-atomic.md)).
> Nothing here builds yet. The first image is Phase 3 work
> ([ROADMAP](../docs/ROADMAP.md#phase-3--boots-and-plays-pc-games)).
>
> QTOS's Yocto skeleton (`meta-qtos`, `kas`) was retired by QOR-0004. It is in
> the history before commit "Retire the Yocto skeleton for Fedora Atomic"
> (`git log -- image/meta-qtos`).

## What the image is

An OCI container image built `FROM` a Fedora bootc base and booted as a whole
operating system. It contains:

| Layer | Contents |
|-------|----------|
| Base | Fedora Atomic: kernel, systemd, PipeWire, NetworkManager, BlueZ, UPower, fwupd, Mesa, libinput |
| Graphics | NVIDIA's open kernel modules for every RTX card, built for the image's kernel and signed with the QOR OS Secure Boot key; Mesa for Intel and AMD |
| Games | Steam and its 32-bit runtime, gamescope |
| QOR OS | Prism and the shell, QTOS.Kit and Lumen, the service daemons, QQ, ARQADE, QOR Media, the vault service from the QOR core, the `qtos` platform theme and Lumen QStyle |
| Session | A systemd unit that starts Prism on the first virtual terminal through a greeter (below) |

Updates are whole images: `bootc upgrade` stages the next one, and a boot that
fails returns to the last good image. Apps update separately.

## Phase 3 checklist

- [ ] `image/Containerfile` from a Fedora bootc base, with NVIDIA's open
      modules and Mesa; built and pushed by CI as an OCI image
- [ ] Secure Boot: a QOR OS key that signs the kernel modules, and enrolment
      on first boot
- [ ] The QOR OS layer installed from a CI build against the commercial Qt kit
      (Qt's distribution terms: [QOR-0008](../docs/decisions/QOR-0008-before-a-public-release.md))
- [ ] Prism on its own KMS backend as the session ([QOR-0005](../docs/decisions/QOR-0005-every-display-at-its-own-refresh.md)),
      started by greetd with a Prism greeter, flicker-free from the firmware logo
- [ ] Steam and Proton inside gamescope as a Wayland client of Prism
- [ ] Installable disk images and an installer ISO (`bootc-image-builder`)
- [ ] Boots the reference laptop with its panel at 360 Hz and a television at
      60 Hz, and rolls back from a deliberately broken update

## The session, as QTOS sketched it

The retired Yocto layer started Prism directly on tty1. The image keeps the
same environment until Prism's own backend replaces Qt's KMS integration:

```ini
[Service]
User=qtos
PAMName=login
TTYPath=/dev/tty1
Environment=QT_QPA_PLATFORM=eglfs
Environment=QT_QPA_EGLFS_INTEGRATION=eglfs_kms
Environment=QT_QPA_EGLFS_KMS_ATOMIC=1
Environment=QT_QPA_PLATFORMTHEME=qtos
Environment=XDG_SESSION_TYPE=wayland
ExecStart=/usr/bin/prism
```
