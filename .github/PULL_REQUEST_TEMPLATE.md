## Summary

<!-- What does this change and why? Link the issue: "Closes #123". -->

## Screenshots / recordings

<!-- Required for visual changes. Use tools/screenshots/capture.ps1 for consistent framing. -->

## Checklist

- [ ] Builds with `windows-release` (or `linux-release`) and `ctest` passes
- [ ] `all_qmllint` is clean (zero warnings)
- [ ] `pre-commit run --all-files` is clean (clang-format, tokens, SPDX headers)
- [ ] Visual changes checked in light and dark, and with Reduce Motion / Reduce Transparency
- [ ] Performance: no new continuous rendering when idle; `--benchmark` p95 not regressed for shell changes
- [ ] Docs updated (README, docs/, ADR for architectural decisions, CHANGELOG)
