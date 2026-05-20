# Rendering Bugs — Open Items

Last updated: 2026-05-19.

The 2026-05-18 batch of rendering regressions is fully fixed; see
[RENDERING_CHANGELOG.md](RENDERING_CHANGELOG.md) for the per-phase
breakdown.

| # | Symptom | Root cause | Severity |
|--:|---|---|:--:|
| 12 | Pomodoro: bed sprite still not visible. (Phase 3 fixed bed *position* — `g_world.screenWidth/Height` now propagate correctly — but no bed sprite renders at that position.) | Suspect: bed comes from `EffectTypePomodoroBed`, registered in `effect_reg_pomodorobed.mm` and drawn in `effect_window.mm::drawRect`. Likely either (a) `s_bedImage` is null because the asset path is wrong, (b) `Pomodoro_GetBedInfo` returns `visible=false` because `isSleeping` never flips true, or (c) the `EffectWindow` ends up at a screen-edge corner where the window can't actually display (off-screen). | High |
| 13 | When the pomodoro behavior is **disabled**, the goose stops moving. | Behaviors only get `tick`'d when their `enabledPtr` is true. Pomodoro's side effect of `goose->isResting = true` (set when the goose reaches the bed) is never unwound when the behavior gets disabled mid-sleep, and the only writer of `isResting` is pomodoro. The goose physics then forces `vel = 0, target = pos` forever. | High |
| 14 | When the **ball** behavior is disabled, the ball window stays on screen. | Same family as #13: behavior tick stops running, but the side effect (the `BallActor` registered with `ActorManager` + its `BehaviorElementWindow`) isn't torn down. No on-disable cleanup hook is wired. | Med |
| 15 | Preferences → Behaviors panel: huge vertical gap between each category header (e.g. "Joy") and the first behavior row under it. | macOS config GUI layout in `config_gui_views.mm` (or similar). Likely the category-section spacer or the row stack inset is doubled. | Low |
| 16 | App holds **~46% CPU** at steady state. | Unknown. Needs profiling — Time Profiler / Activity Monitor / `sample`. | High |
| 17 | App crashes intermittently. | Unknown. Hoping the profiling session in #16 surfaces the trigger; otherwise enable AddressSanitizer or grab the crash report. | High |
