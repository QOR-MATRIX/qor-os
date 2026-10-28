# Where QOR OS stands

For Andrew, 28 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
— *the compositor and desktop in one* — and Lumen design, joined with
Demiurge's QQ engine, ARQADE and QOR Media, on a Linux base. Code:
`github.com/QOR-MATRIX/qor-os` (private), on this PC at `X:\Repoz\QOR-OS`.

Phase 0 is complete. **Phase 1 has begun**: QQ, ARQADE and QOR Media on Linux,
inside Prism, driven by a controller.

## This session

1. **The carried products build and test on Linux.** QQ, ARQADE and QOR Media
   (the *carried products*, copied from demiurge-chain with their history)
   were Windows-only. `tools/dev/products.sh` builds and tests them on Linux.
   Under WSL, ARQADE passes everything. QOR Media passes everything but
   docking. QQ fails its agent-key cases, QQ Studio's F3 overlay and the
   Player's floor of 20 frames a second: it managed 13 to 15 under WSL.
2. **Their tests no longer appear on your desktop.** They run on a display
   nobody sees, with sound off. It is a small compositor of the script's own
   (`tools/dev/products-display.qml`) on Qt's *offscreen* platform, which
   draws into memory. `--display session` shows them if you want to watch.
3. **One Linux run froze this PC** at 3:08 PM, during QQ's Player suite, a few
   minutes after the GPU driver under WSL failed. Windows logged no cause.
   CI will not run the products on Linux until you set
   `QOR_RUN_LINUX_PRODUCTS` ([CI.md](docs/CI.md#the-carried-products)).
4. **Linux fixes in carried code**, each marked `Changed for QOR OS`:
   - Redo answers Ctrl+Y in QQ Studio and QOR Media;
   - QQ Studio's Duplicate keeps Ctrl+D;
   - the QQ Player measures its start-up;
   - QOR Media sorts "shot 2" before "shot 10" in WSL's C locale.

   The Linux kit gained Qt Quick 3D Physics and Qt PDF. **On Windows all
   three still pass every suite**, checked after these changes.

## Needs you

1. **Two decisions for Phase 1**. Both loosen or define a rule, so they are
   yours:
   - *Docking on Wayland.* The products dock into the Windows launcher's
     window by its handle, and Wayland has no handles, so QOR Media's docking
     test fails on Linux. Recommended: on Wayland, Prism places the window,
     and that test is skipped there.
   - *QQ Studio's agent key on Linux.* Windows keeps it in Credential
     Manager; Linux has nowhere yet, so five agent tests fail. Recommended:
     the Secret Service, as the roadmap says, recorded as the creator's own
     provider key, not a QOR key that must live in the core.
2. **Whether the Linux GPU suites may run on this PC again**, given the
   freeze. They will run only when you say.
3. **Still open from before**, as the tree shows: which legal entity;
   trademarks for QQ, ARQADE and "QOR OS"; the Qt letter
   ([docs/release/](docs/release/README.md)); replacing the key exposed on
   5 October; moving the Store keys' backup off this PC.

## Next

Phase 1, in this order: Prism hosts the products (it speaks their launcher
protocol and places their windows, which today opens each one 960 wide);
QOR.Input drives QQ's controls; Lumen themes them; then a controller-first
Home and Library. Find out why F3 fails on Linux. The exit is Orb Run playing
inside Prism on Linux with a DualSense, which needs you and the pad.
