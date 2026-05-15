// ===========================
// behavior_pomodoro.cpp
// Pomodoro Timer Behavior
// Work for 25 minutes, break for 5, long break after 4 sessions
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cmath>

// --- Timing and layout constants ---
static constexpr float kDefaultWorkMinutes = 25.0f;
static constexpr float kSecondsPerMinute = 60.0f;
static constexpr float kBedMarginX = 150.0f;
static constexpr float kBedMarginY = 100.0f;
static constexpr float kArrivalThreshold = 10.0f;
static constexpr float kSlowRotateInterval = 2.0f;
static constexpr float kSlowRotateAmount = 15.0f;
static constexpr float kZzzAnimInterval = 1.5f;
static constexpr float kZzzAnimFrames = 3;
static constexpr float kManicRotationSpeed = 90.0f;
static constexpr float kTimerTextWidth = 80.0f;
static constexpr float kTimerTextHeight = 20.0f;
static constexpr float kTimerTextYOffset = 60.0f;
static constexpr float kTimerTextDrawYOffset = 48.0f;
static constexpr float kTimerTextXPad = 5.0f;
static constexpr float kBedWidth = 60.0f;
static constexpr float kBedHeight = 25.0f;
static constexpr float kBedInnerPad = 4.0f;
static constexpr float kZzzXOffset = 15.0f;
static constexpr float kZzzBaseYOffset = 80.0f;
static constexpr float kZzzFloatHeight = 15.0f;
static constexpr float kZzzFontsize = 14.0f;
static constexpr float kPomoFontsize = 11.0f;
static constexpr float kBgAlpha = 0.85f;
static constexpr float kFallbackBgR = 0.3f;
static constexpr float kFallbackBgG = 0.3f;
static constexpr float kFallbackBgB = 0.3f;
static constexpr float kWorkBgR = 0.2f;
static constexpr float kWorkBgG = 0.5f;
static constexpr float kWorkBgB = 0.2f;
static constexpr float kBreakBgR = 0.6f;
static constexpr float kBreakBgG = 0.2f;
static constexpr float kBreakBgB = 0.2f;
static constexpr float kBedFallbackR = 0.4f;
static constexpr float kBedFallbackG = 0.3f;
static constexpr float kBedFallbackB = 0.2f;
static constexpr float kBedFallbackAlpha = 0.7f;
static constexpr float kBedInnerR = 0.6f;
static constexpr float kBedInnerG = 0.5f;
static constexpr float kBedInnerB = 0.4f;
static constexpr float kBedInnerAlpha = 0.8f;
static constexpr float kZzzFallbackR = 0.9f;
static constexpr float kZzzFallbackG = 0.9f;
static constexpr float kZzzFallbackB = 1.0f;
static constexpr float kZzzAlphaFadeStart = 0.5f;
static constexpr float kAngleNormalizeHalf = 180.0f;
static constexpr float kAngleFull = 360.0f;
static constexpr float kDirTurnSpeed = 5.0f;
static constexpr float kWalkSpeedMultiplier = 1.0f;

static bool s_enabled = true;
static CGImageRef s_bedImage = nullptr;
static CGImageRef s_zzzImages[3] = {nullptr, nullptr, nullptr};

extern void Audio_PlayHonk();

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(ctx.goose->id, "pomodoro");
    state->Reset();
    state->phaseStartTime = ctx.time;

    if (!s_bedImage) {
        s_bedImage = g_assets.GetBehaviorImage("Assets/Items/Bed/bed.png");
        s_zzzImages[0] = g_assets.GetBehaviorImage("Assets/Items/Bed/z1.png");
        s_zzzImages[1] = g_assets.GetBehaviorImage("Assets/Items/Bed/z2.png");
        s_zzzImages[2] = g_assets.GetBehaviorImage("Assets/Items/Bed/z3.png");
    }
}

