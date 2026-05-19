# Rendering Bugs — Open Items

Last updated: 2026-05-19.

The 2026-05-18 batch of rendering regressions is fully fixed; see
[RENDERING_CHANGELOG.md](RENDERING_CHANGELOG.md) for the per-phase
breakdown.

| # | Symptom | Root cause | Severity |
|--:|---|---|:--:|
| 12 | Pomodoro: bed sprite still not visible. (Phase 3 fixed bed *position* — `g_world.screenWidth/Height` now propagate correctly — but no bed sprite renders at that position.) | Suspect: bed comes from `EffectTypePomodoroBed`, registered in `effect_reg_pomodorobed.mm` and drawn in `effect_window.mm::drawRect`. Likely either (a) `s_bedImage` is null because the asset path is wrong, (b) `Pomodoro_GetBedInfo` returns `visible=false` because `isSleeping` never flips true, or (c) the `EffectWindow` ends up at a screen-edge corner where the window can't actually display (off-screen). | High |
| 13 | When the pomodoro behavior is **disabled**, the goose stops moving. | Suspect: `behavior_pomodoro::tick` does work even when disabled, OR something the behavior was doing (target overrides, isResting, isSleeping) doesn't get unwound when the behavior gets disabled mid-run. Most likely candidate: when isSleeping was true, the behavior set `goose->isResting = true` and `target = pos`, then on disable the behavior stops running but `isResting` stays stuck. | High |
