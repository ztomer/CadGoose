#include "goose.h"
#include "config.h"
#include "goose_math.h"
#include "world.h"
#include <cmath>

Vector2 Goose::CalculateSeekForce() {
    Vector2 toTarget = target - pos;
    float dist = Vector2::Length(toTarget);
    float arrivalRadius = g_config.movement.arrivalRadius;
    Vector2 moveDir = (dist > 0.01f) ? toTarget / dist : Vector2{0, 1};
    Vector2 desiredVel = moveDir * currentSpeed;
    if (dist < arrivalRadius) {
        desiredVel = desiredVel * (dist / arrivalRadius);
    }
    return (desiredVel - vel) * g_config.physics.steerSeekForce;
}

Vector2 Goose::CalculateCurveForce(float dist) {
    float curveFade = std::min(1.0f, dist / g_config.physics.curveFadeDistance);
    if (Vector2::Length(vel) > g_config.physics.curveFadeMinVel) {
        Vector2 normVel = Vector2::Normalize(vel);
        Vector2 tangent = {-normVel.y, normVel.x};
        return tangent * (parabolicCurvature * currentSpeed *
                         g_config.physics.curveTangentForce * curveFade);
    }
    return Vector2{0, 0};
}

Vector2 Goose::CalculateSeparationForce() {
    Vector2 force{0, 0};
    if (state == GooseState::WANDER || state == GooseState::FETCHING) {
        for (auto &other : g_geese) {
            if (other.id == id) continue;
            float d = Vector2::Distance(pos, other.pos);
            if (d > g_config.spawn.separationMinDistance &&
                d < g_config.spawn.separationMaxDistance) {
                float strength = (g_config.spawn.separationMaxDistance - d) /
                                g_config.spawn.separationMaxDistance;
                Vector2 away = Vector2::Normalize(pos - other.pos);
                force += away * (strength * g_config.movement.maxForce *
                                g_config.spawn.separationForceMultiplier);
            }
        }
    }
    return force;
}

Vector2 Goose::CalculateEdgeAvoidance(int w, int h) {
    Vector2 avoidance{0, 0};
    if (state != GooseState::FETCHING) {
        float lookAhead = currentSpeed * g_config.physics.edgeLookAheadSpeed +
                         g_config.physics.edgeLookAheadBase;
        Vector2 probePos = pos + Vector2::Normalize(vel) * lookAhead;

        float margin = g_config.physics.edgeAvoidMargin;
        float bMinX = 0, bMinY = 0, bMaxX = (float)w, bMaxY = (float)h;
        if (!g_config.cursor.multiMonitorEnabled && !g_monitors.empty()) {
            for (auto &m : g_monitors) {
                if (m.x == 0 && m.y == 0) {
                    bMaxX = (float)m.width;
                    bMaxY = (float)m.height;
                    break;
                }
            }
        }

        if (probePos.x < bMinX + margin) avoidance.x = currentSpeed;
        else if (probePos.x > bMaxX - margin) avoidance.x = -currentSpeed;
        if (probePos.y < bMinY + margin) avoidance.y = currentSpeed;
        else if (probePos.y > bMaxY - margin) avoidance.y = -currentSpeed;
    }
    return avoidance;
}

void Goose::ClampToScreen(int w, int h) {
    float minX = 0.0f, minY = 0.0f, maxX = (float)w, maxY = (float)h;
    if (!g_config.cursor.multiMonitorEnabled && !g_monitors.empty()) {
        for (auto &m : g_monitors) {
            if (m.x == 0 && m.y == 0) {
                maxX = (float)m.width;
                maxY = (float)m.height;
                break;
            }
        }
    }

    if (state == GooseState::FETCHING) {
        minX -= g_config.physics.screenClampExpanded;
        maxX += g_config.physics.screenClampExpanded;
        minY -= g_config.physics.screenClampExpanded;
        maxY += g_config.physics.screenClampExpanded;
    } else {
        minX += g_config.physics.screenClampTight;
        maxX -= g_config.physics.screenClampTight;
        minY += g_config.physics.screenClampTight;
        maxY -= g_config.physics.screenClampTight;
    }

    bool isSpecialState = (state == GooseState::SNATCH_CURSOR ||
                           state == GooseState::RETURNING ||
                           state == GooseState::FETCHING);

    if (pos.x < minX) {
        pos.x = minX + g_config.physics.snapDistance;
        if (isSpecialState && vel.x < 0) vel.x = std::abs(vel.x) + g_config.physics.screenClampBounce;
    } else if (pos.x > maxX) {
        pos.x = maxX - g_config.physics.snapDistance;
        if (isSpecialState && vel.x > 0) vel.x = -std::abs(vel.x) - g_config.physics.screenClampBounce;
    }

    if (pos.y < minY) {
        pos.y = minY + g_config.physics.snapDistance;
        if (isSpecialState && vel.y < 0) vel.y = std::abs(vel.y) + g_config.physics.screenClampBounce;
    } else if (pos.y > maxY) {
        pos.y = maxY - g_config.physics.screenClampBounce;
        if (isSpecialState && vel.y > 0) vel.y = -std::abs(vel.y) - g_config.physics.screenClampBounce;
    }
}