# sim — simulated system services

Implements every contract in `services/interfaces/` with realistic behaviour,
so the whole shell runs without device hardware.

| Path | Contents |
|------|----------|
| `src/simdevices.*` | `SimNetwork` (connect delay, radio toggle), `SimBluetooth`, `SimPower` (real-time drain and charge, low-battery signals), `SimAudio`, `SimDisplay` |
| `src/simsession.*` | `SimMedia` (queue, real-time position), `SimNotifications` (scenario timeline), `SimSettings` (QSettings persistence unless ephemeral) |
| `src/simservices.*` | `SimServices`: loads a scenario, hosts all sources on a `QRemoteObjectHost`; `SimControlSource` |
| `scenarios/default.json` | Built-in scenario (embedded as `:/qor/sim/scenarios/default.json`) |
| `simd/main.cpp` | `qor-simd`: the services in their own process (`--url`, `--scenario`, `--ephemeral`) |

Scenario format: [docs/SERVICES.md#scenarios](../docs/SERVICES.md#scenarios).
