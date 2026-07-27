# Plan — Future Work

All items from the previous plan completed in v1.63 (June 26, 2026).

## Next Items (Tentative)

### Phase 6 Coverage: AI Mock Layer
- ✓ **DONE**: Created `include/i_local_llm.h`, `include/i_ai_text_meme.h`, `include/i_ai_http_client.h` abstract interfaces
- ✓ **DONE**: Created `src/common/ai_local_llm_mock.cpp`, `ai_text_meme_mock.cpp`, `ai_http_client_mock.cpp` mock implementations
- ✓ **DONE**: 15 tests in `tests/common/test_ai_interfaces.cpp` covering all mock paths
- Target: push AI-related .cpp files to ≥95% line coverage

### Phase 7 Coverage: Thin Wrappers
- ✓ **DONE**: Created `include/i_window_system.h` abstract interface with `WindowSpec`, `CreateWindow`, `DestroyWindow`, `UpdatePosition`, `SetVisible`, `RequestRedraw`
- ✓ **DONE**: Created `src/common/window_system_mock.cpp` mock implementation
- ✓ **DONE**: 8 tests in `tests/common/test_window_interfaces.cpp`

### Phase 8: CI Gate Hardening
- ✓ **DONE**: Updated `coverage_eligible.txt` with P0/P1 tier labels
- ✓ **DONE**: Rewrote `check_coverage.sh` with multi-metric support (P0 ≥95%, P1 ≥80%, total ≥90%)
- ✓ **DONE**: Added Coverage Gate step to `.github/workflows/build_and_release.yml`

### Notarization
- Developer ID signing + hardened runtime + `com.apple.security.cs.allow-jit` entitlement
- Notarytool submission + staple
- Remove `xattr` step from install docs

### CLI `--version` flag
- ✓ **DONE** (commit 4ce4b4a): `CadGoose --version` prints git-describe at configure time.