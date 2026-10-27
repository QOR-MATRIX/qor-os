# Qt in QOR OS: module and tool map

QOR OS is built entirely with Qt. This is the authoritative list of every Qt
module, add-on and tool in the Qt 6.12 Enterprise installation, with its role
in QOR OS and when it is adopted. "In use" covers the OS and the products
carried from demiurge-chain (QQ, ARQADE, QOR Media), named where only they use
a module. Status:

- **In use**: compiled and exercised in the current tree.
- **Phase N**: scheduled for that roadmap phase ([ROADMAP.md](ROADMAP.md)).
- **Evaluate**: promising, needs an ADR before adoption.
- **Not used**: superseded or out of scope (with the reason).

## Essentials

| Module | Role in QOR OS | Status |
|--------|--------------|--------|
| Qt Core | Event loop, objects, properties, `QSettings`, JSON, logging categories | **In use** |
| Qt Gui | `QColor` (OKLCH math), fonts, QRhi graphics abstraction, platform plugins | **In use** |
| Qt Qml | QML engine, `QML_ELEMENT`/`QML_SINGLETON`/`QML_FOREIGN` registration | **In use** |
| Qt Qml Core (`QtCore` QML) | `Settings` for Notes persistence | **In use** |
| Qt Quick | The entire shell and apps; scene graph; `ShaderEffect`, `FrameAnimation`, pointer handlers | **In use** |
| Qt Quick Controls + Templates | The **Lumen** style (24 controls on `QtQuick.Templates`) | **In use** |
| Qt Quick Effects | `MultiEffect` blur for the shared `Backdrop` | **In use** |
| Qt Quick Shapes | The equaliser's response curve in Settings; will host vector icons from Qt VectorImage | **In use** |
| Qt Quick Layouts | Linked; available for app layouts | **In use** (available) |
| Qt Widgets | Lumen `QStyle` and the widgets gallery | **In use** |
| Qt Test / Qt Quick Test | C++ and QML test suites | **In use** |
| Qt Network | Underlies Qt Remote Objects local sockets; the products' launcher links, ARQADE's server, QQ's MCP pipe; online accounts later | **In use** |
| Qt Concurrent | Scanning the music library in the background; the indexer (`qor-indexd`) later | **In use** |
| Qt Sql | SQLite FTS5 index for Beam file search | Phase 3 |
| Qt Svg | Rasterises Lucide icons for GPU tinting | **In use** |
| Qt OpenGL / OpenGL Widgets | Only through QRhi's OpenGL backend; no direct use | Not used directly |
| Qt Print Support | Printing via portals and CUPS | Phase 3 |
| Qt DBus | freedesktop interop: Notifications, MPRIS, portals, Secret Service | Phase 3 |
| Qt Xml | Only through other modules (repc output, SVG) | Not used directly |

## Add-ons

| Module | Role in QOR OS | Status |
|--------|--------------|--------|
| **Qt Shader Tools** | `qt_add_shaders`: Lumen SDF, glass, icon, mask and Aurora shaders → SPIR-V, GLSL, HLSL, MSL | **In use** |
| **Qt Remote Objects** | IPC for every system service; `repc` codegen; replicas typed for QML | **In use** |
| **Qt Graphs** | Monitor app charts (`GraphsView`, `AreaSeries`, `SplineSeries`) | **In use** |
| **Qt Image Formats** | WebP screenshots and documentation images | **In use** |
| **Qt Wayland Compositor** | `prism`: Wayland compositor hosting the shell (Linux) | **In use** (experimental) |
| Qt Wayland (client) | Running QOR OS apps inside Prism | Phase 1 |
| **Qt Multimedia** | The [audio engine](AUDIO.md): real-time output (`QAudioSink` callback interface), music decoding (`QAudioDecoder`, FFmpeg), device tracking (`QMediaDevices`), interface sounds (`SoundEffect`); QOR Media's player; QQ's music and sound. Later: Camera, screen recording in Prism, video Quick Look | **In use** |
| **Qt Quick 3D** (with Asset Utils, Helpers, Particles) | QQ's worlds, glTF models and particles; ARQADE's home and games in 3D. Later: Spatial Overview, 3D Quick Look, Settings hero | **In use** (QQ, ARQADE) |
| Qt Quick 3D Xr | Spatial mode on OpenXR headsets | Phase 4 |
| **Qt Quick 3D Physics** | QQ: bodies, collisions and the 3D player character (commercial or GPLv3: QOR-0008) | **In use** (QQ) |
| **Qt Spatial Audio** | QQ's sounds placed in a world; positional UI sound design later (VISION §5) | **In use** (QQ) |
| Qt Lottie | Expressive animated symbols | Phase 4 |
| Qt Quick Timeline | Choreographed sequences: onboarding, setup assistant | Phase 4 |
| Qt Quick VectorImage (`svgtoqml`) | Icons compiled to native Qt Quick Shapes instead of rasterised SVG | Phase 4 |
| Qt Canvas Painter (new in 6.12) | GPU 2D painting for a sketch app and custom charts | Evaluate |
| Qt Labs StyleKit (new in 6.12) | Declarative styling engine; possible future home of the Lumen style ([QTOS-0007](decisions/qtos/QTOS-0007-lumen-style-on-quick-templates.md)) | Evaluate |
| Qt Quick Effect Maker | Authoring new Lumen effects visually | Phase 4 |
| Qt Quick Particles | Restrained celebratory moments (setup complete) | Evaluate |
| Qt Virtual Keyboard | Tablet mode, touch lock screen, and IME on touch devices | Phase 4 |
| Qt TextToSpeech | Qt-native screen reader (VISION §11) | Phase 4 |
| Qt WebEngine | Browser app, web content in apps (not yet installed in the dev kit) | Phase 3 |
| Qt WebView / WebChannel / WebSockets | Lightweight web views; JS↔QML bridges; companion device channels | Phase 3–4 |
| **Qt PDF** | QOR Media's reader (with PDFium for changing a PDF); PDF Quick Look later | **In use** (QOR Media) |
| Qt Positioning / Qt Location | `qor-locationd`; Maps app (MapLibre plugin) | Phase 3–4 |
| Qt Sensors | Auto-rotate, ambient light → auto-brightness, lid angle | Phase 3 |
| Qt Bluetooth / Connectivity / NFC | App-level BLE and NFC (system Bluetooth uses BluezQt in `qor-btd`) | Phase 3 |
| Qt Network Authorization | OAuth2 for online accounts (`qor-accountsd`) | Phase 3 |
| Qt gRPC / Qt Protobuf | `qor-assistd` API; companion devices | Phase 4 |
| Qt HTTP Server | Local automation endpoint for Flows and developer tooling | Phase 4 |
| Qt MCP (present in the 6.12 kit) | Exposing QOR OS intents to AI assistants, under user control | Evaluate |
| Qt OpenAPI | Generated clients for cloud services in apps | Evaluate |
| Qt MQTT / CoAP / OPC UA / Serial Bus / Serial Port | Smart-home and IoT triggers for Flows | Phase 4 |
| Qt SCXML / State Machine | Session lifecycle (boot, greeter, lock, sleep) in `qor-sessiond` | Phase 3 |
| Qt Task Tree | Structured async pipelines for updates and indexing | Evaluate |
| Qt Help | In-OS documentation and tips | Phase 4 |
| Qt VNC Server | Remote assistance on devices | Phase 4 |
| Qt 3D, Qt Data Visualization, Qt Charts | Superseded by Qt Quick 3D and Qt Graphs | Not used |
| Qt 5 Compatibility | No Qt 5 code | Not used |
| Active Qt | Windows COM integration, not relevant to the OS | Not used |

