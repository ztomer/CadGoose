#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "assets.h"
#include "cursor_backend.h"
#include <cmath>

void Goose::UpdateDrag(double dt) {
    if (!heldItem) return;

    Vector2 beakTip = GetBeakTipDevice();

    if (!dragInit) {
        dragPos = beakTip;
        dragVel = {0, 0};
        dragRot = 0.0f;
        dragRotVel = 0.0f;
        dragInit = true;
    }

    Vector2 prevPos = dragPos;
    dragPos = beakTip;
    dragVel = (dragPos - prevPos) / (float)std::max(dt, (double)g_config.physics.dragMinDt);

    float targetRot = 0.0f;
    float velMag = Vector2::Length(dragVel);
    if (velMag > g_config.physics.dragVelocityThreshold) {
        targetRot = std::atan2(dragVel.y, dragVel.x) + (float)PI / 2.0f;
    }

    float angDiff = targetRot - dragRot;
    while (angDiff > (float)PI)  angDiff -= (float)(2.0 * PI);
    while (angDiff < (float)-PI) angDiff += (float)(2.0 * PI);
    dragRot += angDiff * std::min(1.0f, (float)dt * g_config.physics.dragRotationSpeed);

    if (!std::isfinite(dragPos.x) || !std::isfinite(dragPos.y) || !std::isfinite(dragRot)) {
        dragPos = beakTip;
        dragVel = {0,0};
        dragRot = 0.0f;
        dragRotVel = 0.0f;
        dragInit = false;
    }
}

void Goose::PickNewTarget(int w, int h) {
    target = {
        (float)(rand() % (std::max(1, (int)(w - g_config.spawn.randomTargetMarginX * 2)) + (int)g_config.spawn.randomTargetMarginX)),
        (float)(rand() % (std::max(1, (int)(h - g_config.spawn.randomTargetMarginY * 2)) + (int)g_config.spawn.randomTargetMarginY))
    };
}
