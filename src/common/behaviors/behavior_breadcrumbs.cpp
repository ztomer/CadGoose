#include "behavior.h"
#include "behaviors/states/breadcrumb_state.h"
#include "event_bus.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "cursor_io.h"
#include "hotkey.h"
#include "renderer_interface.h"
#include "render_colors.h"
#include "ring_buffer.h"
#include "actor.h"
#include "actor_breadcrumb.h"
#include "platform_input.h"
#include <cmath>

static bool s_wasKeyDown = false;
static double s_lastKeyCheck = 0;
static int s_nextCrumbId = 0;

static void LogCrumb(const char* msg) {
    fprintf(stderr, "[Breadcrumbs] %s\n", msg);
}

// Deactivate the BreadcrumbActor matching a crumb position.
static void deactivateCrumbActor(const Vector2& crumbPos) {
    auto& mgr = ActorManager::Instance();
    for (int i = mgr.totalCount() - 1; i >= 0; i--) {
        Actor* a = mgr.getByIndex(i);
        if (a && a->actorType() == ActorType::Breadcrumb &&
            a->position().x == crumbPos.x && a->position().y == crumbPos.y) {
            a->setActive(false);
            break;
        }
    }
}

static void init(BehaviorContext& ctx) {
    s_wasKeyDown = false;
    s_lastKeyCheck = 0;
    s_nextCrumbId = 0;
    g_world.crumbs.clear();
    (void)ctx;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    (void)ctx; (void)dt;

    if (time - s_lastKeyCheck < 0.016) return;
    s_lastKeyCheck = time;

    if (!g_cursorProvider) return;
    CursorState cs = g_cursorProvider->Read();
    if (!cs.hasPos()) return;
    Vector2 cursorPos = cs.position;
    int keyCode = KeyNameToKeyCode(g_config.behaviors.breadCrumbs.hotkey);
    bool keyDown = Platform_IsKeyPressed(keyCode);

    if (keyDown && !s_wasKeyDown) {
        s_wasKeyDown = true;
        Crumbs crumb;
        crumb.pos = cursorPos;
        crumb.time = time;
        crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
        g_world.crumbs.push(crumb);
        // Create BreadcrumbActor
        BreadcrumbActor* actor = new BreadcrumbActor(cursorPos, time, crumb.lifetime);
        ActorManager::Instance().add(actor);
        EventBus::Instance().Publish(BreadcrumbDroppedEvent{cursorPos.x, cursorPos.y});
        LogCrumb("first crumb dropped");
        goose->onHonk();
    } else if (!keyDown) {
        if (s_wasKeyDown) LogCrumb("key released");
        s_wasKeyDown = false;
    }

    if (keyDown && !g_world.crumbs.empty()) {
        Crumbs& last = g_world.crumbs.back();
        float dist = std::hypot(cursorPos.x - last.pos.x, cursorPos.y - last.pos.y);
        if (dist >= g_config.behaviors.breadCrumbs.spawnDist) {
            Crumbs crumb;
            crumb.pos = cursorPos;
            crumb.time = time;
            crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
            g_world.crumbs.push(crumb);
            // Create BreadcrumbActor
            BreadcrumbActor* actor = new BreadcrumbActor(cursorPos, time, crumb.lifetime);
            ActorManager::Instance().add(actor);
            EventBus::Instance().Publish(BreadcrumbDroppedEvent{cursorPos.x, cursorPos.y});
        }
    }

    int maxCrumbs = g_config.behaviors.breadCrumbs.maxCrumbs;
    while (g_world.crumbs.size() > (size_t)maxCrumbs) {
        Vector2 frontPos = g_world.crumbs.front().pos;
        g_world.crumbs.pop();
        deactivateCrumbActor(frontPos);
    }

    while (!g_world.crumbs.empty() && time - g_world.crumbs.front().time > g_world.crumbs.front().lifetime) {
        g_world.crumbs.pop();
    }

    float eatRadius = g_config.render.footSize * 2.0f;
    for (size_t i = 0; i < g_world.crumbs.size(); ++i) {
        Crumbs& crumb = g_world.crumbs[i];
        if (crumb.eaten) continue;
        float dist = std::hypot(goose->pos.x - crumb.pos.x, goose->pos.y - crumb.pos.y);
        if (dist < eatRadius) {
            crumb.eaten = true;
            deactivateCrumbActor(crumb.pos);
            EventBus::Instance().Publish(ItemEatenEvent{goose->id, crumb.pos.x, crumb.pos.y, "breadcrumb"});
            g_assets.Bite();
            goose->isChewing = true;
            goose->chewingStartTime = time;
            break;
        }
    }

    while (!g_world.crumbs.empty() && g_world.crumbs.front().eaten) {
        g_world.crumbs.pop();
    }
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    (void)goose; (void)ctx; (void)irenderer;
}

static Behavior g_breadcrumbBehavior = BEHAVIOR_DEF(
    "breadcrumbs", "Bread Crumbs", "Hold hotkey to drop breadcrumbs at cursor. Based on BreadCrumbs by Straaft",
    g_config.behaviors.fun.breadCrumbs, init, tick, render
);

REGISTER_BEHAVIOR(g_breadcrumbBehavior);
