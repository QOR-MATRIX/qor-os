# QOR-0018: The security and quality bar, checked by machines

- **Status:** Accepted
- **Date:** 2026-10-30
- **Deciders:** the owner, who asked that "all development is being done with the
  highest degree of quality and security standards for users of our platform
  and ecosystem". The checks are the assistant's choice.
- **Relates to:** [QOR-0008](QOR-0008-before-a-public-release.md) (the external
  review stays a release gate; this does not replace it), `AGENTS.md` §6 and §7

## Context

A review of how QOR OS is built on 30 October 2026 found these:

- **No exploit mitigations were asked for.** No build asked the compiler or linker
  for any, and the products were no different, though they read what games,
  Wayland clients, controllers and files hand them. Ubuntu's compiler adds some by
  default; QOR OS's image is on Fedora, and MSVC adds fewer.
- **Nothing looked for memory bugs.** No suite had ever run under a sanitizer.
- **CI's supply chain could be moved under it.** Every action was named by a tag its
  owner can move, two of them third parties', and they run on the workstation, which
  holds the Store's signing keys and the Qt account. Corrosion was also fetched by a
  tag, and the Python tooling's dependencies were unpinned, on the machine that
  signs with `cryptography`.
- **Nothing looked for a secret** before it reached a commit, or in the history,
  though `AGENTS.md` §6 forbids one there. Nothing would have said so if an advisory
  came out against the QOR core's dependencies after its pin moved.
- **Suites heard the machine they ran on.** QQ's Studio and agent suites took input
  from a real controller on the runner, whose stick walked and turned the player out
  of their cases, while `docs/CONTROLLERS.md` said every test was fenced from them.

Two independent reviews of the trust boundaries, read from the code that day, found
what those tools would not:

- **The service bus** was an abstract socket, which takes no permissions: any user on
  the machine could change settings, post notifications and crash the compositor.
- **Any Wayland client took the keyboard** by opening a window it never drew, on the
  lock screen too. **A game controller unlocked the session**, and the lock screen
  checked nothing.
- **A client could be Settings or ARQADE** by saying so, and its title could paint over
  every window and make Prism fetch a URL.
- **A Store widget was verified once**, then ran from a folder any process of the
  person's could rewrite, inside the compositor.
- **A game package could run native code** in the Player through a `qmldir`, read any
  file through a symbolic link and reach the network through a sound.
- **Opening a downloaded project in QQ Studio ran its code**, with no click, with the
  creator's own reach, and logic a model wrote ran with no approval.

## Decision

1. **Every program is built with exploit mitigations**, everywhere and on every
   preset (`cmake/QorHardening.cmake`, included by the OS and each product):
   - **Windows:** Control Flow Guard, CET shadow stacks, high-entropy address
     randomisation, no execution from data, stack cookies.
   - **Linux:** position-independent executables, full RELRO, no executable stack,
     `_FORTIFY_SOURCE=3`, `_GLIBCXX_ASSERTIONS`, strong stack protection, stack-clash
     protection, and control-flow protection on x86-64.
   - **Checked by reading what came out.** `tools/ci/check_hardening.py` reads each
     program's own headers in CI, so a lost flag fails the build.
2. **The suites run under AddressSanitizer and UBSan** on Linux (the `linux-asan`
   preset). A finding is a bug to fix, not to suppress; a suppression names its cause
   and whose code it is.
3. **What CI runs is pinned by content.** Actions by commit, tools by SHA-256 or
   exact version, Corrosion by commit, the Python tooling by hash
   (`requirements-dev.in` to `requirements-dev.txt`). Dependabot proposes the moves.
4. **A dependency update waits for a person.** Dependabot's pull requests do not run
   on the workstation; they run on hosted runners, and on the workstation only once
   a person has merged them.
5. **Secrets are looked for at the door and in the history.**
   `tools/ci/check_secrets.py`, in pre-commit and Lint, with Python alone; and
   `gitleaks` over the whole history in the Security workflow, on a hosted runner.
   Neither prints a value. A file that holds a made-up value is listed with its
   reason.
6. **The QOR core's dependencies are audited at the pin, weekly** (`cargo audit`, the
   Security workflow), since an advisory comes whether or not anything here changed.
7. **A suite hears nothing of the machine it runs on.** No real controller, sound
   device or keyring reaches one, by a means a new suite cannot forget
   (`products/qq/tests/virtualpads.cpp`, built into every suite by CMake).
8. **What crosses a trust boundary is hostile, and has a test from that side.**
   - **Prism treats every client as hostile.** It takes the keyboard only when it can
     be seen and steals it from nobody, and never on the lock screen; its name never
     stands for one of Prism's; its strings are plain text; the shell's engine has no
     network.
   - **One way off the lock screen**, through one check. A controller types the
     password; it never unlocks. A session with nothing to check against stays locked.
   - **The bus is the person's alone** (a socket in their 0700 runtime folder), and a
     device's image has no simulator at all (`QOR_BUILD_SIMULATOR=OFF`).
   - **What the Store installed is verified each time it is loaded**, and only QOR OS's
     own widgets run inside the shell.
9. **A game is walled in by the kernel as well as by QQ.** On Linux the QQ Player
   raises Landlock and seccomp before it makes a thread (`products/qq/player/sandbox.h`):
   the operating-system half of Demiurge's ADR-087, for what no check inside the process
   can close. It comes before Phase 3's bubblewrap and portals, which it does not replace.
10. **QQ Studio asks before a project made elsewhere runs**, keeps a list of what was
    made or trusted here (which is also what Prism's Library calls a person's own), and
    runs logic an agent wrote only once the creator allows it.

## Consequences

- A build folder made before this change rebuilds in full once.
- `linux-asan` costs a build of its own, about as long as a full build.
- Mitigations cost a little at run time. Prism was measured before and after on the
  same machine ([PERFORMANCE.md](../PERFORMANCE.md)).
- A Dependabot pull request shows no Windows or Linux build until it is merged, so a
  person reads its diff first.
- **Prism's and the Player's limits are written down** in [SECURITY.md](../../SECURITY.md#known-limits),
  with what would close each.
- A window a client opens no longer takes focus from another client's: it asks for
  attention, unless it presents a token or Prism started it. Starting an editor from a
  terminal with no token behaves so too.
- Anything already installed from the Store before this has no manifest beside it and
  is not loaded until installed again.
- **What this does not do.** It does not make the runner safe to hold the Store's
  keys: a job on the workstation runs as its user and can read them. Moving the keys
  off that machine is the owner's, and is on [OWNER.md](../../OWNER.md). Nor does it
  replace the external review, which stays a release gate
  ([QOR-0008](QOR-0008-before-a-public-release.md)).
- **A sign this is wrong:** a mitigation that breaks something real, such as Qt's QML
  JIT under CET on some machine. Then a later record lifts that one, for that target,
  and says why.
