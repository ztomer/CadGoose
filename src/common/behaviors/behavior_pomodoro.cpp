// ===========================
// behavior_pomodoro.cpp
// Pomodoro Timer Behavior
// Work for 25 minutes, break for 5, long break after 4 sessions
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cmath>

static bool s_enabled = true;

extern void Audio_PlayHonk();

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(ctx.goose->id, "pomodoro");
    state->Reset();
    state->phaseStartTime = ctx.time;
}

static double GetPhaseDuration(PomodoroPhase phase) {
    const auto& cfg = g_config.behaviors.pomodoro;
    switch (phase) {
        case PomodoroPhase::Work: return cfg.workMinutes * 60.0;
        case PomodoroPhase::Break: return cfg.breakMinutes * 60.0;
        case PomodoroPhase::LongBreak: return cfg.longBreakMinutes * 60.0;
    }
    return 25 * 60.0;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.systems.pomodoro) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(goose->id, "pomodoro");
    const auto& cfg = g_config.behaviors.pomodoro;

    double elapsed = time - state->phaseStartTime;
    double phaseDuration = GetPhaseDuration(state->phase);

    if (elapsed >= phaseDuration) {
        if (state->phase == PomodoroPhase::Work) {
            state->completedSessions++;
            if (state->completedSessions >= cfg.sessionsBeforeLongBreak) {
                state->phase = PomodoroPhase::LongBreak;
                state->completedSessions = 0;
                state->isAggressive = false;
            } else {
                state->phase = PomodoroPhase::Break;
                state->isAggressive = false;
            }
        } else {
            state->phase = PomodoroPhase::Work;
            state->isAggressive = cfg.enableAggressiveMode;
        }
        state->phaseStartTime = time;
        elapsed = 0;
    }

    if (state->phase == PomodoroPhase::Work && state->isAggressive) {
        // Work phase: goose spins, honks, runs
        float rotationAmount = 90.0f * (float)dt;
        goose->dir += rotationAmount;
        state->accumulatedRotation += rotationAmount;

        if (time - state->lastHonkTime >= cfg.aggressiveHonkInterval) {
            Audio_PlayHonk();
            state->lastHonkTime = time;
        }

        if (!state->speedMultiplierApplied) {
            goose->currentSpeed = g_config.movement.baseRunSpeed * cfg.aggressiveSpeedMultiplier;
            state->speedMultiplierApplied = true;
        }
    } else if (state->phase == PomodoroPhase::Break || state->phase == PomodoroPhase::LongBreak) {
        // Break/LongBreak: goose rests in place
        goose->target = goose->pos;
        goose->vel = {0, 0};
        if (state->accumulatedRotation > 0) {
            goose->dir -= state->accumulatedRotation;
            state->accumulatedRotation = 0;
        }
        if (state->speedMultiplierApplied) {
            goose->currentSpeed = 0.0f;
            state->speedMultiplierApplied = false;
        }
    } else {
        // Non-aggressive work: normal walking
        if (state->accumulatedRotation > 0) {
            goose->dir -= state->accumulatedRotation;
            state->accumulatedRotation = 0;
        }
        if (state->speedMultiplierApplied) {
            goose->currentSpeed = g_config.movement.baseWalkSpeed;
            state->speedMultiplierApplied = false;
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.systems.pomodoro) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(goose->id, "pomodoro");
    if (!state) return;

    double elapsed = ctx.time - state->phaseStartTime;
    double remaining = GetPhaseDuration(state->phase) - elapsed;
    int minutes = (int)(remaining / 60.0);
    int seconds = (int)fmod(remaining, 60.0);

    const char* phaseLabel = "W";
    float bgR = 0.3f, bgG = 0.3f, bgB = 0.3f;

    switch (state->phase) {
        case PomodoroPhase::Work:
            phaseLabel = state->isAggressive ? "ATK!" : "W";
            bgR = 0.6f; bgG = 0.2f; bgB = 0.2f;
            break;
        case PomodoroPhase::Break:
            phaseLabel = "B";
            bgR = 0.2f; bgG = 0.5f; bgB = 0.2f;
            break;
        case PomodoroPhase::LongBreak:
            phaseLabel = "LB";
            bgR = 0.2f; bgG = 0.3f; bgB = 0.6f;
            break;
    }

    char timerText[32];
    snprintf(timerText, sizeof(timerText), "%s %02d:%02d", phaseLabel, minutes, seconds);

    Vector2 headPos = WorldCoord::RigNeckHead(*goose);
    float textWidth = 80.0f;
    float textHeight = 20.0f;

    CGContextSaveGState(cg);
    CGContextSetRGBFillColor(cg, bgR, bgG, bgB, 0.85f);
    CGContextFillRect(cg, CGRectMake(headPos.x - textWidth/2, headPos.y - 60.0f, textWidth, textHeight));

    CTFontRef font = CTFontCreateWithName(CFSTR("Helvetica-Bold"), 11.0f, NULL);
    if (font) {
        CGColorRef white = CGColorCreateGenericRGB(1.0f, 1.0f, 1.0f, 1.0f);
        CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        CFTypeRef values[] = { font, white };
        CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)timerText, strlen(timerText), kCFStringEncodingUTF8, false);
        CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
        CTLineRef line = CTLineCreateWithAttributedString(attrStr);

        if (line) {
            float textX = headPos.x - textWidth/2 + 5.0f;
            float textY = headPos.y - 48.0f;
            CGContextSaveGState(cg);
            CGContextTranslateCTM(cg, textX, textY);
            CGContextScaleCTM(cg, 1.0, -1.0);
            CGContextSetTextPosition(cg, 0, 0);
            CTLineDraw(line, cg);
            CGContextRestoreGState(cg);
            CFRelease(line);
        }

        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attributes);
        CGColorRelease(white);
        CFRelease(font);
    }

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_pomodoroBehavior = {
    .id = "pomodoro",
    .name = "Pomodoro",
    .description = "Work/rest timer: 25 min work, 5 min break, 15 min long break after 4 sessions",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 10,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_pomodoroBehavior);
