#include <gtest/gtest.h>
#include <cmath>
#include "goose_math.h"
#include "coordinate_system.h"
#include "world_coord.h"
#include "config.h"

TEST(CoordinateSystem, DevicePointDefault) {
    DevicePoint p;
    EXPECT_EQ(p.x, 0.0f);
    EXPECT_EQ(p.y, 0.0f);
}

TEST(CoordinateSystem, DevicePointConstruct) {
    DevicePoint p(3.0f, 4.0f);
    EXPECT_EQ(p.x, 3.0f);
    EXPECT_EQ(p.y, 4.0f);
}

TEST(CoordinateSystem, DevicePointFromVector2) {
    DevicePoint p(Vector2{1.0f, 2.0f});
    EXPECT_EQ(p.x, 1.0f);
    EXPECT_EQ(p.y, 2.0f);
}

TEST(CoordinateSystem, DevicePointToVector2) {
    DevicePoint p(3.0f, 4.0f);
    Vector2 v = (Vector2)p;
    EXPECT_EQ(v.x, 3.0f);
    EXPECT_EQ(v.y, 4.0f);
}

TEST(CoordinateSystem, DevicePointToVector2Explicit) {
    DevicePoint p(3.0f, 4.0f);
    Vector2 v = p.toVector2();
    EXPECT_EQ(v.x, 3.0f);
    EXPECT_EQ(v.y, 4.0f);
}

TEST(CoordinateSystem, DevicePointPlus) {
    DevicePoint a(1, 2), b(3, 4);
    DevicePoint c = a + b;
    EXPECT_EQ(c.x, 4);
    EXPECT_EQ(c.y, 6);
}

TEST(CoordinateSystem, DevicePointMinus) {
    DevicePoint a(5, 7), b(2, 3);
    DevicePoint c = a - b;
    EXPECT_EQ(c.x, 3);
    EXPECT_EQ(c.y, 4);
}

TEST(CoordinateSystem, DevicePointMulScalar) {
    DevicePoint p(2, 3);
    DevicePoint r = p * 2.0f;
    EXPECT_EQ(r.x, 4);
    EXPECT_EQ(r.y, 6);
}

TEST(CoordinateSystem, DevicePointDivScalar) {
    DevicePoint p(6, 8);
    DevicePoint r = p / 2.0f;
    EXPECT_EQ(r.x, 3);
    EXPECT_EQ(r.y, 4);
}

TEST(CoordinateSystem, DevicePointAddAssign) {
    DevicePoint p(1, 2);
    p += DevicePoint(3, 4);
    EXPECT_EQ(p.x, 4);
    EXPECT_EQ(p.y, 6);
}

TEST(CoordinateSystem, DevicePointSubAssign) {
    DevicePoint p(5, 7);
    p -= DevicePoint(2, 3);
    EXPECT_EQ(p.x, 3);
    EXPECT_EQ(p.y, 4);
}

TEST(CoordinateSystem, DevicePointDistance) {
    float d = DevicePoint::Distance({0, 0}, {3, 4});
    EXPECT_FLOAT_EQ(d, 5.0f);
}

TEST(CoordinateSystem, WorldPointDefault) {
    WorldPoint p;
    EXPECT_EQ(p.x, 0.0f);
    EXPECT_EQ(p.y, 0.0f);
}

TEST(CoordinateSystem, WorldPointConstruct) {
    WorldPoint p(3.0f, 4.0f);
    EXPECT_EQ(p.x, 3.0f);
    EXPECT_EQ(p.y, 4.0f);
}

TEST(CoordinateSystem, WorldPointFromVector2) {
    WorldPoint p(Vector2{1.0f, 2.0f});
    EXPECT_EQ(p.x, 1.0f);
    EXPECT_EQ(p.y, 2.0f);
}

TEST(CoordinateSystem, WorldPointOperators) {
    WorldPoint a(1, 2), b(3, 4);
    EXPECT_EQ((a + b).x, 4);
    EXPECT_EQ((a - b).x, -2);
    EXPECT_EQ((a * 2.0f).x, 2);
    EXPECT_EQ((a / 2.0f).x, 0.5f);
}

