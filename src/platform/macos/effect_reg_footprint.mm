#import "effect_window.h"
// effect_reg_footprint.mm
// Registers Footprint effect type with EffectWindowManager.

#import "effect_registration.h"
#import "world.h"

// Time-base note: footprints are stamped with `fp.timeSpawned` set to the
// goose-tick clock (renderer's currentTime, published via g_time on each
// tick), NOT wall-clock NSDate. Comparing against [[NSDate date]
// timeIntervalSince1970] makes every footprint look billions of seconds old
// and filters them all out, so nothing ever renders. Use g_time to stay in
// the same time base as the producer.

static std::vector<Vector2> Footprint_GetPositions() {
    std::vector<Vector2> positions;
    for (const auto& fp : g_world.footprints) {
        float age = (float)(g_time - fp.timeSpawned);
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
    for (const auto& fp : g_world.footprints) {
        float age = (float)(g_time - fp.timeSpawned);
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