static double GetPhaseDuration(PomodoroPhase phase) {
    const auto& cfg = g_config.behaviors.pomodoro;
    switch (phase) {
        case PomodoroPhase::Work: return cfg.workMinutes * kSecondsPerMinute;
        case PomodoroPhase::Break: return cfg.breakMinutes * kSecondsPerMinute;
        case PomodoroPhase::LongBreak: return cfg.longBreakMinutes * kSecondsPerMinute;
    }
    return kDefaultWorkMinutes * kSecondsPerMinute;
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
                state->isAggressive = cfg.enableAggressiveMode;
            } else {
                state->phase = PomodoroPhase::Break;
                state->isAggressive = cfg.enableAggressiveMode;
            }
            state->isSleeping = false;
        } else {
            state->phase = PomodoroPhase::Work;
            state->isAggressive = false;
            state->isSleeping = false;
            state->bedPosition = {
                (float)g_screenWidth - kBedMarginX,
                (float)g_screenHeight - kBedMarginY
            };
        }
        state->phaseStartTime = time;
        elapsed = 0;
    }

    if (state->phase == PomodoroPhase::Work) {
        goose->isResting = true;
        if (goose->state == GooseState::FETCHING || goose->state == GooseState::RETURNING) {
            if (goose->heldItem) {
                delete goose->heldItem;
                goose->heldItem = nullptr;
            }
            goose->state = GooseState::WANDER;
        }
        goose->forceItemFetch = -1;

        if (!state->isSleeping) {
            float bedMargin = kBedMarginX;
            float distToBed = Vector2::Distance(goose->pos, state->bedPosition);
            float arrivalThreshold = kArrivalThreshold;

            if (distToBed > arrivalThreshold) {
                Vector2 toBed = state->bedPosition - goose->pos;
                Vector2 dir = Vector2::Normalize(toBed);
                float walkSpeed = g_config.movement.baseWalkSpeed;
                goose->vel = dir * walkSpeed;
                goose->target = state->bedPosition;

                float targetAngle = std::atan2f(dir.y, dir.x) * RAD_TO_DEG;
                float angleDiff = targetAngle - goose->dir;
                while (angleDiff > kAngleNormalizeHalf) angleDiff -= kAngleFull;
                while (angleDiff < -kAngleNormalizeHalf) angleDiff += kAngleFull;
                goose->dir += angleDiff * kDirTurnSpeed * (float)dt;
            } else {
                state->isSleeping = true;
                goose->vel = {0, 0};
                goose->target = goose->pos;
            }
        } else {
            goose->vel = {0, 0};
            goose->target = goose->pos;

            state->slowRotateTimer += dt;
            if (state->slowRotateTimer > kSlowRotateInterval) {
                state->slowRotateDir = -state->slowRotateDir;
                state->slowRotateTimer = 0;
            }
            float slowRotateAmount = kSlowRotateAmount * (float)dt * (float)state->slowRotateDir;
            goose->dir += slowRotateAmount;
            state->accumulatedRotation += slowRotateAmount;

            state->zzzAnimTime += dt;
            if (state->zzzAnimTime > kZzzAnimInterval) {
                state->zzzAnimTime = 0;
                state->zzzFrame = (state->zzzFrame + 1) % (int)kZzzAnimFrames;
            }
        }

        if (state->accumulatedRotation > 0 && !state->isSleeping) {
            goose->dir -= state->accumulatedRotation;
            state->accumulatedRotation = 0;
        }
        if (state->speedMultiplierApplied) {
            goose->currentSpeed = 0.0f;
            state->speedMultiplierApplied = false;
        }
    } else if (state->isAggressive && (state->phase == PomodoroPhase::Break || state->phase == PomodoroPhase::LongBreak)) {
        // Break/LongBreak: goose goes manic - spins, honks, runs
        float rotationAmount = kManicRotationSpeed * (float)dt;
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
    } else {
        // Non-aggressive break: normal walking
        goose->isResting = false;
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

static CTFontRef s_pomoFont = nullptr;
static CGColorRef s_pomoWhite = nullptr;

static void cleanupPomoFont(BehaviorContext&) {
    if (s_pomoFont) { CFRelease(s_pomoFont); s_pomoFont = nullptr; }
    if (s_pomoWhite) { CGColorRelease(s_pomoWhite); s_pomoWhite = nullptr; }
}

static void ensurePomoFont() {
    if (!s_pomoFont) s_pomoFont = CTFontCreateWithName(CFSTR("Helvetica-Bold"), kPomoFontsize, NULL);
    if (!s_pomoWhite) s_pomoWhite = CGColorCreateGenericRGB(1.0f, 1.0f, 1.0f, 1.0f);
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
    int minutes = (int)(remaining / kSecondsPerMinute);
    int seconds = (int)fmod(remaining, kSecondsPerMinute);

    const char* phaseLabel = "R";
    float bgR = kFallbackBgR, bgG = kFallbackBgG, bgB = kFallbackBgB;

    switch (state->phase) {
        case PomodoroPhase::Work:
            phaseLabel = "R";
            bgR = kWorkBgR; bgG = kWorkBgG; bgB = kWorkBgB;
            break;
        case PomodoroPhase::Break:
            phaseLabel = state->isAggressive ? "ATK!" : "B";
            bgR = kBreakBgR; bgG = kBreakBgG; bgB = kBreakBgB;
            break;
        case PomodoroPhase::LongBreak:
            phaseLabel = state->isAggressive ? "ATK!" : "LB";
            bgR = kBreakBgR; bgG = kBreakBgG; bgB = kBreakBgB;
            break;
    }

    char timerText[32];
    snprintf(timerText, sizeof(timerText), "%s %02d:%02d", phaseLabel, minutes, seconds);

    Vector2 headPos = WorldCoord::RigNeckHead(*goose);
    float textWidth = kTimerTextWidth;
    float textHeight = kTimerTextHeight;

    CGContextSaveGState(cg);
    CGContextSetRGBFillColor(cg, bgR, bgG, bgB, kBgAlpha);
    CGContextFillRect(cg, CGRectMake(headPos.x - textWidth/2, headPos.y - kTimerTextYOffset, textWidth, textHeight));

    ensurePomoFont();
    if (s_pomoFont && s_pomoWhite) {
        CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        CFTypeRef values[] = { s_pomoFont, s_pomoWhite };
        CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)timerText, strlen(timerText), kCFStringEncodingUTF8, false);
        CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
        CTLineRef line = CTLineCreateWithAttributedString(attrStr);

        if (line) {
            float textX = headPos.x - textWidth/2 + kTimerTextXPad;
            float textY = headPos.y - kTimerTextDrawYOffset;
            CGContextSaveGState(cg);
            CGContextSetTextPosition(cg, textX, textY);
            CTLineDraw(line, cg);
            CGContextRestoreGState(cg);
            CFRelease(line);
        }

        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attributes);
    }

    if (state->isSleeping) {
        Vector2 bedPos = WorldCoord::ToDevice(state->bedPosition, *goose);
        float bedWidth = kBedWidth;
        float bedHeight = kBedHeight;

        if (s_bedImage) {
            float imgWidth = (float)CGImageGetWidth(s_bedImage);
            float imgHeight = (float)CGImageGetHeight(s_bedImage);
            CGRect bedRect = CGRectMake(bedPos.x - imgWidth / 2.0f, bedPos.y - imgHeight / 2.0f, imgWidth, imgHeight);
            CGContextDrawImage(cg, bedRect, s_bedImage);
        } else {
            CGContextSetRGBFillColor(cg, kBedFallbackR, kBedFallbackG, kBedFallbackB, kBedFallbackAlpha);
            CGContextFillRect(cg, CGRectMake(bedPos.x - bedWidth/2, bedPos.y - bedHeight/2, bedWidth, bedHeight));

            CGContextSetRGBFillColor(cg, kBedInnerR, kBedInnerG, kBedInnerB, kBedInnerAlpha);
            CGContextFillRect(cg, CGRectMake(bedPos.x - bedWidth/2 + kBedInnerPad, bedPos.y - bedHeight/2 + kBedInnerPad, bedWidth - kBedInnerPad*2, bedHeight - kBedInnerPad*2));
        }

        const char* zzzStr;
        switch (state->zzzFrame) {
            case 0: zzzStr = "Z"; break;
            case 1: zzzStr = "z"; break;
            default: zzzStr = "."; break;
        }

        float zzzX = headPos.x + kZzzXOffset;
        float zzzY = headPos.y - kZzzBaseYOffset - (float)(state->zzzAnimTime / kZzzAnimInterval) * kZzzFloatHeight;
        float zzzAlpha = 1.0f - (float)(state->zzzAnimTime / kZzzAnimInterval) * kZzzAlphaFadeStart;

        CGImageRef zzzImg = s_zzzImages[state->zzzFrame];
        if (zzzImg) {
            float imgW = (float)CGImageGetWidth(zzzImg);
            float imgH = (float)CGImageGetHeight(zzzImg);
            CGContextSetAlpha(cg, zzzAlpha);
            CGRect zzzRect = CGRectMake(zzzX - imgW / 2.0f, zzzY - imgH / 2.0f, imgW, imgH);
            CGContextDrawImage(cg, zzzRect, zzzImg);
        } else {
            CGContextSetRGBFillColor(cg, kZzzFallbackR, kZzzFallbackG, kZzzFallbackB, zzzAlpha);
            CTFontRef zzzFont = CTFontCreateWithName(CFSTR("Helvetica-Bold"), kZzzFontsize, NULL);
            CFTypeRef zzzKeys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
            CGColorRef zzzColor = CGColorCreateGenericRGB(kZzzFallbackR, kZzzFallbackG, kZzzFallbackB, zzzAlpha);
            CFTypeRef zzzValues[] = { zzzFont, zzzColor };
            CFDictionaryRef zzzAttrs = CFDictionaryCreate(NULL, (const void**)zzzKeys, (const void**)zzzValues, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

            CFStringRef zzzString = CFStringCreateWithCString(NULL, zzzStr, kCFStringEncodingUTF8);
            CFAttributedStringRef zzzAttrStr = CFAttributedStringCreate(NULL, zzzString, zzzAttrs);
            CTLineRef zzzLine = CTLineCreateWithAttributedString(zzzAttrStr);

            if (zzzLine) {
                CGContextSetTextPosition(cg, zzzX, zzzY);
                CTLineDraw(zzzLine, cg);
                CFRelease(zzzLine);
            }

            CFRelease(zzzAttrStr);
            CFRelease(zzzString);
            CFRelease(zzzAttrs);
            CFRelease(zzzColor);
            CFRelease(zzzFont);
        }
    }

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_pomodoroBehavior = {
    .id = "pomodoro",
    .name = "Pomodoro",
    .description = "Pomodoro timer: goose rests during work, goes wild during break",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.systems.pomodoro,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = cleanupPomoFont,
    .conflicts = nullptr,
    .priority = 10,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_pomodoroBehavior);
