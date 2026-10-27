# Roadmap

The only roadmap for QOR OS. Each phase ends at its **exit**, shown working,
not on a date; durations are rough guides for one founder working with agents.
What 1.0 contains is [QOR-0007](decisions/QOR-0007-what-1-0-ships.md). The
phases are tracked as GitHub milestones.

QOR OS began on 26 October 2026 from QTOS's Phase 0 and most of its Phase 1
([history below](#history-qor-phases)) and from Demiurge's device-side
products ([UPSTREAM.md](UPSTREAM.md)).

## Phase 0 · Clear the ground

**Complete, 27 October 2026.** *A few days.* Exit: `qor-os` builds and tests in CI on Windows and Linux, the carried
products included on Windows.

- [x] Disk space for the work: 204.7 GB free on the build drive (was 9.6 GB), by cleaning Demiurge's Rust build output
- [x] `QOR-MATRIX/qor-os`, private, seeded with QTOS's full history; QTOS frozen at `37b621e`, with a note pointing here (`3ecdded`) ([QOR-0002](decisions/QOR-0002-one-repository-from-qtos-and-demiurge.md))
- [x] QQ, ARQADE's app, games and fixtures, and QOR Media carried from demiurge-chain with their history, at upstream paths; `tools/upstream/demiurge.sh` pulls and patches ([UPSTREAM.md](UPSTREAM.md))
- [x] The QOR core built from demiurge-chain at a pinned commit and tested through its C bridge (`core/`, `tst_qorcore`; [QOR-0003](decisions/QOR-0003-the-qor-core-at-a-pinned-commit.md))
- [x] One rulebook ([AGENTS.md](../AGENTS.md)) and one decision log, QOR-0001 to QOR-0009 ([decisions/](decisions/README.md))
- [x] The carried products build and pass their suites here (`tools/dev/products.ps1`)
- [x] One name: QOR OS in the interface, the documents and the code, QTOS only for the predecessor (27 October, [QOR-0009](decisions/QOR-0009-the-name-is-qor-os.md))
- [x] The CI runners serve both QTOS and QOR OS from the organisation, since 27 October ([CI.md](CI.md#runners-for-the-organisation))
- [x] CI green on Windows (with the products) and Linux: the first run on 26 October passed every job, the products on their first attempt
- [x] Demiurge's L13.2 merged upstream (#107, `e99c7ce`, 27 October: QOR ID's client, the chain client, Social's client, the Library's index, the Mesh and Qontrol in the core), and the core's pin moved to it with the `desktop` feature ([UPSTREAM.md](UPSTREAM.md#moves-so-far))

## Phase 1 · One stack

*About 2–4 weeks.* Exit: Orb Run plays inside Prism on Linux with a DualSense, and Linux CI runs
QQ's, ARQADE's and QOR Media's suites.

- [ ] QQ, ARQADE and QOR Media build and pass their suites on Linux. Their Windows-only parts get platform seams:
  - XInput becomes QOR.Input;
  - the DXGI GPU choice is handled per platform;
  - the agent's key moves from Credential Manager to the Secret Service;
  - window docking moves out.
- [ ] Each runs as a Wayland client of Prism, and Prism places it (the launcher's `place` request becomes a Prism layout request)
- [ ] The controller service drives QQ through `Input`'s gamepad seam; ARQADE and QOR Media gain controller focus navigation
- [ ] Lumen's tokens theme every app: Demiurge's one look (`docs/design/tokens.json`) mapped onto Lumen roles, with one source
- [ ] A controller-first **Home** and **Library** in the shell: QQ games, ARQADE, media
- [ ] From QTOS's Phase 1: evaluate Qt Interface Framework for service code generation and simulation (a QOR record)
- [ ] From QTOS's Phase 1: a Qt Design Studio project for Lumen components

## Phase 2 · Identity and keys

*About 3–6 weeks.* Exit: sign in once, chat in World, and publish Orb Run on the devnet from QQ
Studio running inside Prism.

- [ ] The vault as a system service built from the QOR core, outside Prism and every app; Prism draws its approvals in the trusted layer
- [ ] The core's bridge grows (upstream, with tests) from four commands to what the shell needs, with events
- [ ] One QOR ID sign-in at the shell, with offline login
- [ ] Social in the shell: chat, friends and World
- [ ] The Library's index as a system service (the role QTOS planned for its indexing daemon, now `qor-indexd`)
- [ ] Publishing from QQ Studio through the vault service, to the devnet, in test CGT only ([QOR-0006](decisions/QOR-0006-qor-os-1-0-moves-no-money.md))

## Phase 3 · Boots and plays PC games

*About 2–4 months.* Exit: the reference laptop boots QOR OS with its panel at 360 Hz and the
television at 60 Hz, and plays a Steam game and Orb Run from the
controller-first Home.

- [ ] The image on Fedora Atomic with bootc: NVIDIA's open kernel modules for every RTX card, Mesa, the QOR OS layer ([QOR-0004](decisions/QOR-0004-the-base-image-is-fedora-atomic.md), [image/](../image/README.md))
- [ ] Secure Boot with QOR OS's own key; image updates with rollback (`bootc upgrade`, a forced rollback tested)
- [ ] **Prism's own KMS backend** ([QOR-0005](decisions/QOR-0005-every-display-at-its-own-refresh.md)):
  - atomic modesetting and scheduling per output;
  - several GPUs: rendering on one and scanning out on another;
  - each display at its own refresh rate from 60 to 360 Hz;
  - VRR, hotplug and direct scan-out.

  This backend also fixes both outputs appearing on one display.
- [ ] The display acceptance test on the reference laptop and on RTX and Intel desktops: under 1% missed frames per output at its own rate, and input to swap within two refresh periods at p95
- [ ] Steam and Proton inside gamescope, as a Wayland client of Prism; a game mode
- [ ] greetd with a Prism greeter, flicker-free from the firmware logo
- [ ] Touchpad gestures from libinput for the KMS backend
- [ ] Real daemons for every service contract (`qor-netd`, `-btd`, `-powerd`, `-audiod`, `-notifyd`, `-settingsd`) and an xdg-desktop-portal backend with prompts in the trusted layer
- [ ] Games and apps sandboxed (bubblewrap, seccomp, Landlock): the operating-system half of Demiurge's ADR-087

## Phase 4 · The ecosystem

*Ongoing.* Exit: an outside developer ships a QQ game to QOR OS using only the public
SDK.

- [ ] The public SDK: QOR.Kit, Lumen, the service interfaces and QQ, with a Qt Creator wizard and the simulator
- [ ] The Store and the Market as one catalogue in the shell: the Store's signature decides what installs; the chain records who made a work and who holds a licence
- [ ] Store stage 2: the developer program and portal, review, privacy-preserving analytics, with no payments ([STORE.md](STORE.md#roadmap))
- [ ] QQ's engine work continues, upstream until the cut-over (Demiurge's P3 track)
- [ ] The Mesh for distributing games and creations, off until turned on
- [ ] From QTOS's Phases 2 and 4: a browser (Qt WebEngine), HDR, fractional scaling, Quick Look, Stacks and Projects, hero transitions, a Qt-native screen reader, Increase Contrast, translations
- [ ] Paid items, only once [QOR-0006](decisions/QOR-0006-qor-os-1-0-moves-no-money.md)'s conditions are met and a record chooses the model
- [ ] Later than 1.0 ([QOR-0007](decisions/QOR-0007-what-1-0-ships.md)): GNOSIS, Stream, Spatial mode (Qt Quick 3D Xr), on-device intelligence, Continuity

## Release gates

Nothing is published until all of these hold
([QOR-0008](decisions/QOR-0008-before-a-public-release.md)). They are the
owner's to obtain. This list is their status. [release/](release/README.md)
holds, for each one, who must act, the next step, and what is prepared for
the owner's advisers.

- [ ] The Qt Company's written confirmation of the licence to distribute Qt in a QOR OS image and SDK ([release/QT.md](release/QT.md))
- [ ] Trademark clearance for "QOR OS", QQ and ARQADE ([release/TRADEMARKS.md](release/TRADEMARKS.md))
- [ ] Terms of Service, a privacy policy, and a legal opinion on rewards, prizes, identity checks, sanctions and age limits ([release/LEGAL.md](release/LEGAL.md))
- [ ] Code-signing certificates and the Secure Boot key ([release/SIGNING.md](release/SIGNING.md))
- [ ] An external security review of the QOR core and the vault service ([release/SECURITY-REVIEW.md](release/SECURITY-REVIEW.md))
- [ ] QOR ID in a second region with offline login; the key exposed on 5 October 2026 replaced ([release/SIGN-IN.md](release/SIGN-IN.md))

## History: QTOS phases

QOR OS carries QTOS's work. QTOS's own phases ended where QOR OS began:

- **Phase 0 · Foundations: complete (v0.1.0, 23 October 2026).**
  - Lumen tokens, `Theme`, `Spring`, SDF `Surface`, `Glass` and `Backdrop`.
  - The Lumen controls style and `QStyle`, and the platform theme.
  - Service contracts, the simulator, `qor-simd` and `qorctl`.
  - The whole shell in simulation, and six apps.
  - The benchmark, the tests and CI.
- **Phase 1 · Nested Prism: complete except three items.**
  - **Done:**
    - Prism nested on Linux and WSLg with real clients;
    - client windows (configure, decorations, moves and resizes, popups,
      activation) and Spaces for client windows;
    - the controller interface and real audio;
    - Wayland extensions (fractional-scale, cursor-shape, presentation-time,
      idle-inhibit);
    - three-finger gestures and several displays;
    - the Store's stage 1.
  - **Carried into QOR OS:**
    - the decision gate, deferred by QOR-0005;
    - the Qt Interface Framework evaluation and the Design Studio project,
      now in Phase 1 above.
- **Phases 2 to 5** are folded into the phases above:
  - the bootable image and the KMS backend into Phase 3;
  - the platform daemons and sandbox into Phase 3;
  - the premium and ecosystem items into Phase 4.

  The Yocto plan was replaced by QOR-0004.
