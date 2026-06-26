# Plan — Future Work

All items from the previous plan completed in v1.63 (June 26, 2026).

## Next Items (Tentative)

### Phase 6 Coverage: AI Mock Layer
- Extract interfaces for `ai_http_client.mm`, `local_llm_model.mm`, `local_llm_inference.mm`, `local_llm_tokenizer.mm`
- Add mock implementations for unit testing
- Target: push AI-related .cpp files to ≥95% line coverage

### Phase 7 Coverage: Thin Wrappers
- Window wrappers: `window.mm`, `tick_manager.mm`, `effect_window.mm`, `item_window.mm`, `behavior_element_window.mm`
- Target: extract platform interfaces, add mocks

### Phase 8: CI Gate Hardening
- Multi-metric threshold: P0 .cpp ≥95%, P1 .mm ≥80%, project total ≥90%
- Enforce in `scripts/check_coverage.sh` and CI

### Notarization
- Developer ID signing + hardened runtime + `com.apple.security.cs.allow-jit` entitlement
- Notarytool submission + staple
- Remove `xattr` step from install docs

### CLI `--version` flag
- Add `CadGoose --version` printing bundle `CFBundleShortVersionString`