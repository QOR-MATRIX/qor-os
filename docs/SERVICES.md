# System services

QTOS system services are defined as **Qt Remote Objects contracts** in
[`services/interfaces/`](../services/interfaces/). The shell, apps,
`qtosctl` and tests use *replicas* of these contracts. Simulated *sources*
live in [`sim/`](../sim/); on a device, the `qtos-*d` daemons implement the
same contracts.

Property modes used in the contracts:

| Mode | Source | Replica |
|------|--------|---------|
| `READWRITE` | getter + setter | getter + setter (writes are sent to the source) |
| `SOURCEONLYSETTER` | getter + setter | read-only |

## Contracts

### `Network` — `network.rep` (device: qtos-netd over NetworkManagerQt)

| Member | Type | Mode / kind | Meaning |
|--------|------|-------------|---------|
| `wifiEnabled` | bool | READWRITE | Wi-Fi radio on |
| `online` | bool | source-only | Connected with a route |
| `activeSsid` | QString | source-only | Current network, empty if none |
| `signalBars` | int | source-only | 0 to 3 |
| `accessPoints` | QVariantList | source-only | `[{ssid, bars, secure}]` |
| `connecting` | bool | source-only | Association in progress |
| `connectTo(ssid)` | slot | | Join a network |
| `disconnectNetwork()` | slot | | Leave the current network |
| `connectionFailed(ssid, reason)` | signal | | Join failed |

### `Bluetooth` — `bluetooth.rep` (device: qtos-btd over BluezQt)

| Member | Type | Mode / kind | Meaning |
|--------|------|-------------|---------|
| `enabled` | bool | READWRITE | Adapter powered |
| `devices` | QVariantList | source-only | `[{name, kind, connected, battery}]`. `kind`: headphones, keyboard, mouse or speaker. `battery` is -1 if unknown |
| `toggleDevice(name)` | slot | | Connect or disconnect a paired device |

### `Power` — `power.rep` (device: qtos-powerd over UPower and logind)

| Member | Type | Mode / kind | Meaning |
|--------|------|-------------|---------|
| `batteryLevel` | int | source-only | 0 to 100 |
| `charging` | bool | source-only | |
| `onBattery` | bool | source-only | |
| `minutesRemaining` | int | source-only | To empty, or to full while charging; -1 if unknown |
| `profile` | QString | READWRITE | `power-saver`, `balanced` or `performance` |
| `suspend()` | slot | | |
| `batteryLow(level)` | signal | | At 20% and 10% |

### `Audio` — `audio.rep` (device: qtos-audiod over PipeWire)

| Member | Type | Mode / kind | Meaning |
|--------|------|-------------|---------|
| `volume` | double | READWRITE | 0.0 to 1.0 |
| `muted` | bool | READWRITE | |
| `outputDevice` | QString | source-only | |
| `outputDevices` | QStringList | source-only | |
| `selectOutput(device)` | slot | | |

### `Display` — `display.rep` (owned by Prism, persisted by settingsd)

| Member | Type | Mode | Meaning |
|--------|------|------|---------|
| `brightness` | double | READWRITE | 0.0 to 1.0 |
| `nightLight` | bool | READWRITE | |
| `nightLightTemperature` | int | READWRITE | Kelvin |

### `Media` — `media.rep` (device: aggregated MPRIS sessions)

| Member | Type | Mode / kind | Meaning |
|--------|------|-------------|---------|
| `available` | bool | source-only | A session exists |
| `title`, `artist`, `album`, `player` | QString | source-only | |
| `artworkHue` | double | source-only | OKLCH hue used for generated artwork |
| `playing` | bool | READWRITE | |
| `position`, `duration` | double | source-only | Seconds |
| `queue` | QVariantList | source-only | `[{title, artist, album, duration, hue}]` |
| `queueIndex` | int | source-only | |
| `togglePlayback()`, `next()`, `previous()`, `seek(s)`, `playIndex(i)` | slots | | `previous()` restarts the track if more than 3 s in |