TEST(CoordinateSystem, WorldPointDistance) {
    float d = WorldPoint::Distance({0, 0}, {3, 4});
    EXPECT_FLOAT_EQ(d, 5.0f);
}

TEST(CoordinateSystem, ScreenPoint) {
    ScreenPoint p(1.0f, 2.0f);
    EXPECT_EQ(p.x, 1.0f);
    EXPECT_EQ(p.y, 2.0f);
    Vector2 v = (Vector2)p;
    EXPECT_EQ(v.x, 1.0f);
}

TEST(CoordinateSystem, ViewPoint) {
    ViewPoint p(1.0f, 2.0f);
    EXPECT_EQ(p.x, 1.0f);
    EXPECT_EQ(p.y, 2.0f);
    Vector2 v = (Vector2)p;
    EXPECT_EQ(v.x, 1.0f);
}

TEST(CoordinateTransform, WorldToDevice) {
    DevicePoint r = CoordTransform::WorldToDevice(
        WorldPoint{10, 20}, DevicePoint{100, 200}, 2.0f);
    EXPECT_EQ(r.x, 100 + (10 - 100) * 2);
    EXPECT_EQ(r.y, 200 + (20 - 200) * 2);
}

TEST(CoordinateTransform, DeviceToWorld) {
    WorldPoint r = CoordTransform::DeviceToWorld(
        DevicePoint{100, 200}, DevicePoint{50, 60}, 2.0f);
    EXPECT_FLOAT_EQ(r.x, 50 + (100 - 50) / 2.0f);
    EXPECT_FLOAT_EQ(r.y, 60 + (200 - 60) / 2.0f);
}

TEST(CoordinateTransform, DeviceToWorldTinyScale) {
    WorldPoint r = CoordTransform::DeviceToWorld(
        DevicePoint{100, 200}, DevicePoint{50, 60}, 0.0f);
    EXPECT_EQ(r.x, 100);
    EXPECT_EQ(r.y, 200);
}

TEST(CoordinateTransform, Scale) {
    EXPECT_FLOAT_EQ(CoordTransform::Scale(10.0f, 2.0f), 20.0f);
}

TEST(CoordinateTransform, ScreenToDevice) {
    DevicePoint d = CoordTransform::ScreenToDevice(ScreenPoint{5, 10});
    EXPECT_EQ(d.x, 5);
    EXPECT_EQ(d.y, 10);
}

TEST(CoordinateTransform, DeviceToScreen) {
    ScreenPoint s = CoordTransform::DeviceToScreen(DevicePoint{5, 10});
    EXPECT_EQ(s.x, 5);
    EXPECT_EQ(s.y, 10);
}

TEST(CoordinateTransform, ScreenToDeviceMacOS) {
    DevicePoint d = CoordTransform::ScreenToDeviceMacOS(ScreenPoint{5, 10}, 100);
    EXPECT_EQ(d.x, 5);
    EXPECT_EQ(d.y, 90);
}

TEST(CoordinateTransform, DeviceToScreenMacOS) {
    ScreenPoint s = CoordTransform::DeviceToScreenMacOS(DevicePoint{5, 90}, 100);
    EXPECT_EQ(s.x, 5);
    EXPECT_EQ(s.y, 10);
}

TEST(CoordinateTransform, ViewToDevice) {
    DevicePoint d = CoordTransform::ViewToDevice(ViewPoint{5, 10}, 30);
    EXPECT_EQ(d.x, 5);
    EXPECT_EQ(d.y, 30);
}

TEST(CoordinateTransform, CairoToDevice) {
    DevicePoint d = CoordTransform::CairoToDevice(5, 10, 100);
    EXPECT_EQ(d.x, 5);
    EXPECT_EQ(d.y, 90);
}

TEST(CoordinateTransform, DeviceToCairo) {
    Vector2 v = CoordTransform::DeviceToCairo(DevicePoint{5, 90}, 100);
    EXPECT_EQ(v.x, 5);
    EXPECT_EQ(v.y, 10);
}

