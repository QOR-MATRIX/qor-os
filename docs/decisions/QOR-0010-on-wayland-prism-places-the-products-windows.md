# QOR-0010: On Wayland, Prism places the products' windows

- **Status:** Accepted
- **Date:** 2026-10-28
- **Deciders:** the owner
- **Relates to:** [QOR-0002](QOR-0002-one-repository-from-qtos-and-demiurge.md)
  (the carried products); Demiurge ADR-046 and ADR-102 (the launcher's host
  protocol); the roadmap's Phase 1

## Context

ARQADE and QOR Media dock into the Windows launcher's window. The launcher's
`place` command carries a rectangle and the native handle of its own window.
The product's `Dock` (`products/arqade/qt/src/dock.cpp`,
`tools/qor-media/src/dock.cpp`) wraps that handle with
`QWindow::fromWinId`, makes it the product window's parent, and sets the
window frameless at the rectangle. QOR Media's playback suite tests this
(`it_docks_inside_the_launchers_window_and_lifts_out_to_full_screen`).

Wayland has no handles for one program to take another's window by. Qt's
Wayland platform cannot wrap a foreign window, so there `fromWinId` returns
nothing, `Dock` leaves the product's window a toplevel of its own, and the
compositor decides where it goes. On 28 October, with the products building
on Linux, that docking case was the one QOR Media failure on Wayland.

Alternatives considered:

- **xdg-foreign**, a protocol by which one client lets another use its
  toplevel as a parent. It makes a dialog-like relationship, not a child
  drawn inside the parent. Prism would have to support it, and the products
  would need a Wayland-specific path.
- **Embedding through Prism's own protocol.** That would be new client-side
  code in carried products, for something Prism can do alone.

## Decision

1. **On Wayland the products' windows stay toplevels, and Prism places
   them.** The launcher's `place` request becomes a layout request to Prism,
   as the roadmap's Phase 1 says. The products' `Dock` stays as it is for
   Windows, and no Wayland-specific code is added to it.
2. **QOR Media's docking case is skipped on Wayland**, saying why. It still
   runs, and must pass, on Windows.

## Consequences

- Until Prism speaks the launcher's side of the host protocol, a product on
  Wayland opens as an ordinary window that Prism lays out. On QOR OS, docking
  belongs to the compositor.
- A skipped case is a check not run, which `AGENTS.md` §7 leaves to the owner.
  This record is that decision, for this one case on this one platform.
- Prism's handling of `place` gets its own tests when it is built (Phase 1).
  When those pass, the products' docking is tested again on Wayland, by them.
- This would be wrong if a product had to draw inside another program's
  window on Wayland, which only a compositor protocol could do. Then a later
  record supersedes this one.
