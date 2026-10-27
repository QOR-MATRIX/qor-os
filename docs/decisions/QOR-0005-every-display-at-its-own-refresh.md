# QOR-0005: Every display at its own refresh rate, on every GPU; the bare-metal gate is deferred

- **Status:** Accepted
- **Date:** 2026-10-26
- **Deciders:** the owner
- **Relates to:** [QTOS-0003](qtos/QTOS-0003-compositor-strategy.md) (amended), QOR-0004

## Context

QTOS-0003 built Prism on Qt Wayland Compositor and held a decision gate. A
run on bare metal (DRM/KMS) with two outputs at different refresh rates would
confirm that choice, or else Prism would move to Waylib. The tooling was
finished on 26 October 2026: `tools/dev/prism-gate.sh` and a bundle that runs
from an Ubuntu 26.04.1 live USB session
([PERFORMANCE.md](../PERFORMANCE.md#compositor-decision-gate)).

The owner ran it that day on the reference laptop, a Dell G15 5530 with an
RTX 4060 and Intel UHD graphics, driving its 1920×1080 360 Hz panel and a
1080p 60 Hz television:

- Prism started on Qt's KMS integration and drew two outputs, but both
  appeared on the television.
- The runs did not pass.
- Their results were lost, because GitHub refuses `.tgz` attachments. The
  bundle now packs `.zip` (`gate-bundle-2`).

The owner judged the live-USB procedure too costly to repeat and set the
requirement that matters instead: high-resolution displays at 360 Hz and at
60 Hz, with drivers for every RTX card and for Intel.

Two facts shape the path from here:

- Qt's `eglfs_kms` drives one DRM device. The official kit is built without
  udev, so it takes the first card unless told otherwise.
- Gaming laptops often split their displays across GPUs. On the G15 the panel
  is driven by the Intel GPU, and the external ports may be wired to the
  NVIDIA one.

## Decision

1. **The gate is deferred, not failed.** QTOS-0003's Option A stands: Qt
   Wayland Compositor for the scene and the protocols. QTOS's ADR-0011 is not
   written; this record replaces it.
2. **The owner's requirement is the acceptance test for displays.** Each
   connected display runs at its own native mode and refresh rate. 60 Hz and
   360 Hz are tested together, with VRR where the display supports it. This
   holds on:
   - NVIDIA RTX, on the open kernel modules;
   - Intel and AMD, on Mesa;
   - hybrid laptops whose displays sit on different GPUs.
3. **Prism gets its own KMS backend (Phase 3).** It covers:
   - atomic modesetting and frame scheduling per output;
   - several GPUs, rendering on one and scanning out on another (dmabuf and
     PRIME);
   - VRR, and hotplug;
   - direct scan-out of a full-screen client.

   Qt's `eglfs_kms` stays a development path only.
4. **Both outputs on one display is a defect, tracked to that backend.** It
   is fixed there and tested on the reference laptop with the television.
5. **The gate kit becomes a diagnostic.** `prism-gate.sh` and its bundle stay
   for probing a machine. The backend's acceptance runs come from an
   installed QOR OS image (QOR-0004), not a live USB.

## Consequences

- Phase 3's exit includes the acceptance test above on the reference laptop:
  its panel at 360 Hz and the television at 60 Hz, at once.
- The pass criteria QTOS proposed carry over as the backend's targets:
  - under 1% missed frames on each output at its own refresh rate;
  - input to swap within two refresh periods at p95.
- Running Steam games uses gamescope as a Wayland client of Prism, which
  brings XWayland, frame limiting and upscaling. Prism's own XWayland support
  follows.
- If the backend cannot meet the targets on hardware QOR OS supports, a later
  record revisits Option B (Waylib) for the backend only, keeping the shell.
