// ===========================
// behavior_jail.cpp
// Jail Behavior - Trap the goose in a cage
// Based on GooseJail by WackyModer
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "hotkey.h"
#include "ring_buffer.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#endif

static bool s_oWasKeyDown = false;
static bool s_pWasKeyDown = false;
static constexpr size_t kMaxJails = 10;
static RingBuffer<Vector2, kMaxJails> s_jails;
static bool s_jailsActive = false;
static double s_lastInputTime = 0;

#ifdef __APPLE__
static CTFontRef s_jailFont = nullptr;
static void cleanupJailFont() {
    if (s_jailFont) { CFRelease(s_jailFont); s_jailFont = nullptr; }
}
#endif

static bool IsKeyPressed(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose->id, "jail");

    if (!g_config.behaviors.control.jail) {
        state->isJailed = false;
        s_jailsActive = false;
        s_jails.clear();
        return;
    }

    if (time > s_lastInputTime) {
        s_lastInputTime = time;

        Vector2 cursorPos{-1, -1};
        if (g_cursorProvider) {
            CursorState cs = g_cursorProvider->Read();
            if (cs.hasPos()) {
                cursorPos = cs.position;
            }
        }

        bool oDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyO));
        if (oDown && !s_oWasKeyDown) {
            if (s_jailsActive) {
                // Reset functionality: if active, placing a new trap clears the old ones
                s_jails.clear();
                s_jailsActive = false;
            }
            s_jails.push(cursorPos);
        }
        s_oWasKeyDown = oDown;

        bool pDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyP));
        if (pDown && !s_pWasKeyDown && !s_jails.empty()) {
            s_jailsActive = !s_jailsActive;
            g_assets.Honk();
        }
        s_pWasKeyDown = pDown;
    }

    state->isJailed = s_jailsActive && !s_jails.empty();
    ctx.isJailed = state->isJailed;

    if (state->isJailed) {
        // Find nearest trap
        Vector2 nearest = s_jails[0];
        float minDist = Vector2::Distance(goose->pos, nearest);
        for (size_t i = 1; i < s_jails.size(); ++i) {
            float d = Vector2::Distance(goose->pos, s_jails[i]);
            if (d < minDist) {
                minDist = d;
                nearest = s_jails[i];
            }
        }
        
        goose->target = nearest;
        goose->pos = nearest;
        goose->vel = {0, 0};
        goose->state = GooseState::WANDER;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (goose->id != 0 || s_jails.empty()) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGRenderer renderer(cg);
    renderer.SaveState();

    float jailSize = g_config.behaviors.jail.size;
    float scale = ctx.globalScale;
    float pulse = 0.6f + 0.4f * sin(ctx.time * 3.0f);
    bool jailed = s_jailsActive;

    for (const auto& jailPos : s_jails) {
        Vector2 drawPos{goose->pos.x + (jailPos.x - goose->pos.x) / scale,
                        goose->pos.y + (jailPos.y - goose->pos.y) / scale};
        RenderRect rect{drawPos.x - jailSize/2, drawPos.y - jailSize/2, jailSize, jailSize};

        float strokeAlpha = jailed ? 1.0f : pulse * 0.6f;
        float fillAlpha = jailed ? 0.2f : 0.05f;
        RenderColor jailColor{1.0f, 0.6f, 0.0f, strokeAlpha};

        renderer.DrawRectOutline(rect, jailColor, jailed ? 4.0f : 2.0f);
        renderer.DrawRect(rect, RenderColor{1.0f, 0.6f, 0.0f, fillAlpha});

        const char* label = jailed ? "JAIL" : "SET";
        if (!s_jailFont) s_jailFont = CTFontCreateWithName(CFSTR("Helvetica-Bold"), 14.0f, NULL);
        if (s_jailFont) {
            float textX = drawPos.x - jailSize/2;
            float textY = drawPos.y - jailSize/2 - 20.0f;
            renderer.Translate(textX, textY);
            renderer.Scale(1.0f, -1.0f);
            renderer.DrawText(label, {0, 0}, RenderColor{1.0f, 0.6f, 0.0f, strokeAlpha}, 14.0f);
        }
    }

    renderer.RestoreState();
#endif
}

static Behavior g_jailBehavior = BEHAVIOR_DEF_CUSTOM(
    "jail", "Jail", "Press O to set jail position, P to trap goose. Based on GooseJail by WackyModer",
    g_config.behaviors.control.jail, init, tick, render,
    [](BehaviorContext&) { cleanupJailFont(); }, true, false
);

REGISTER_BEHAVIOR(g_jailBehavior);