TEST(ItemCoords, Center) {
    DevicePoint c = ItemCoords::Center({10, 20}, 100, 50, 2.0f);
    EXPECT_FLOAT_EQ(c.x, 10 + 100 * 2 * 0.5f);
    EXPECT_FLOAT_EQ(c.y, 20 + 50 * 2 * 0.5f);
}

TEST(ItemCoords, HalfSize) {
    DevicePoint h = ItemCoords::HalfSize(100, 50, 2.0f);
    EXPECT_FLOAT_EQ(h.x, 100 * 2 * 0.5f);
    EXPECT_FLOAT_EQ(h.y, 50 * 2 * 0.5f);
}

TEST(ItemCoords, Size) {
    DevicePoint s = ItemCoords::Size(100, 50, 2.0f);
    EXPECT_FLOAT_EQ(s.x, 200);
    EXPECT_FLOAT_EQ(s.y, 100);
}

TEST(HitTest, PointInItem) {
    bool hit = HitTest::PointInItem(
        {110, 120}, {100, 100}, 100, 50, 0.0f, 2.0f);
    EXPECT_TRUE(hit);
}

TEST(HitTest, PointInItemMiss) {
    bool hit = HitTest::PointInItem(
        {500, 500}, {100, 100}, 100, 50, 0.0f, 2.0f);
    EXPECT_FALSE(hit);
}

TEST(HitTest, PointInCloseButton) {
    bool hit = HitTest::PointInCloseButton(
        {0, 0}, {100, 100}, 100, 50, 0.0f, 20, 2.0f);
    EXPECT_FALSE(hit);
}

TEST(ScreenBounds, FromDimensions) {
    ScreenBounds b = ScreenBounds::FromDimensions(800, 600);
    EXPECT_EQ(b.minX, 0);
    EXPECT_EQ(b.minY, 0);
    EXPECT_EQ(b.maxX, 800);
    EXPECT_EQ(b.maxY, 600);
}

TEST(ScreenBounds, FromMonitor) {
    ScreenBounds b = ScreenBounds::FromMonitor(100, 200, 800, 600);
    EXPECT_EQ(b.minX, 100);
    EXPECT_EQ(b.minY, 200);
    EXPECT_EQ(b.maxX, 900);
    EXPECT_EQ(b.maxY, 800);
}

TEST(ScreenBounds, Contains) {
    ScreenBounds b = ScreenBounds::FromDimensions(800, 600);
    EXPECT_TRUE(b.Contains({400, 300}));
    EXPECT_FALSE(b.Contains({900, 300}));
}

TEST(ScreenBounds, ContainsWithMargin) {
    ScreenBounds b = ScreenBounds::FromDimensions(800, 600);
    EXPECT_FALSE(b.Contains({5, 5}, 10));
    EXPECT_TRUE(b.Contains({50, 50}, 10));
}

TEST(ScreenBounds, Clamp) {
    ScreenBounds b = ScreenBounds::FromDimensions(800, 600);
    DevicePoint p = b.Clamp({-50, -50});
    EXPECT_EQ(p.x, 0);
    EXPECT_EQ(p.y, 0);
    p = b.Clamp({900, 700});
    EXPECT_EQ(p.x, 800);
    EXPECT_EQ(p.y, 600);
    p = b.Clamp({400, 300});
    EXPECT_EQ(p.x, 400);
    EXPECT_EQ(p.y, 300);
}

TEST(WorldCoord, WorldToDeviceWithGoose) {
    Config_Init();
    g_config.general.globalScale = 2.0f;
    DevicePoint r = WorldCoord::WorldToDevice(WorldPoint{10, 20}, DevicePoint{100, 200}, 2.0f);
    EXPECT_FLOAT_EQ(r.x, 100 + (10 - 100) * 2);
    EXPECT_FLOAT_EQ(r.y, 200 + (20 - 200) * 2);
}

TEST(WorldCoord, OriginToDevice) {
    Config_Init();
    g_config.general.globalScale = 2.0f;
    DevicePoint d = WorldCoord::OriginToDevice({10, 20});
    EXPECT_FLOAT_EQ(d.x, 20);
    EXPECT_FLOAT_EQ(d.y, 40);
}