### `Notifications` — `notifications.rep` (device: qtos-notifyd, also serves `org.freedesktop.Notifications`)

| Member | Type | Mode / kind | Meaning |
|--------|------|-------------|---------|
| `doNotDisturb` | bool | READWRITE | Focus: banners are suppressed, history is kept |
| `unreadCount` | int | source-only | |
| `notify(appId, title, body, icon)` | slot | | `icon` is a Lumen icon name |
| `dismiss(id)`, `markAllRead()` | slots | | |
| `posted(id, appId, title, body, icon)` | signal | | |
| `dismissed(id)` | signal | | |

### `Settings` — `settings.rep` (device: qtos-settingsd)

| Member | Type | Mode | Meaning |
|--------|------|------|---------|
| `darkMode` | bool | READWRITE | Appearance |
| `accentHue` | double | READWRITE | OKLCH degrees |
| `reduceMotion`, `reduceTransparency`, `livingWallpaper` | bool | READWRITE | |
| `userName`, `hostName` | QString | source-only | |

### `SimControl` — `simcontrol.rep` (simulation only)

| Member | Kind | Meaning |
|--------|------|---------|
| `scenario` | property | Loaded scenario name |
| `setBattery(level, charging)` | slot | |
| `setSignal(bars)` | slot | |
| `postNotification(appId, title, body)` | slot | |
| `replayScenario()` | slot | Re-schedule the scenario's notifications |

## Using services from QML

```qml
import QTOS.Services

Switch {
    checked: Services.network.wifiEnabled
    onToggled: Services.network.wifiEnabled = checked    // READWRITE: sent to the source
}
Text { text: Services.power.batteryLevel + "%" }
Button { onClicked: Services.media.togglePlayback() }    // remote slot
Connections {
    target: Services.notifications
    function onPosted(id: int, appId: string, title: string, body: string, icon: string) { … }
}
// Ask the system (the shell) to do something:
Button { onClicked: Intents.openApp("settings", { page: "display" }) }
```

`Intents` actions: `app.open {id, page?, folder?, select?}`, `session.lock`,
`shell.overview`, `shell.beam`, `shell.space {index}`.

## Endpoints

| Platform | Default URL |
|----------|-------------|
| Windows, macOS | `local:qtos-services` |
| Linux | `localabstract:qtos-services` |

The embedded host (`qtos-shell`) and `qtos-simd` fall back to
`…-<pid>` if the default is taken. Override with `--services-url` /
`--url`.

## Scenarios

The simulator loads [`sim/scenarios/default.json`](../sim/scenarios/default.json)
(embedded in the binary) or a file passed with `--scenario`. Keys:

| Key | Content |
|-----|---------|
| `name`, `description` | Metadata |
| `user` | `{name, host}` → Settings `userName`, `hostName` |
| `network` | `{ssid, accessPoints: [{ssid, bars, secure}]}` |
| `bluetooth` | `{devices: [{name, kind, connected, battery}]}` |
| `power` | `{batteryLevel, charging, secondsPerPercent}` |
| `audio` | `{volume, devices: [..], output}` |
| `media` | `{player, playing, index, position, queue: [{title, artist, album, duration, hue}]}` |
| `notifications` | `[{atMs, app, title, body, icon}]`, posted relative to start |

## qtosctl

```text
qtosctl [--url <url>] <command> [args]

status                         Print system state
theme light|dark               Set the appearance
accent <hue>                   Set the accent hue (0-360)
motion on|off                  Reduce motion
wifi on|off                    Toggle Wi-Fi
connect <ssid>                 Join a Wi-Fi network
volume <0-100>                 Set output volume
media play|pause|next|prev     Control the media session
dnd on|off                     Do Not Disturb
notify <title> [body] [--app id]   Post a notification
watch                          Stream notifications, appearance and media changes
battery <0-100> [charging]     (simulation) Set the battery level
scenario replay                (simulation) Replay scenario notifications
```