## Device and enterprise components

| Component | Role in QOR OS | Status |
|-----------|--------------|--------|
| Qt for Device Creation / Boot2Qt (`meta-boot2qt`) | QTOS's Yocto option; the image is built on Fedora Atomic instead ([QOR-0004](decisions/QOR-0004-the-base-image-is-fedora-atomic.md)) | Not used |
| Qt for WebAssembly (multithreaded kit) | The QQ Player in a browser | **In use** (QQ) |
| Qt Application Manager | App lifecycle, packages, intents, quick-launch on device | Phase 3 |
| Qt Interface Framework (QFace + `ifcodegen`) | Generate service frontends, QtRO backends and simulation backends from one IDL | Evaluate (Phase 1) |
| Qt for MCUs | Companion hardware (remote, wearable) speaking QOR OS services | Phase 4 |
| Qt Safe Renderer | Not applicable to a desktop OS | Not used |

## Tools

| Tool | Role in QOR OS | Status |
|------|--------------|--------|
| CMake integration (`qt_add_qml_module`, `qt_add_shaders`, `qt_add_repc_*`, `qt_generate_deploy_qml_app_script`) | The entire build | **In use** |
| `qmlsc` (Qt Quick Compiler extensions) | AOT compilation of all QML | **In use** |
| `qmllint` | Zero-warning gate (`all_qmllint`) | **In use** |
| `qmlls` | Editor intelligence (`QT_QML_GENERATE_QMLLS_INI`) | **In use** |
| `qmlformat` | Formatting config in `.qmlformat.ini` (advisory) | **In use** (advisory) |
| `repc` | Service contract code generation | **In use** |
| `qsb` | Shader baking (through `qt_add_shaders`) | **In use** |
| `windeployqt` | Deployment through `cmake --install` | **In use** |
| `balsam` / `balsamui` | Importing 3D assets for QQ, Quick Look and Spatial | Phase 4 |
| `lottietoqml`, `svgtoqml` | Converting motion and vector assets to native QML | Phase 4 |
| Qt Linguist (`lupdate`, `lrelease`) | Translations (all strings already use `qsTr`) | Phase 3 |
| Qt Creator (incl. Qt AI Assistant) | Primary IDE; QML profiler; debugger | **In use** |
| Qt Design Studio | Designer workflow against Lumen tokens; Figma bridge | Phase 1 |
| Qt Installer Framework | Developer SDK installer | Phase 4 |
| Squish, Coco, Axivion (Qt Group quality tools) | GUI automation, coverage, architecture analysis, if licensed | Evaluate |
| Qt VS Code extensions | Recommended in `.vscode/extensions.json` | **In use** |

## Third-party Qt-based software

Planned adoptions from the wider Qt ecosystem (VISION §9). These are
licensing-reviewed before inclusion:

| Software | Role |
|----------|------|
| KDE Frameworks (NetworkManagerQt, BluezQt, ModemManagerQt, KFileMetaData, KIO, Kirigami) | Device-side service backends and file metadata |
| Calamares | Installer |
| KDE Connect | Phone continuity |
| libQuotient | Matrix messaging |
| LibreOffice (Qt 6 VCL), Kate, Okular, Krita, Kdenlive, OBS Studio, MuseScore, FreeCAD, KeePassXC, VLC | Curated third-party apps, themed by the `qor` platform theme and Lumen QStyle |
