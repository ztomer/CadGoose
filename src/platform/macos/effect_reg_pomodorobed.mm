#import "effect_window.h"
// effect_reg_pomodorobed.mm
// Registers PomodoroBed effect type with EffectWindowManager.

#import "effect_registration.h"
#import "pomodoro_bed.h"
#import "world.h"
#import "actor.h"

static std::vector<Vector2> PomodoroBed_GetPositions() {
    std::vector<Vector2> positions;
    for (auto* goose : ActorManager::Instance().getGeese()) {
        auto bedInfo = Pomodoro_GetBedInfo(goose->id);
        if (bedInfo.visible) {
            positions.push_back(bedInfo.position);
        }
    }
    return positions;
}

static float PomodoroBed_GetRadius(const Vector2& pos) {
    for (auto* goose : ActorManager::Instance().getGeese()) {
        auto bedInfo = Pomodoro_GetBedInfo(goose->id);
        if (bedInfo.visible && bedInfo.bedImage &&
            std::abs(bedInfo.position.x - pos.x) < 1.0f && std::abs(bedInfo.position.y - pos.y) < 1.0f) {
            CGImageRef img = (CGImageRef)bedInfo.bedImage;
            float w = (float)CGImageGetWidth(img);
            float h = (float)CGImageGetHeight(img);
            return std::max(w, h) * 0.5f;
        }
    }
    return 30.0f;
}

static bool PomodoroBed_ExistsAt(const Vector2& pos) {
    for (auto* goose : ActorManager::Instance().getGeese()) {
        auto bedInfo = Pomodoro_GetBedInfo(goose->id);
        if (bedInfo.visible && std::abs(bedInfo.position.x - pos.x) < 1.0f && std::abs(bedInfo.position.y - pos.y) < 1.0f) {
            return true;
        }
    }
    return false;
}

static void PomodoroBed_ConfigureWindow(EffectWindow* win, const Vector2& pos) {
    for (auto* goose : ActorManager::Instance().getGeese()) {
        auto bedInfo = Pomodoro_GetBedInfo(goose->id);
        if (bedInfo.visible && bedInfo.bedImage &&
            std::abs(bedInfo.position.x - pos.x) < 1.0f && std::abs(bedInfo.position.y - pos.y) < 1.0f) {
            win.gooseId = goose->id;
            win.cgImage = bedInfo.bedImage;
            return;
        }
    }
}

static struct PomodoroBedRegistration {
    PomodoroBedRegistration() {
        EffectRegister({
            5, // EffectTypePomodoroBed
            PomodoroBed_GetPositions,
            PomodoroBed_GetRadius,
            PomodoroBed_ExistsAt,
            PomodoroBed_ConfigureWindow
        });
    }
} s_pomodoroBedReg;