TEST(WorldCoord, ScaleStatic) {
    Config_Init();
    g_config.general.globalScale = 2.0f;
    EXPECT_FLOAT_EQ(WorldCoord::Scale(10), 20);
}

TEST(WorldCoord, FromCairo) {
    DevicePoint d = WorldCoord::FromCairo(5, 10, 100);
    EXPECT_EQ(d.x, 5);
    EXPECT_EQ(d.y, 90);
}

TEST(WorldCoord, ToCairo) {
    Vector2 v = WorldCoord::ToCairo(DevicePoint{5, 90}, 100);
    EXPECT_EQ(v.x, 5);
    EXPECT_EQ(v.y, 10);
}

TEST(GooseMath, HSVToRGB) {
    float r, g, b;
    HSV_to_RGB(0.0f, 1.0f, 1.0f, &r, &g, &b);
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 0.0f);
}

TEST(GooseMath, HSVToRGB120) {
    float r, g, b;
    HSV_to_RGB(120.0f, 1.0f, 1.0f, &r, &g, &b);
    EXPECT_FLOAT_EQ(r, 0.0f);
    EXPECT_FLOAT_EQ(g, 1.0f);
    EXPECT_FLOAT_EQ(b, 0.0f);
}

TEST(GooseMath, HSVToRGB240) {
    float r, g, b;
    HSV_to_RGB(240.0f, 1.0f, 1.0f, &r, &g, &b);
    EXPECT_FLOAT_EQ(r, 0.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 1.0f);
}

TEST(GooseMath, HSVToRGB300) {
    float r, g, b;
    HSV_to_RGB(300.0f, 1.0f, 1.0f, &r, &g, &b);
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 1.0f);
}

TEST(GooseMath, HSVToRGB360) {
    float r, g, b;
    HSV_to_RGB(360.0f, 1.0f, 1.0f, &r, &g, &b);
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
    EXPECT_FLOAT_EQ(b, 0.0f);
}

TEST(GooseMath, HSVToRGBZeroSaturation) {
    float r, g, b;
    HSV_to_RGB(100.0f, 0.0f, 0.5f, &r, &g, &b);
    EXPECT_FLOAT_EQ(r, 0.5f);
    EXPECT_FLOAT_EQ(g, 0.5f);
    EXPECT_FLOAT_EQ(b, 0.5f);
}

TEST(GooseMath, WorldToDeviceVec) {
    Vector2 r = WorldToDevice({100, 200}, {10, 20}, 2.0f);
    EXPECT_FLOAT_EQ(r.x, 100 + (10 - 100) * 2);
    EXPECT_FLOAT_EQ(r.y, 200 + (20 - 200) * 2);
}

TEST(GooseMath, DeviceToWorldVec) {
    Vector2 r = DeviceToWorld({100, 200}, {50, 60}, 2.0f);
    EXPECT_FLOAT_EQ(r.x, 100 + (50 - 100) / 2.0f);
    EXPECT_FLOAT_EQ(r.y, 200 + (60 - 200) / 2.0f);
}

TEST(GooseMath, DeviceToWorldTinyScaleVec) {
    Vector2 r = DeviceToWorld({100, 200}, {50, 60}, 0.0f);
    EXPECT_EQ(r.x, 50);
    EXPECT_EQ(r.y, 60);
}

TEST(CoordinateSystem, WorldPointImplicitVector2Conversion) {
    WorldPoint p(3.0f, 4.0f);
    Vector2 v = p;
    EXPECT_EQ(v.x, 3.0f);
    EXPECT_EQ(v.y, 4.0f);
}

TEST(CoordinateSystem, ScreenPointExplicitVector2Constructor) {
    ScreenPoint p(Vector2{1.0f, 2.0f});
    EXPECT_EQ(p.x, 1.0f);
    EXPECT_EQ(p.y, 2.0f);
}

TEST(CoordinateSystem, ViewPointExplicitVector2Constructor) {
    ViewPoint p(Vector2{1.0f, 2.0f});
    EXPECT_EQ(p.x, 1.0f);
    EXPECT_EQ(p.y, 2.0f);
}
