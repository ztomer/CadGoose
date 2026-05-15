#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "goose_math.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ctime>
#include <cmath>


static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<InteractiveDropsState>(ctx.goose->id, "interactive_drops");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<InteractiveDropsState>(goose->id, "interactive_drops");

    if (goose->heldItem) return;
    if (goose->state != GooseState::WANDER) return;

    double lastDrop = state->lastDropTime;
    if (lastDrop == 0) lastDrop = goose->lastDropTime;
    bool shouldDrop = (time - lastDrop >= g_config.behaviors.interactiveDrops.dropInterval) && ((rand() % 400) == 0);

    if (shouldDrop) {
        int dropType = rand() % 2;
        Vector2 dropPos = goose->GetBeakTipDevice();
        if (dropType == 0) {
            InteractivePuddle puddle;
            puddle.pos = dropPos;
            puddle.spawnTime = time;
            puddle.radius = 5.0f;
            puddle.maxRadius = 20.0f + (rand() % 20);
            state->puddles.push_back(puddle);
        } else {
            InteractiveFlower flower;
            flower.pos = dropPos;
            flower.spawnTime = time;
            flower.hue = (rand() % 360) / 360.0f * 360.0f;
            state->flowers.push_back(flower);
        }
        state->lastDropTime = time;
        g_assets.Bite();
    }
    for (auto it = state->puddles.begin(); it != state->puddles.end(); ) {
        double age = time - it->spawnTime;
        if (age > g_config.behaviors.interactiveDrops.puddleLifetime) {
            it = state->puddles.erase(it);
            continue;
        }
        if (age < 1.0) {
            it->radius = it->maxRadius * (float)age * 2.0f;
            it->alpha = 0.6f;
        } else {
            it->alpha = 0.6f * (1.0f - (float)((age - 1.0) / (g_config.behaviors.interactiveDrops.puddleLifetime - 1.0)));
        }
        if (!it->splashed) {
            Vector2 cursorPos;
            if (g_cursorProvider) {
                CursorState cs = g_cursorProvider->Read();
                cursorPos = cs.position;
                if (cs.hasPos() && Vector2::Distance(cursorPos, it->pos) < it->radius + 10.0f) {
                    it->splashed = true;
                    it->vel = Vector2::Normalize(cursorPos - it->pos) * 60.0f;
                }
            }
        }
        if (it->splashed) {
            it->pos = it->pos + it->vel * (float)dt;
            it->vel = it->vel * 0.95f;
            it->alpha *= 0.98f;
        }
        ++it;
    }

    for (auto it = state->flowers.begin(); it != state->flowers.end(); ) {
        double age = time - it->spawnTime;
        float growTime = g_config.behaviors.interactiveDrops.flowerGrowTime;
        if (age > growTime * 3.0f) {
            it = state->flowers.erase(it);
            continue;
        }
        if (age < growTime) {
            float p = (float)age / growTime;
            it->growth = p;
            it->stemHeight = 15.0f * p;
            it->petalSize = 5.0f * p;
        }
        ++it;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<InteractiveDropsState>(goose->id, "interactive_drops");

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    for (auto& p : state->puddles) {
        if (p.alpha <= 0.01f) continue;
        CGContextSetRGBFillColor(cg, 0.3f, 0.5f, 0.8f, p.alpha * 0.4f);
        CGContextFillEllipseInRect(cg, CGRectMake(p.pos.x - p.radius, p.pos.y - p.radius, p.radius * 2, p.radius * 2));
        CGContextSetRGBStrokeColor(cg, 0.2f, 0.4f, 0.7f, p.alpha * 0.6f);
        CGContextSetLineWidth(cg, 1.0f);
        CGContextStrokeEllipseInRect(cg, CGRectMake(p.pos.x - p.radius, p.pos.y - p.radius, p.radius * 2, p.radius * 2));
    }

    for (auto& f : state->flowers) {
        if (f.growth <= 0.01f) continue;
        CGContextSetRGBStrokeColor(cg, 0.2f, 0.6f, 0.2f, 0.8f);
        CGContextSetLineWidth(cg, 2.0f);
        CGContextMoveToPoint(cg, f.pos.x, f.pos.y);
        CGContextAddLineToPoint(cg, f.pos.x, f.pos.y - f.stemHeight);
        CGContextStrokePath(cg);

        float h = f.hue;
        float r, g, b;
        HSV_to_RGB(h, 1.0f, 0.8f, &r, &g, &b);
        CGContextSetRGBFillColor(cg, r, g, b, 0.8f);
        for (int i = 0; i < 5; i++) {
            float angle = i * 72.0f * (M_PI / 180.0f);
            float px = f.pos.x + std::cos(angle) * f.petalSize;
            float py = f.pos.y - f.stemHeight + std::sin(angle) * f.petalSize * 0.5f;
            CGContextFillEllipseInRect(cg, CGRectMake(px - f.petalSize * 0.5f, py - f.petalSize * 0.3f, f.petalSize, f.petalSize * 0.6f));
        }
        CGContextSetRGBFillColor(cg, 1.0f, 0.9f, 0.3f, 0.9f);
        CGContextFillEllipseInRect(cg, CGRectMake(f.pos.x - 2.0f, f.pos.y - f.stemHeight - 2.0f, 4.0f, 4.0f));
    }

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_interactiveDropsBehavior = BEHAVIOR_DEF_GROUND(
    "interactive_drops", "Interactive Drops", "Goose drops puddles that splash and flowers that grow",
    g_config.behaviors.fun.interactiveDrops, init, tick, render
);

REGISTER_BEHAVIOR(g_interactiveDropsBehavior);
