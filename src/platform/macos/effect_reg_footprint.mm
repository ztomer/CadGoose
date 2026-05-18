#import "effect_window.h"
// effect_reg_footprint.mm
// Registers Footprint effect type with EffectWindowManager.

#import "effect_registration.h"
#import "world.h"
#import <Foundation/Foundation.h>



static std::vector<Vector2> Footprint_GetPositions() {
    std::vector<Vector2> positions;
    double now = [[NSDate date] timeIntervalSince1970];
    for (const auto& fp : g_world.footprints) {
        float age = (float)(now - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if (age <= life) {
            positions.push_back(fp.pos);
        }
    }
    return positions;
}

static float Footprint_GetRadius(const Vector2&) {
    return g_config.render.footprintWidth * 0.5f;
}

static bool Footprint_ExistsAt(const Vector2& pos) {
    double now = [[NSDate date] timeIntervalSince1970];
    for (const auto& fp : g_world.footprints) {
        float age = (float)(now - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if (age <= life && std::abs(fp.pos.x - pos.x) < 1.0f && std::abs(fp.pos.y - pos.y) < 1.0f) {
            return true;
        }
    }
    return false;
}

static struct FootprintRegistration {
    FootprintRegistration() {
        EffectRegister({
            1, // EffectTypeFootprint
            Footprint_GetPositions,
            Footprint_GetRadius,
            Footprint_ExistsAt
        });
    }
} s_footprintReg;
