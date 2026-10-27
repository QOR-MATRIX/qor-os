# tools

| Path | Purpose |
|------|---------|
| `qorctl/` | `qorctl`, a CLI client of the system services ([docs/SERVICES.md#qorctl](../docs/SERVICES.md#qorctl)) |
| `tokens/generate_tokens.py` | Lumen token generator → `kit/Tokens.qml`, `platform/common/lumentokens.h`, `docs/design/tokens.md`. `--check` fails if stale |
| `icons/import_lucide.py` | Imports `kit/icons/icons.txt` from a `lucide-static` package, recoloured for GPU tinting |
| `screenshots/capture.ps1` | Deterministic screenshots of named scenes into `docs/images` (WebP) |
| `dev/Enter-QorShell.ps1` | Loads MSVC, Qt, CMake and Ninja into the current PowerShell |
| `dev/build.ps1` | Configure, build, optionally test or build a target with a preset |
| `dev/run.ps1` | Runs `qor-shell` with arguments |
| `ci/check_headers.py` | Verifies SPDX headers |
| `ci/install_qt.py` | Unattended commercial Qt install for CI |
| `github/bootstrap.py` | Applies repository settings, labels, milestones and (optionally) the branch ruleset |
