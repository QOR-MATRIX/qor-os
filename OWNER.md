# Where QOR OS stands

For Andrew, 30 October 2026. One page, rewritten every session (`AGENTS.md` §2).

## What QOR OS is

The operating system for players, game makers and creators: the Prism shell
(*the compositor and desktop in one*) and Lumen design, with Demiurge's QQ
engine, ARQADE and QOR Media, on Linux. Code: `QOR-MATRIX/qor-os` (private),
here at `X:\Repoz\QOR-OS`. Phase 1: the three products on Linux, in Prism.

## This session

1. **QQ runs inside Prism**, as ARQADE and QOR Media do: the Studio from Beam
   (*the search bar*), the Player with the game the shell hands it. Every
   window now opens at the size its app asks for. Speed is unchanged: 2.78
   and 2.79 ms a frame, before and after, on this PC.
2. **QQ Studio's errors are fixed:** a missing import that broke the Agent
   panel's colours, and Linux tests that pressed keys before the window had
   the keyboard. QQ's tests now fail on any such error.
3. **QQ plays with QOR OS's controllers.** QQ reads controllers through
   QOR.Input (*QOR OS's own controller code*) instead of Windows-only XInput.
   So DualSense and DualShock 4 work in QQ on Windows too, and on Linux any
   controller the kernel knows. ARQADE's games get the same. Only the app in
   front hears the controller.
   - In Prism, while the QQ Player has the focus, the shell's controller
     mode steps aside so a press is the game's alone; Guide brings
     Overview back.
   - Tested with virtual controllers, on Windows and Linux. A real DualSense
     in Prism needs a Linux machine: WSL (*Windows' built-in Linux*) sees no
     controllers at all.
4. **Checked:** QOR OS builds clean with every suite passing on Windows and
   Linux; QQ and ARQADE compile on both; on Linux without a graphics card
   QQ fails only its 7 known timing tests. Pushing runs the products' tests
   on Windows, which open windows on your desktop for about 10 minutes.

## Needs you

1. **A real Linux machine with a graphics card** (a Linux boot is best): it
   is now what stands between QOR OS and Phase 1's goal, Orb Run played in
   Prism with a DualSense (*the PlayStation 5 controller*).
2. **Legal, still open:** trademark advice for "QOR OS", which the site now
   uses publicly; the legal entity; the Qt letter. When a lawyer reads the
   list, add the new paragraph in `LICENSE` that makes the products' new work
   proprietary. Also open: replacing the 5 October key, and moving the Store
   keys' backup off this PC.

## Next

Phase 1: ARQADE and QOR Media answer the controller's D-pad with visible
focus; Lumen themes the products; then a controller-first Home and Library.
