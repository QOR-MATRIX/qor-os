# Qt in QTOS: module and tool map

QTOS is built entirely with Qt. This is the authoritative list of every Qt
module, add-on and tool in the Qt 6.12 Enterprise installation, with its role
in QTOS and when it is adopted. Status:

- **In use**: compiled and exercised in the current tree.
- **Phase N**: scheduled for that roadmap phase ([ROADMAP.md](ROADMAP.md)).
- **Evaluate**: promising, needs an ADR before adoption.
- **Not used**: superseded or out of scope (with the reason).

## Essentials

| Module | Role in QTOS | Status |
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
| Qt Network | Underlies Qt Remote Objects local sockets; online accounts later | **In use** |
| Qt Concurrent | Scanning the music library in the background; the indexer (`qtos-indexd`) later | **In use** |
| Qt Sql | SQLite FTS5 index for Beam file search | Phase 3 |
| Qt Svg | Rasterises Lucide icons for GPU tinting | **In use** |
| Qt OpenGL / OpenGL Widgets | Only through QRhi's OpenGL backend; no direct use | Not used directly |
| Qt Print Support | Printing via portals and CUPS | Phase 3 |
| Qt DBus | freedesktop interop: Notifications, MPRIS, portals, Secret Service | Phase 3 |
| Qt Xml | Only through other modules (repc output, SVG) | Not used directly |

## Add-ons

| Module | Role in QTOS | Status |
|--------|--------------|--------|
| **Qt Shader Tools** | `qt_add_shaders`: Lumen SDF, glass, icon, mask and Aurora shaders → SPIR-V, GLSL, HLSL, MSL | **In use** |
| **Qt Remote Objects** | IPC for every system service; `repc` codegen; replicas typed for QML | **In use** |
| **Qt Graphs** | Monitor app charts (`GraphsView`, `AreaSeries`, `SplineSeries`) | **In use** |
| **Qt Image Formats** | WebP screenshots and documentation images | **In use** |
| **Qt Wayland Compositor** | `prism`: Wayland compositor hosting the shell (Linux) | **In use** (experimental) |
| Qt Wayland (client) | Running QTOS apps inside Prism | Phase 1 |
| **Qt Multimedia** | The [audio engine](AUDIO.md): real-time output (`QAudioSink` callback interface), music decoding (`QAudioDecoder`, FFmpeg), device tracking (`QMediaDevices`), interface sounds (`SoundEffect`). Later: Camera, screen recording in Prism, video Quick Look | **In use** |
| Qt Quick 3D | Spatial Overview, 3D Quick Look (with Asset Utils), Settings hero | Phase 4 |
| Qt Quick 3D Xr | Spatial mode on OpenXR headsets | Phase 5 |
| Qt Quick 3D Physics | Not planned for the shell | Not used |
| Qt Spatial Audio | Positional UI sound design (VISION §5) | Phase 4 |
| Qt Lottie | Expressive animated symbols | Phase 4 |
| Qt Quick Timeline | Choreographed sequences: onboarding, setup assistant | Phase 4 |
| Qt Quick VectorImage (`svgtoqml`) | Icons compiled to native Qt Quick Shapes instead of rasterised SVG | Phase 4 |
| Qt Canvas Painter (new in 6.12) | GPU 2D painting for a sketch app and custom charts | Evaluate |
| Qt Labs StyleKit (new in 6.12) | Declarative styling engine; possible future home of the Lumen style ([ADR-0007](adr/0007-lumen-style-on-quick-templates.md)) | Evaluate |
| Qt Quick Effect Maker | Authoring new Lumen effects visually | Phase 4 |
| Qt Quick Particles | Restrained celebratory moments (setup complete) | Evaluate |
| Qt Virtual Keyboard | Tablet mode, touch lock screen, and IME on touch devices | Phase 4 |
| Qt TextToSpeech | Qt-native screen reader (VISION §11) | Phase 4 |
| Qt WebEngine | Browser app, web content in apps (not yet installed in the dev kit) | Phase 2 |
| Qt WebView / WebChannel / WebSockets | Lightweight web views; JS↔QML bridges; companion device channels | Phase 3–5 |
| Qt PDF | PDF Quick Look and a document viewer | Phase 4 |
| Qt Positioning / Qt Location | `qtos-locationd`; Maps app (MapLibre plugin) | Phase 3–5 |
| Qt Sensors | Auto-rotate, ambient light → auto-brightness, lid angle | Phase 2 |
| Qt Bluetooth / Connectivity / NFC | App-level BLE and NFC (system Bluetooth uses BluezQt in `qtos-btd`) | Phase 3 |
| Qt Network Authorization | OAuth2 for online accounts (`qtos-accountsd`) | Phase 3 |
| Qt gRPC / Qt Protobuf | `qtos-assistd` API; companion devices | Phase 5 |
| Qt HTTP Server | Local automation endpoint for Flows and developer tooling | Phase 5 |
| Qt MCP (present in the 6.12 kit) | Exposing QTOS intents to AI assistants, under user control | Evaluate |
| Qt OpenAPI | Generated clients for cloud services in apps | Evaluate |
| Qt MQTT / CoAP / OPC UA / Serial Bus / Serial Port | Smart-home and IoT triggers for Flows | Phase 5 |
| Qt SCXML / State Machine | Session lifecycle (boot, greeter, lock, sleep) in `qtos-sessiond` | Phase 3 |
| Qt Task Tree | Structured async pipelines for updates and indexing | Evaluate |
| Qt Help | In-OS documentation and tips | Phase 5 |
| Qt VNC Server | Remote assistance on devices | Phase 5 |
| Qt 3D, Qt Data Visualization, Qt Charts | Superseded by Qt Quick 3D and Qt Graphs | Not used |
| Qt 5 Compatibility | No Qt 5 code | Not used |
| Active Qt | Windows COM integration, not relevant to the OS | Not used |

