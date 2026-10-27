# sdk — building apps for QTOS

> **Status:** template and guidance. An installable SDK (QTOS.Kit, Lumen and
> QTOS.Services as an exported CMake package plus a simulator) is a Phase 4
> deliverable.

## Template

[`templates/qtos-app/`](templates/qtos-app/) is a minimal app: Lumen
controls, `Theme` and `Tokens`, a `Spring`, and an `Intent`, plus a Qt
Application Manager manifest (`info.yaml`) for device packaging.

### Building it today

Until the SDK package exists, build apps **inside this repository**: add the
template as a subdirectory, link the QTOS modules, and import their plugins:

```cmake
# CMakeLists.txt (in-tree)
add_subdirectory(sdk/templates/qtos-app)
target_link_libraries(hello-lumen PRIVATE qtos_kit qtos_kitplugin qtos_lumen_styleplugin
                                          qtos_services qtos_servicesplugin)
```

```cpp
Q_IMPORT_QML_PLUGIN(QTOS_KitPlugin)
Q_IMPORT_QML_PLUGIN(LumenPlugin)
Q_IMPORT_QML_PLUGIN(QTOS_ServicesPlugin)
```

To appear in the shell today, add the app to `QTOS.Apps` instead
([DEVELOPMENT.md → Add an app](../docs/DEVELOPMENT.md#add-an-app)).

## Themes and widgets

Themes and desktop widgets ship through the QTOS Store as signed packages.
The formats, the widget contract and the signing tool are in
[docs/STORE.md](../docs/STORE.md). Until the developer program opens, the
Store installs only QOR-MATRIX's own.

## Guidelines

- Use Lumen controls (`import QtQuick.Controls`), Kit components, `Theme`
  roles and `Tokens`. No hard-coded colours, radii or durations.
- Reach the system only through `Services` (state) and `Intents` (actions).
- Respect Reduce Motion, Reduce Transparency, keyboard focus and
  accessibility roles.
- Render on demand. See [PERFORMANCE.md](../docs/PERFORMANCE.md).
