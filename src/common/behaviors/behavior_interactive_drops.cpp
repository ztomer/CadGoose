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
        Vector2 dropPos = goose->GetBeakTipDevice();
        InteractiveFlower flower;
        flower.pos = dropPos;
        flower.spawnTime = time;
        flower.hue = (rand() % 360) / 360.0f * 360.0f;
        state->flowers.push_back(flower);
        state->lastDropTime = time;
        g_assets.Bite();
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
