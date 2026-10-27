# platform — Lumen for every Qt app

| Path | Output | Purpose |
|------|--------|---------|
| `common/` | `qor_lumen_common` | Palette and fonts from generated tokens (`lumentokens.h`); Qt Gui only |
| `style/` | `plugins/styles/lumenstyle` | **Lumen QStyle** for Qt Widgets (proxy over Fusion): rounded controls, accent default buttons, focus rings, thin scroll bars, sliders |
| `theme/` | `plugins/platformthemes/qorplatformtheme` | **`qor` QPA platform theme**: palette, fonts, style names (Lumen, Fusion), colour scheme, UI hints |
| `gallery/` | `lumen-widgets-gallery` | Preview of the QStyle (`--light`, `--accent`, `--screenshot`, `--fonts`) |

Try the QStyle on any Qt Widgets app:

```powershell
$env:QT_PLUGIN_PATH = "$PWD\build\windows-release\plugins"
some-widgets-app.exe -style Lumen
```

On a device the platform theme is loaded for every app with
`QT_QPA_PLATFORMTHEME=qor`. Appearance comes from `QOR_APPEARANCE`
(`light`/`dark`), `QOR_ACCENT_HUE` and `QOR_CONTRAST` (`standard`/`high`)
until it is wired to `qor-settingsd` (Phase 3).
