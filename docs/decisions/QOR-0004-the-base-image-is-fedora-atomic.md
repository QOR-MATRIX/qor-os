# QOR-0004: The base image is Fedora Atomic

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** [QTOS-0002](qtos/QTOS-0002-linux-kernel-and-the-qt-line.md) (extended); replaces the Yocto plan in QTOS's
  VISION §3 and `image/`

## Context

QTOS planned its image on the Yocto Project:

- `meta-qt6` and its own `meta-qtos` layer;
- RAUC A/B updates;
- a read-only root.

That gives full control and a complete SBOM. For a gaming operating system it
also makes the hardest parts the slowest:

- **NVIDIA drivers.** The owner's requirement is every RTX card. NVIDIA's
  open kernel modules support every RTX generation (Turing onward), but they
  are built outside the kernel tree and must keep pace with both kernel and
  driver releases.
- **Steam.** The Steam client and Proton need 32-bit libraries and a large,
  fast-moving set of runtime packages.
- **Mesa.** New GPUs need current Mesa and firmware quickly.

Gaming distributions solve this on a maintained base. Bazzite builds on
Fedora Atomic, through Universal Blue's NVIDIA images, and SteamOS builds on
Arch. The options:

| Option | For | Against |
|--------|-----|---------|
| A. Yocto (QTOS's plan) | Full control, SBOM, reproducible | QOR OS would own NVIDIA, Mesa, firmware and Steam's runtime alone |
| B. Fedora Atomic, built with bootc | Image-based updates with rollback, current kernel and Mesa, proven NVIDIA images, Flatpak | Fedora's release cadence; a smaller set of supported architectures |
| C. Arch, as SteamOS does | Newest packages | Rolling, harder to reproduce, no image tooling of its own |
| D. Ubuntu or Debian | Familiar | Older Mesa; image-based updates are not the default |

## Decision

1. **QOR OS images are built on Fedora Atomic with bootc**, as OCI container
   images. A QOR OS image is a `Containerfile` from a Fedora bootc base, plus:
   - **NVIDIA's open kernel modules** for every RTX card, built and signed for
     each kernel in the image;
   - **Mesa** for Intel and AMD;
   - the QOR OS layer: Prism, the Kit, the services, the products and the QOR
     core, built against the commercial Qt kit and installed into the image.
2. **Updates are whole images with rollback.** `bootc upgrade` stages the next
   image, and a failed boot returns to the last good one. Apps update
   separately (VISION §8).
3. **Everything above the Qt Line stays QOR OS's own.** QTOS-0002 stands: a
   Linux kernel and headless substrate underneath, and everything a person
   sees drawn by Qt.
4. **The Yocto skeleton is retired.** `image/meta-qtos` and `image/kas` leave
   the tree; they remain in history. `image/` holds the Fedora Atomic image
   (Phase 3).

## Consequences

- Secure Boot needs QOR OS's own key: kernel modules built for the image are
  signed with it, and a machine enrols it once.
- Steam runs from the image or as a Flatpak, with its 32-bit runtime, inside
  gamescope (QOR-0005, Phase 3).
- The SBOM comes from the image's package database, rather than from a build
  QOR OS controls line by line.
- Fedora's twice-yearly release becomes part of QOR OS's release rhythm.
- The decision is wrong if the image cannot meet QTOS's boot and login budgets
  (VISION §12) or keep its graphics stack current. Either is measured on the
  reference laptop in Phase 3.
