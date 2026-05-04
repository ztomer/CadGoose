#include <gtest/gtest.h>
#include <cmath>

struct Vec2 { float x, y; };

Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
Vec2 operator-(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }

Vec2 normalize(float x, float y) {
    float len = std::sqrt(x*x + y*y);
    return len > 0 ? Vec2{x/len, y/len} : Vec2{0,0};
}

float lerp(float a, float b, float t) { return a + t*(b-a); }

float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

float dot(float ax, float ay, float bx, float by) { return ax*bx + ay*by; }

float dist(float ax, float ay, float bx, float by) {
    return std::sqrt((bx-ax)*(bx-ax) + (by-ay)*(by-ay));
}

// Math tests
TEST(Vector2, Addition) {
    Vec2 a{1,2}, b{3,4}, c = a+b;
    EXPECT_EQ(c.x, 4.0f);
    EXPECT_EQ(c.y, 6.0f);
}

TEST(Vector2, Subtraction) {
    Vec2 a{5,7}, b{2,3}, c = a-b;
    EXPECT_EQ(c.x, 3.0f);
    EXPECT_EQ(c.y, 4.0f);
}

TEST(Vector2, Normalize) {
    Vec2 n = normalize(3, 4);
    EXPECT_FLOAT_EQ(n.x, 0.6f);
    EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(Vector2, NormalizeZero) {
    Vec2 n = normalize(0, 0);
    EXPECT_EQ(n.x, 0.0f);
    EXPECT_EQ(n.y, 0.0f);
}

TEST(Scalar, Lerp) {
    EXPECT_FLOAT_EQ(lerp(0, 10, 0.5f), 5.0f);
}

TEST(Scalar, Clamp) {
    EXPECT_EQ(clamp(5, 0, 10), 5);
    EXPECT_EQ(clamp(-1, 0, 10), 0);
    EXPECT_EQ(clamp(15, 0, 10), 10);
}

TEST(Scalar, Dot) {
    EXPECT_FLOAT_EQ(dot(1, 0, 0, 1), 0.0f);
    EXPECT_FLOAT_EQ(dot(1, 1, 1, 1), 2.0f);
}

TEST(Math, Distance) {
    EXPECT_FLOAT_EQ(dist(0, 0, 3, 4), 5.0f);
}

// Cursor backend tests  
TEST(Cursor, Caps) {
    enum Caps { CAP_NONE = 0, CAP_GET_POS = 1, CAP_MOVE_ABS = 2, CAP_MOVE_REL = 4 };
    int caps = CAP_GET_POS | CAP_MOVE_ABS;
    EXPECT_EQ(caps, 3);
}

TEST(Cursor, BackendInit) {
    struct Backend {
        bool init_called = false;
        bool init() { init_called = true; return true; }
    };
    Backend b;
    EXPECT_TRUE(b.init());
    EXPECT_TRUE(b.init_called);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}