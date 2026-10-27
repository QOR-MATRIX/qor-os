# sdk — building apps for QOR OS

> **Status:** template and guidance. An installable SDK (QOR.Kit, Lumen and
> QOR.Services as an exported CMake package plus a simulator) is a Phase 4
> deliverable.

## Template

[`templates/qor-app/`](templates/qor-app/) is a minimal app: Lumen
controls, `Theme` and `Tokens`, a `Spring`, and an `Intent`, plus a Qt
Application Manager manifest (`info.yaml`) for device packaging.

### Building it today

Until the SDK package exists, build apps **inside this repository**: add the
template as a subdirectory, link the QOR OS modules, and import their plugins:

```cmake
# CMakeLists.txt (in-tree)
add_subdirectory(sdk/templates/qor-app)
target_link_libraries(hello-lumen PRIVATE qor_kit qor_kitplugin qor_lumen_styleplugin
                                          qor_services qor_servicesplugin)
```

```cpp
Q_IMPORT_QML_PLUGIN(QOR_KitPlugin)
Q_IMPORT_QML_PLUGIN(LumenPlugin)
Q_IMPORT_QML_PLUGIN(QOR_ServicesPlugin)
```

To appear in the shell today, add the app to `QOR.Apps` instead
([DEVELOPMENT.md → Add an app](../docs/DEVELOPMENT.md#add-an-app)).

## Themes and widgets

Themes and desktop widgets ship through the QOR OS Store as signed packages.
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
