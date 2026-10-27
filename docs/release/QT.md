# Qt: the licence to distribute QOR OS

*Prepared on 27 October 2026 by an assistant, from the research listed under
[Sources](#sources), for the owner and their advisers. It is research, not
legal advice.*

**The gate** ([QOR-0008](../decisions/QOR-0008-before-a-public-release.md)):
"The Qt Company has confirmed in writing the licence for distributing Qt in a
QOR OS image and SDK."

Qt's defined terms keep Qt's spelling here ("License Certificate",
"Distribution License").

## What the owner holds, as recorded

- [QTOS-0004](../decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md),
  [VISION.md](../../VISION.md) §14 and [QT-MODULES.md](../QT-MODULES.md) call it
  a **"Qt Enterprise"** commercial licence. It covers the framework, Qt
  Creator, Qt Design Studio and Qt for MCUs, with commercial-only tooling such
  as the Qt Quick Compiler extensions (`qmlsc`).
- An assistant's notes from earlier sessions, recording what the owner said,
  add: valid to 2027-09-27, includes Qt AI Assistant. The end date is not in
  the repository ([QT-MODULES.md](../QT-MODULES.md) lists Qt Creator "incl.
  Qt AI Assistant"), and neither was checked against the Qt account.
- **Not recorded anywhere:** the product as Qt now names it (Qt for
  Application Development Professional or Enterprise, ADP or ADE; Qt for
  Device Creation Professional or Enterprise, DCP or DCE); the agreement and
  Appendix versions; the number of Designated Users; the Development and
  Deployment Platforms on the License Certificate; and **the licensee**. See
  the entity question in [README.md](README.md#one-question-first-which-legal-entity).
- No Distribution Licenses, no SDK agreement and no written permission for a
  "Prohibited Combination" are recorded.

A hint, unconfirmed: QTOS-0004 says the licence includes Qt for MCUs, and
Qt's FAQ says MCUs subscriptions convert to DCP or DCE. Only the License
Certificate settles it.

## What QOR OS uses

From the build files and [QT-MODULES.md](../QT-MODULES.md). Under a commercial
licence "all modules are available under a single, commercial Qt license"
([doc.qt.io/qt-6/licensing.html](https://doc.qt.io/qt-6/licensing.html)). The
open-source class is given for reference: it matters for open-source use,
if commercial cover ends, and for the Community Edition questions in the
letter (7 and 8).

| Module | Open-source class | Used by |
|--------|-------------------|---------|
| Qt Core, GUI (with private API), QML, Quick, Quick Controls (Templates, Basic, Material), Quick Dialogs, Quick Layouts, Quick Effects, Quick Shapes, Widgets | LGPLv3 or commercial | The shell, Kit, Lumen, every app, QQ, ARQADE, QOR Media |
| Qt Network, Concurrent, SVG, Image Formats, Remote Objects | LGPLv3 or commercial | Service IPC, the music scan, icons, WebP, ARQADE's server, QQ's MCP pipe |
| Qt Shader Tools | LGPLv3 or commercial for the library; the `qsb` tool GPLv3 or commercial | Lumen's shaders (build time) |
| Qt Multimedia, with the FFmpeg 9.0.1 libraries from Qt's installer (LGPL-2.1-or-later) | LGPLv3 or commercial | The audio engine, QOR Media, QQ |
| Qt PDF | LGPLv3 or commercial per current docs; unconfirmed for 6.12 (a 2019 Qt blog announced GPLv3 from 5.14) | QOR Media's PDF reader |
| Qt Graphs | GPLv3 or commercial | The Monitor app |
| Qt Wayland Compositor (with private API) | GPLv3 or commercial | Prism, the compositor and shell |
| Qt Quick 3D (Helpers, Asset Utils, Particles3D), Qt Quick 3D Physics | GPLv3 or commercial | QQ; ARQADE's 3D home and games |
| Qt Spatial Audio | LGPLv3 or GPLv3, or commercial | QQ |
| Qt for WebAssembly (multithreaded) | GPLv3 or commercial | The QQ Player in a browser |
| Qt Quick Compiler extensions (`qmlsc`) | Commercial only | Ahead-of-time compiled QML in every build |
| Build tools (`moc`, `rcc`, `qmlcachegen`, `qmllint`, `qsb`, `repc`, `windeployqt`) | GPLv3 with the Qt GPL exception, or commercial | Build only |
| Qt Test, Qt Quick Test | LGPLv3 or commercial | Tests (not expected in the image) |
| Planned: Qt Application Manager (Phase 3; the SDK template's `info.yaml`), Qt Interface Framework (being evaluated) | GPLv3 or commercial (Application Manager) | Commercially, Exhibit 1 lists both **only for DCE** |
| Planned: Qt Wayland client plugin, Virtual Keyboard, Lottie, Quick Timeline, Network Authorization, gRPC, HTTP Server, Canvas Painter | Mixed | Phases 1 to 4 |

**Qt in Demiurge's products, from the same owner** (upstream, public):

- **The Windows launcher** bundles QOR Media in every pre-release from
  `launcher-v0.1.10`, and ARQADE from `launcher-v0.1.14`, as Qt apps
  deployed by `windeployqt`, with Qt's Windows libraries. QOR Media brings Qt
  Multimedia's FFmpeg backend, and from `launcher-v0.1.17` Qt PDF and PDFium;
  ARQADE links Qt Quick 3D (`tools/qor-media/build.ps1`,
  `products/arqade/qt/build.ps1`, `tools/qor-launcher/scripts/stage-arqade.mjs`;
  each tag's `tauri.bundle.conf.json` and `CMakeLists.txt`).
- **The Android launcher on Qt** ([ADR-104](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-104-the-launcher-on-android-on-qt.md)):
  a Qt 6.12 app (`tools/qor-launcher/shell/`, ID `app.qorsync.dev`, Android 9
  and later, target SDK 36) over the same Rust core. Today it links Qt Core,
  GUI, QML, Quick and Quick Controls 2. ADR-104 plans ARQADE's Qt module in
  the same process, delivery as a signed APK on GitHub Releases, and Google
  Play later. No Android release exists yet; an arm64 APK builds.
- **The QQ Player** builds natively and as WebAssembly (Demiurge's
  `docs/SYSTEMS.md`). No publicly hosted copy was found.

## How Qt's published terms treat applications and devices

All pages were read on 27 October 2026. The qt.io pages show no last-updated
date (footer: © 2026). The terms read are the **Appendix for Qt Development
Framework, Version 2026-01** ("compliant with Qt Frame Agreement 2023-06 or
later") and its **Exhibit: Licensed Software 2026-01**. The owner's agreement
may be an older version with other wording and numbering.

**Two developer products.**
[qt.io/development/qt-framework/commercial-qt](https://www.qt.io/development/qt-framework/commercial-qt):

> Qt for Application Development is "used to create applications that run
> exclusively on desktop and mobile operating systems as one of many
> applications (such as email, browser, games, office apps, etc.)".
> "The AD license can't be used to create devices powered by Qt-based software."

> Qt for Device Creation is "needed for each Qt user to build hardware devices
> that incorporate, integrate, or link to parts of Qt libraries. It's also
> needed in cases where the main functionality of your hardware product
> depends on Qt-based software, and it can be used to create both
> applications and devices."

[qt.io/faq/qt-commercial-licensing](https://www.qt.io/faq/qt-commercial-licensing):

> Application Development "is sufficient to create applications that run on
> desktop PCs, mobile phones, or tablets … A Qt-based application is only one
> of many applications on these computers."

> Answering "What does a 'joint hardware and software distribution' mean in
> practice?", which begins "If you are making an embedded device with Qt, you
> most likely are creating a Device …": "Any Device built with or
> incorporating Qt is subject to a distribution license fee, please contact
> us for more information."

**Applications** (Appendix §5.2,
[qt.io/terms-conditions/qt-dev-framework](https://www.qt.io/terms-conditions/qt-dev-framework)):
a "perpetual, royalty-free" right to distribute Applications "for use on the
Deployment Platforms". Deployment Platforms are the operating systems or
hardware named on the License Certificate (§2.6).

**Devices** (§2.10). A Device is hardware distributed with Licensed Software,
or:

> "Applications which provide Substantial Functionality … by communicating,
> integrating, or otherwise interacting in any way with a hardware or compute
> unit, regardless of whether the Application is provided together with the
> hardware … or separate therefrom".

"Substantial Functionality" means "a normal, intended or specified operation
of a hardware or compute unit is dependent on or limited by the Application".

Device distribution needs **Distribution Licenses** (§2.11, §5.3, §9.3):
bought before distribution, "sold separately for each type of Device", and
"Each time Customer distributes a Device, one Distribution License is used
…".
The commercial-qt page adds that they "must be purchased latest 60 days before
devices are shipped or placed into inventory, and they are based on device
features and volumes", with tier thresholds of "10,000 -100,000 pcs" or packs
of "5 to 5000 devices". **No prices are published**:
[qt.io/terms-conditions/pricing](https://www.qt.io/terms-conditions/pricing)
says "Please contact sales".

**Conditions on both** (§5.4.1):

- (iv) "Applications may not pass on functionality which in any way makes it
  possible for others to create software with Licensed Software". The
  exception covers scripting and QML used "solely in order to enable
  scripting, themes and styles that augment the functionality and appearance
  of the Application(s) without adding primary and substantial functionality".
- (vi) No Qt names or logos in marketing, except the "Built with Qt" logo.
- (viii) End Customers may not be given rights to copy, modify,
  reverse-engineer or redistribute the Redistributables "except as part of
  the onward sale".
- (ix) No "Prohibited Combination" with software "created with or
  incorporating Qt Community Edition" without "specific advance written
  permission". Without it, distributing "a hardware device or product" that
  incorporates Licensed Software or Qt Community Edition, or whose material
  functionality depends on it, "shall be considered to be Device
  distribution". The exception is "Permitted Software" (§2.21): "third party
  products that are generally available to the public, which may include
  parts of Qt Community Edition or be developed using Qt Community Edition".
- (x) Affiliates, Contractors and End Customers must be bound by these terms.
- §5.4.3: use beyond the Appendix needs another licence, for example "a
  software development kit "SDK" agreement with regard to limitations of
  Section 5.4.1 iv, vii or viii".

**Redistributables** (Exhibit 1,
[exhibit-licensed-software-2026-01](https://www.qt.io/terms-conditions/qt-dev-framework/exhibit-licensed-software-2026-01)):
modules are marked "X, R" (redistributable in object code); tools such as
`moc`, `rcc`, `qmlcachegen`, `qmllint`, `windeployqt`, Qt Creator and Qt
Design Studio are "X" only. Exhibit 1 does not name Qt Graphs, Quick 3D
Physics, Spatial Audio, Quick Effects, Quick Shapes or Canvas Painter.

**After a subscription ends** (FAQ): "you can continue to distribute your
applications after your subscription has expired", but "you are not allowed
to make any changes to the software including bug fixes or security updates
using Qt's tools." Terms renew automatically unless cancelled 30 days before
the end (§5.1.4).

## Why this is the open question

The published terms do not settle whether a free OS image that people install
on PCs they already own is an **Application** (royalty-free) or a **Device**
(a Distribution License per unit).

- Read plainly, the "one of many applications" wording may not fit a system
  compositor and shell; that reading is the research's, not Qt's. Prism is
  the only graphical session.
- §2.10's "Substantial Functionality … regardless of whether … provided
  together with the hardware", and §5.4.1(ix)'s "hardware device or product",
  could be read to cover it.
- Handhelds sold with QOR OS preinstalled are a separate case for Qt to
  answer.
- The SDK, QML written by players and an AI assistant in QQ Studio, QML
  widgets from the Store, and theming of Community Edition apps all touch
  §5.4.1(iv) and (ix).

Only The Qt Company can answer. That is why QOR-0008 makes it a gate.

## Already distributed: worth checking now

- Demiurge's launcher pre-releases (`launcher-v0.1.10` to `launcher-v0.1.17`;
  there is no 0.1.13) already carry Qt, as described above.
  [ADR-083](https://github.com/QOR-MATRIX/demiurge-chain/blob/main/docs/decisions/ADR-083-qq-on-qt.md)
  assumed the owner's licence covers this and left it "for the owner to
  confirm with Qt". The letter asks.
- Separately from Qt's agreement, FFmpeg (LGPL-2.1-or-later), PhysX
  (BSD-3-Clause), Resonance Audio (Apache-2.0), libwebp (BSD-3-Clause) and
  PDFium have licence terms of their own on notices, and the LGPL has terms
  on relinking; what they require of a distributor is for counsel to
  confirm. Whether the public installers carry Qt's and FFmpeg's notices was
  not checked. The licence files they are known to add are
  `licenses/libgit2-COPYING.txt`, PDFium's licences beside QOR Media (from
  `launcher-v0.1.17`) and the fonts' OFL texts beside ARQADE (its
  `build.ps1`); they carry no copy of demiurge-chain's `LICENSE`.

## The contact route

qt.io has no form for licensing questions. Found on qt.io on 27 October 2026:

1. **Existing customers:** [qt.io/contact-us](https://www.qt.io/contact-us)
   says questions about "your licenses, payment, or account" go "through your
   Qt account web portal": [account.qt.io](https://account.qt.io/login).
   Support issues go to [login.qt.io](https://login.qt.io).
2. **The account manager** named on the order confirmation or renewal quote:
   [name and email]. The repository does not record one.
3. **Qt Sales:** [contact form](https://www.qt.io/contact-us#sales-contact);
   [Device Creation quote request](https://www.qt.io/contact-us/dcp-quote-request);
   the sales phone listed on the contact page, +358 988 618 040.
4. **The licence management team:** `license-management@qt.io`. The
   [License Asset Management FAQ](https://www.qt.io/faq/license-asset-management)
   says "If unclear, please contact the license management team at
   license-management@qt.io". It handles licence reviews and may pass new
   terms to sales.

`qtaccount@qt.io` appears only for the Small Business plan, so it is not the
right desk.

**Suggested:** send through the Qt Account portal or to the account manager,
copy `license-management@qt.io`, and ask for a reply from someone authorised to
bind The Qt Company.

## The letter

Fill every [placeholder] from the License Certificate and the owner's records.
Remove a bracketed option that does not apply. Whether to mention the
already-public launcher builds is the owner's call, best made with counsel;
the draft includes them.

[Counsel to review section 1's description and questions 2, 13 and 15 before
sending.]

---

**Subject:** Request for written confirmation: distributing Qt in the QOR OS
operating system image and SDK ([Qt customer or agreement number])

**To:** [account manager named on our order confirmation or renewal quote],
The Qt Company
**Sent via:** the Qt Account customer portal (account.qt.io); copy to
license-management@qt.io
**From:** [Name], [Title], [Legal entity: full registered name, country of
registration]
**Date:** [date]

Dear [Name],

[Legal entity] holds a commercial Qt licence. Our records call it "Qt
Enterprise". On our License Certificate it is [product name, for example Qt
for Application Development Enterprise or Qt for Device Creation Enterprise],
under agreement [number] ([Frame Agreement and Appendix versions]), for
[number] Designated User(s), with the current term ending [date on the
certificate]. Our Qt Account is [account email or customer ID].

We are building an operating system with Qt, called QOR OS. We have decided
not to publish it until The Qt Company has confirmed in writing how our
licence applies to distributing it. This letter describes what we plan to
distribute and asks specific questions. Where our licence does not cover
something, please tell us what would, and send a quote.

Section numbers refer to the Appendix for Qt Development Framework, version
2026-01, as published on qt.io. If our agreement is another version, please
map them to it.

**1. What we plan to distribute**

*The QOR OS image.* QOR OS is a Linux operating system for x86-64 PCs and
handheld PCs. It is built on Fedora Atomic (bootc), which we do not develop.
The graphical session and our own apps are software we write, built with Qt
6.12 from our commercial kit:

- Prism, the Wayland compositor and desktop shell, which is the system's only
  graphical session;
- system services that talk to the shell over Qt Remote Objects;
- first-party apps: settings, a system monitor, and a media player and PDF
  reader;
- QQ, a 3D game engine with an editor (QQ Studio) and a player;
- ARQADE, a games arcade.

The image will be a free download that people install on hardware they
already own. [We do not make or sell hardware today.] [We may later work with
[partner] to sell handhelds with QOR OS preinstalled.] Updates are delivered
as complete new images. We expect about [number] installations in the first
year and [number] by [year].

*Qt in the image.* The image will contain:

- Qt Core, GUI (including private APIs), QML, Quick, Quick Controls, Quick
  Layouts, Quick Effects, Quick Shapes and Widgets;
- Qt Network, Concurrent, SVG, Image Formats, Remote Objects and Shader Tools;
- Qt Multimedia, with the FFmpeg libraries supplied by the Qt online
  installer;
- Qt PDF and Qt Graphs;
- Qt Wayland Compositor (including private APIs);
- Qt Quick 3D (with Helpers, Asset Utils and Particles), Qt Quick 3D Physics
  and Qt Spatial Audio.

All of our own QML is compiled ahead of time with the Qt Quick Compiler
extensions (`qmlsc`); QML that users write in QQ Studio is loaded at run time
(see below). Planned later: the Qt Wayland client plugin, Qt Application
Manager, Qt Virtual Keyboard, [Qt WebEngine] and [others].

*A public SDK (later).* We plan an SDK so that other developers can build apps
for QOR OS. It would contain our QML modules (QOR.Kit, and Lumen, a Qt Quick
Controls style), our service interfaces, the QQ engine, a desktop simulator
of the shell, and a Qt Creator project wizard.

*Content made by users.* In QQ Studio, players and makers build games whose
scenes and logic are QML and JavaScript, run by QQ's runtime. An AI assistant
in the editor also writes that QML. Games run in the QQ Player, natively and
as a WebAssembly build in a web browser. Desktop widgets from our store are
also QML, run by the shell.

*Third-party Qt software.* Fedora's repositories and Flatpak runtimes carry
Qt Community Edition. We would like the image to include, or let users
install, open-source software built with it: KDE Frameworks libraries, the
Calamares installer, and apps such as KeePassXC, VLC or Krita. We have also
written a Qt platform-theme plugin and a QStyle so that those apps match the
system's look. To load into those apps, the plugins have to be built against
the same Qt the apps use.

*Our launcher apps.* [We / Our affiliate [name]] also make[s] the QOR
Launcher:

- for Windows, whose installer includes our QOR Media Qt app and, since
  version 0.1.14, our ARQADE Qt app, with Qt's Windows libraries;
- for Android, a Qt 6.12 app (package `app.qorsync.dev`, Android 9 and later)
  that will include ARQADE in the same process, to be delivered as a signed
  APK on GitHub and later through Google Play.

*What is already public.*

- The source of QQ, ARQADE and QOR Media is published under the Apache-2.0
  licence at github.com/QOR-MATRIX/demiurge-chain. It contains no Qt source
  code.
- Tester builds of the Windows launcher, published as GitHub pre-releases
  since 17 October 2026, include QOR Media with Qt's Windows libraries (among
  them Qt Multimedia with FFmpeg, and in the latest build Qt PDF); those
  since 18 October 2026 also include ARQADE, which uses Qt Quick 3D.
- [The Android launcher has not been released.]
- [The QQ Player's WebAssembly build is not hosted publicly.] [or: is hosted
  at [address].]
- QOR OS's own source is private and proprietary.

*How we develop.* We have [one] Designated User, working on [a Windows PC,
with Linux under WSL]. AI coding assistants run on that machine. Self-hosted
CI runners at [location or locations] install Qt with our Qt Account.

**2. Our questions**

1. Which product, agreement version, Development Platforms and Deployment
   Platforms does our licence include? Are Linux (x86-64), Windows (x86-64),
   Android (arm64-v8a and x86_64) and WebAssembly Deployment Platforms for us?
2. Is the QOR OS image, downloaded free and installed by users on PCs they
   own, an "Application" (Section 5.2) or a "Device" (Sections 2.10 and 5.3)?
   Prism is the system's compositor and shell. Does the "Substantial
   Functionality" part of Section 2.10 apply to it? Is the image a "product"
   under the second sentence of Section 5.4.1(ix)?
3. Does the answer change for handheld PCs: (a) installed by the owner on a
   handheld bought elsewhere; or (b) preinstalled by us or a partner on
   handhelds sold to consumers?
4. If Distribution Licenses are needed:
   - Which product must we hold?
   - What counts as one distribution: a download, an installation, an
     activated machine, or each image update?
   - How many Distribution Licenses does one installation use, and how does
     "type of Device" apply to PCs and handhelds?
   - How does the rule to buy 60 days before shipping apply to downloads?
   - Is there an arrangement for software given away free?
   - What is the price? Please quote for [number] installations.
5. End users will copy the image to USB installers, reinstall it, run it in
   virtual machines and use it on more than one of their own computers, and
   mirror sites may host it. What may we permit under Section 5.4.1(viii),
   and may third-party mirrors distribute the image?
6. What must our end-user licence contain to meet Sections 5.4.1(ii) and (x)?
   How must users accept it: at download, or at first boot? Is there notice
   text you require?
7. Are Fedora's Qt Community Edition packages, KDE Frameworks, Calamares and
   third-party apps like those named above "Permitted Software" (Section
   2.21)? May they ship in the same image, or be installable on it?
8. May we build our platform-theme plugin and QStyle against Qt Community
   Edition so they load into those apps, while the rest of QOR OS uses our
   commercial licence? If that is a Prohibited Combination, will you give
   advance written permission? If not, what do you recommend?
9. What licence or agreement do we need to publish the SDK, given Section
   5.4.1(iv) and the SDK agreement mentioned in Section 5.4.3?
   - Must third-party developers hold their own commercial Qt licence?
   - If they use Qt Community Edition, may their apps run on QOR OS using the
     Qt libraries in our image or our QML modules, or does that need your
     written permission?
   - Would apps that bring their own Qt (for example in a Flatpak runtime)
     and talk to the system only over IPC avoid the question?
10. Is QQ Studio within the scripting exception of Section 5.4.1(iv)? In it,
    users and an AI assistant write QML and JavaScript game scenes and logic
    that our runtime executes. Are QML desktop widgets from other developers
    within it? If not, what agreement would cover them, including games
    played in a browser through our WebAssembly player?
11. May we distribute a Qt Creator project wizard or plugin for QOR OS? We
    understand that Qt Creator itself, and the tools that Exhibit 1 does not
    list as redistributable (`moc`, `rcc`, `qmlcachegen`, `qmlsc`, `qsb`,
    `repc` and so on), cannot be included in our SDK. Please confirm.
12. Please confirm that every module listed above may be redistributed in
    the image under our licence, including the private APIs, the FFmpeg
    libraries from the installer, and code generated by `qmlsc`. Several
    modules we use (Qt Graphs, Quick 3D Physics, Spatial Audio, Quick Effects,
    Quick Shapes) are not named in Exhibit 1. Does our licence include Qt
    Application Manager and Qt Interface Framework, which Exhibit 1 lists only
    for Qt for Device Creation Enterprise?
13. Are these covered as Applications under Section 5.2: (a) the Windows
    launcher with ARQADE and QOR Media, including the tester builds already
    published; (b) the Android launcher, on GitHub and later Google Play;
    (c) the QQ Player as WebAssembly, if we host it on a website?
14. Is publishing our own Qt-using source under Apache-2.0 consistent with
    our agreement? How should we handle outside contributions to that public
    repository, given Section 5.1.1?
15. Is our development setup within Section 5.1: one Designated User, AI
    coding assistants, and self-hosted CI runners that use our Qt Account? If
    not, how many Development Licenses do we need?
16. What must we keep active to keep shipping security updates to installed
    systems, given Section 5.3.2 and your FAQ on changes after a subscription
    ends? What happens to installed systems if our subscription ends?
17. May our public materials say QOR OS is "built with Qt", and use the
    "Built with Qt" logo?
18. If the licence must be held by a different legal entity of ours, how is
    it transferred?

**3. What we are asking for**

Please send a written reply to each question from someone authorised to bind
The Qt Company. Where our licence does not cover something, please send the
amendment, written permission, SDK agreement or quote that would. We will not
publish a QOR OS image or SDK until we have it. If a call would be easier
first, we are glad to arrange one.

Kind regards,

[Name]
[Title], [Legal entity]
[Postal address]
[Email] | [Phone]

---

## After the reply

- Keep the reply with the agreement records, outside the repository.
- Tick the gate in [ROADMAP.md](../ROADMAP.md#release-gates) with the date.
- If the answer is "Device", or changes the image's contents or the SDK, a
  QOR record says what changes.
- Correct the licensee's name in [`LICENSE`](../../LICENSE),
  [`THIRD_PARTY_NOTICES.md`](../../THIRD_PARTY_NOTICES.md) and
  [`VISION.md`](../../VISION.md) §14 if it is not "QOR-MATRIX", and, since
  records are not edited, write a QOR record that corrects
  [QTOS-0004](../decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md)'s
  statement.
- Whatever end-user licence Qt's answer to question 6 requires becomes part
  of [LEGAL.md](LEGAL.md)'s work.

## Uncertainties

- Application or Device is unsettled by the published terms. Only Qt can
  answer.
- The licence product, agreement version and licensee are unverified.
- The term end date 2027-09-27 comes from an assistant's notes of what the
  owner said, not the Qt account.
- Qt PDF's class for 6.12 is unconfirmed. doc.qt.io's Qt PDF pages redirect to
  a development snapshot that says LGPLv3/GPLv2. A 2019 Qt blog, seen only as
  a search result, announced GPLv3 from Qt 5.14.
- No Distribution License prices are published, and nothing was found on free
  distribution.
- The image's third-party contents (Community Edition Qt, KDE Frameworks,
  Calamares, Flatpak Qt apps) are not decided yet (Phase 3). Whether §5.4.1(ix)
  applies depends on them.
- Reading the platform theme, the QStyle, QQ Studio and Store widgets against
  §5.4.1(iv) and (ix) is the research's reading, not Qt's.
- The Android and Windows dependency graphs of the launchers were not scanned
  for third-party licences.

## Sources

| What | Where |
|------|-------|
| Appendix for Qt Development Framework 2026-01 (§2.6, 2.10, 2.11, 2.21, 2.23, 5.1 to 5.4, 9.3, 10.2, 11.2), read 27 October 2026 | <https://www.qt.io/terms-conditions/qt-dev-framework> |
| Exhibit: Licensed Software 2026-01 | <https://www.qt.io/terms-conditions/qt-dev-framework/exhibit-licensed-software-2026-01> |
| Commercial Qt: AD and DC, Distribution licenses, 60 days, tiers and packs | <https://www.qt.io/development/qt-framework/commercial-qt> |
| Commercial licensing FAQ | <https://www.qt.io/faq/qt-commercial-licensing> |
| Pricing terms: "Please contact sales" | <https://www.qt.io/terms-conditions/pricing> |
| License Asset Management FAQ | <https://www.qt.io/faq/license-asset-management> |
| Contact page | <https://www.qt.io/contact-us> |
| Qt 6.12 licensing: GPLv3-only modules; one commercial licence | <https://doc.qt.io/qt-6/licensing.html> |
| `qmlsc` "Available for commercial customers" | <https://doc.qt.io/qt-6/qtqml-qtquick-compiler-tech.html> |
| Qt for WebAssembly licensing (Qt 6.12): commercial or GPLv3 | <https://doc.qt.io/qt-6/wasm.html> |
| Module licence statements (Qt 6.12.0) | doc.qt.io pages for Wayland Compositor, Quick 3D, Quick 3D Physics, Graphs, Spatial Audio, Shader Tools, Remote Objects, Multimedia, Image Formats |
| Qt PDF (development snapshot docs) | <https://doc-snapshots.qt.io/qtpdf/qtpdf-index.html> |
| Qt Application Manager 6.11 | <https://doc.qt.io/QtApplicationManager-6.11/index.html> |
| QOR OS's module map and records | [QT-MODULES.md](../QT-MODULES.md), [QTOS-0004](../decisions/qtos/QTOS-0004-commercial-qt-and-proprietary-repository.md), [QOR-0004](../decisions/QOR-0004-the-base-image-is-fedora-atomic.md), [QOR-0008](../decisions/QOR-0008-before-a-public-release.md), [VISION.md](../../VISION.md) §14, [image/README.md](../../image/README.md), [sdk/README.md](../../sdk/README.md), [STORE.md](../STORE.md) |
| Demiurge (origin/main `e99c7ce`, read only) | `docs/decisions/ADR-083-qq-on-qt.md`; `docs/decisions/ADR-104-the-launcher-on-android-on-qt.md`; `docs/SYSTEMS.md`; `tools/qor-launcher/shell/CMakeLists.txt`; `tools/qor-launcher/src-tauri/tauri.bundle.conf.json`; `tools/qor-launcher/scripts/stage-arqade.mjs`; `tools/qor-media/build.ps1`; `products/arqade/qt/build.ps1`; the same files at each `launcher-v0.1.*` tag |
| Public launcher releases | `gh release list -R QOR-MATRIX/demiurge-chain`, 27 October 2026 |
