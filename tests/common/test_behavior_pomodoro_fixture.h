// test_behavior_pomodoro_fixture.h
// Shared fixture (mocks + BehaviorPomodoroTest) for the pomodoro behavior
// tests. Included by test_behavior_pomodoro.mm and
// test_behavior_pomodoro_render.mm.

#pragma once

#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "event_bus.h"
#include "pomodoro_bed.h"
#include "renderer_interface.h"
#include "behaviors/states/pomodoro_state.h"
#include "items.h"

struct MockPomoRenderer : public IRenderer {
    int saveCount = 0, restoreCount = 0;
    int roundedRectCount = 0;
    int textCount = 0;
    int setAlphaCount = 0;
    float measureTextReturn = 50.0f;

    void SaveState() override { ++saveCount; }
    void RestoreState() override { ++restoreCount; }
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override { ++roundedRectCount; }
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    int imageCount = 0;
    void DrawImage(void*, RenderRect) override { ++imageCount; }
    bool GetImageSize(void* img, float* w, float* h) override { if (img && w && h) { *w = 30; *h = 30; return true; } return false; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override { ++textCount; }
    float MeasureText(const char*, float) override { return measureTextReturn; }
    void SetAlpha(float) override { ++setAlphaCount; }
};

struct PomoSpyGoose : public Goose {
    int honkCount = 0;
    PomoSpyGoose(int id) : Goose(id, "pomodoro", 1920, 1080) { behaviorsEnabled = true; m_canHonk = true; }
    void onHonk() override { ++honkCount; }
};

class BehaviorPomodoroTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("pomodoro")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedPomoEnabled = g_config.behaviors.systems.pomodoro;
        savedWorkMin = g_config.behaviors.pomodoro.workMinutes;
        savedBreakMin = g_config.behaviors.pomodoro.breakMinutes;
        savedLongBreakMin = g_config.behaviors.pomodoro.longBreakMinutes;
        savedSessions = g_config.behaviors.pomodoro.sessionsBeforeLongBreak;
        savedAggressive = g_config.behaviors.pomodoro.enableAggressiveMode;
        savedAggressiveHonkInterval = g_config.behaviors.pomodoro.aggressiveHonkInterval;
        savedBaseWalkSpeed = g_config.movement.baseWalkSpeed;
        savedBaseRunSpeed = g_config.movement.baseRunSpeed;
        savedP2Width = g_config.portal.p2Width;
        savedP2Height = g_config.portal.p2Height;

        g_config.behaviors.systems.pomodoro = true;
        g_config.behaviors.pomodoro.workMinutes = 1;
        g_config.behaviors.pomodoro.breakMinutes = 1;
        g_config.behaviors.pomodoro.longBreakMinutes = 1;
        g_config.behaviors.pomodoro.sessionsBeforeLongBreak = 2;
        g_config.behaviors.pomodoro.enableAggressiveMode = true;
        g_config.behaviors.pomodoro.aggressiveHonkInterval = 2.0f;
        g_config.movement.baseWalkSpeed = 100.0f;
        g_config.movement.baseRunSpeed = 200.0f;

        g_world.screenWidth = 1920;
        g_world.screenHeight = 1080;

        goose = new PomoSpyGoose(1);
        goose->pos = {500, 500};
        goose->dir = 0;
        goose->vel = {0, 0};
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        delete goose;
        g_config.behaviors.systems.pomodoro = savedPomoEnabled;
        g_config.behaviors.pomodoro.workMinutes = savedWorkMin;
        g_config.behaviors.pomodoro.breakMinutes = savedBreakMin;
        g_config.behaviors.pomodoro.longBreakMinutes = savedLongBreakMin;
        g_config.behaviors.pomodoro.sessionsBeforeLongBreak = savedSessions;
        g_config.behaviors.pomodoro.enableAggressiveMode = savedAggressive;
        g_config.behaviors.pomodoro.aggressiveHonkInterval = savedAggressiveHonkInterval;
        g_config.movement.baseWalkSpeed = savedBaseWalkSpeed;
        g_config.movement.baseRunSpeed = savedBaseRunSpeed;
    }

    PomoSpyGoose* goose;
    BehaviorContext ctx{};

private:
    bool savedPomoEnabled;
    int savedWorkMin, savedBreakMin, savedLongBreakMin, savedSessions;
    bool savedAggressive;
    float savedAggressiveHonkInterval;
    float savedBaseWalkSpeed, savedBaseRunSpeed;
    float savedP2Width, savedP2Height;
};
