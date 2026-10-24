# tests

| Suite | Path | Framework | Notes |
|-------|------|-----------|-------|
| `tst_spring` | `kit/tst_spring.cpp` | Qt Test | Spring physics; deterministic stepping via `Spring::step` |
| `tst_oklch` | `kit/tst_oklch.cpp` | Qt Test | Color math against reference values |
| `tst_services` | `services/tst_services.cpp` | Qt Test | End-to-end Qt Remote Objects, on a private endpoint |
| `tst_shell` | `shell/tst_search.qml` | Qt Quick Test | Beam search and calculator (pure JS) |
| `tst_kit_qml` | `kit/tst_kit.qml` | Qt Quick Test | Kit in a real window; label `gpu` |
| `tst_golden` | `golden/tst_golden.cpp` | Qt Test | Golden images of Kit and Lumen controls (light, dark, high contrast, RTL), perceptual OKLab diff, one reference set for all platforms; label `gpu`. See [DEVELOPMENT.md](../docs/DEVELOPMENT.md#golden-image-tests) |
| `tools_generate_tokens` | `tools/test_generate_tokens.py` | Python unittest | Generator math, output freshness and WCAG contrast guarantees |

```powershell
ctest --preset windows-release            # everything
ctest --preset windows-release -LE gpu    # without GPU-dependent suites (CI hosted runners)
ctest --preset windows-release -R spring -V
```

All suites set `QT_FORCE_STDERR_LOGGING=1` so QtTest output is visible under
ctest on Windows.