## Device and enterprise components

| Component | Role in QTOS | Status |
|-----------|--------------|--------|
| Qt for Device Creation / Boot2Qt (`meta-boot2qt`) | Commercial Yocto base for the QTOS image | Phase 2 |
| Qt Application Manager | App lifecycle, packages, intents, quick-launch on device | Phase 3 |
| Qt Interface Framework (QFace + `ifcodegen`) | Generate service frontends, QtRO backends and simulation backends from one IDL | Evaluate (Phase 1) |
| Qt for MCUs | Companion hardware (remote, wearable) speaking QTOS services | Phase 5 |
| Qt Safe Renderer | Not applicable to a desktop OS | Not used |

## Tools

| Tool | Role in QTOS | Status |
|------|--------------|--------|
| CMake integration (`qt_add_qml_module`, `qt_add_shaders`, `qt_add_repc_*`, `qt_generate_deploy_qml_app_script`) | The entire build | **In use** |
| `qmlsc` (Qt Quick Compiler extensions) | AOT compilation of all QML | **In use** |
| `qmllint` | Zero-warning gate (`all_qmllint`) | **In use** |
| `qmlls` | Editor intelligence (`QT_QML_GENERATE_QMLLS_INI`) | **In use** |
| `qmlformat` | Formatting config in `.qmlformat.ini` (advisory) | **In use** (advisory) |
| `repc` | Service contract code generation | **In use** |
| `qsb` | Shader baking (through `qt_add_shaders`) | **In use** |
| `windeployqt` | Deployment through `cmake --install` | **In use** |
| `balsam` / `balsamui` | Importing 3D assets for Quick Look and Spatial | Phase 4 |
| `lottietoqml`, `svgtoqml` | Converting motion and vector assets to native QML | Phase 4 |
| Qt Linguist (`lupdate`, `lrelease`) | Translations (all strings already use `qsTr`) | Phase 3 |
| Qt Creator (incl. Qt AI Assistant) | Primary IDE; QML profiler; debugger | **In use** |
| Qt Design Studio | Designer workflow against Lumen tokens; Figma bridge | Phase 1 |
| Qt Installer Framework | Developer SDK installer | Phase 5 |
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
| LibreOffice (Qt 6 VCL), Kate, Okular, Krita, Kdenlive, OBS Studio, MuseScore, FreeCAD, KeePassXC, VLC | Curated third-party apps, themed by the `qtos` platform theme and Lumen QStyle |
