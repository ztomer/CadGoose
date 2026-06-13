#include <gtest/gtest.h>
#include "goose_math.h"

TEST(Vector2Math, DefaultConstructor) {
    Vector2 v;
    EXPECT_EQ(v.x, 0.0f);
    EXPECT_EQ(v.y, 0.0f);
}

TEST(Vector2Math, ParameterizedConstructor) {
    Vector2 v{3.0f, 4.0f};
    EXPECT_EQ(v.x, 3.0f);
    EXPECT_EQ(v.y, 4.0f);
}

TEST(Vector2Math, Addition) {
    Vector2 a{1.0f, 2.0f};
    Vector2 b{3.0f, 4.0f};
    Vector2 c = a + b;
    EXPECT_EQ(c.x, 4.0f);
    EXPECT_EQ(c.y, 6.0f);
}

TEST(Vector2Math, Subtraction) {
    Vector2 a{5.0f, 7.0f};
    Vector2 b{2.0f, 3.0f};
    Vector2 c = a - b;
    EXPECT_EQ(c.x, 3.0f);
    EXPECT_EQ(c.y, 4.0f);
}

TEST(Vector2Math, MultiplicationByScalar) {
    Vector2 v{2.0f, 3.0f};
    Vector2 c = v * 2.0f;
    EXPECT_EQ(c.x, 4.0f);
    EXPECT_EQ(c.y, 6.0f);
}

TEST(Vector2Math, DivisionByScalar) {
    Vector2 v{6.0f, 8.0f};
    Vector2 c = v / 2.0f;
    EXPECT_EQ(c.x, 3.0f);
    EXPECT_EQ(c.y, 4.0f);
}

TEST(Vector2Math, AddAssign) {
    Vector2 v{1.0f, 2.0f};
    v += Vector2{3.0f, 4.0f};
    EXPECT_EQ(v.x, 4.0f);
    EXPECT_EQ(v.y, 6.0f);
}

TEST(Vector2Math, AddAssignChain) {
    Vector2 v{1.0f, 1.0f};
    v += Vector2{1.0f, 1.0f};
    v += Vector2{1.0f, 1.0f};
    EXPECT_EQ(v.x, 3.0f);
    EXPECT_EQ(v.y, 3.0f);
}

TEST(Vector2Math, Normalize) {
    Vector2 v{3.0f, 4.0f};
    Vector2 n = Vector2::Normalize(v);
    EXPECT_FLOAT_EQ(n.x, 0.6f);
    EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(Vector2Math, NormalizeZero) {
    Vector2 v{0.0f, 0.0f};
    Vector2 n = Vector2::Normalize(v);
    EXPECT_EQ(n.x, 0.0f);
    EXPECT_EQ(n.y, 0.0f);
}

TEST(Vector2Math, Distance) {
    Vector2 a{0.0f, 0.0f};
    Vector2 b{3.0f, 4.0f};
    float d = Vector2::Distance(a, b);
    EXPECT_FLOAT_EQ(d, 5.0f);
}

TEST(Vector2Math, Length) {
    Vector2 v{3.0f, 4.0f};
    float len = Vector2::Length(v);
    EXPECT_FLOAT_EQ(len, 5.0f);
}

TEST(Vector2Math, FromAngleDegrees) {
    Vector2 v = Vector2::FromAngleDegrees(0.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);

    v = Vector2::FromAngleDegrees(90.0f);
    EXPECT_NEAR(v.x, 0.0f, 1e-6f);
    EXPECT_FLOAT_EQ(v.y, 1.0f);
}

TEST(Vector2Math, Rotate) {
    Vector2 v{1.0f, 0.0f};
    Vector2 r = v.Rotate(90.0f);
    EXPECT_NEAR(r.x, 0.0f, 1e-6f);
    EXPECT_FLOAT_EQ(r.y, 1.0f);
}

TEST(Vector2Math, CopyAssignment) {
    Vector2 a{1.0f, 2.0f};
    Vector2 b = a;
    EXPECT_EQ(b.x, 1.0f);
    EXPECT_EQ(b.y, 2.0f);
}

TEST(Vector2Math, AddResultIsNewVector) {
    Vector2 a{1.0f, 2.0f};
    Vector2 b{3.0f, 4.0f};
    Vector2 c = a + b;
    EXPECT_EQ(a.x, 1.0f);
    EXPECT_EQ(a.y, 2.0f);
    EXPECT_EQ(b.x, 3.0f);
    EXPECT_EQ(b.y, 4.0f);
}

TEST(ScalarMath, Lerp) {
    EXPECT_FLOAT_EQ(Lerp(0.0f, 10.0f, 0.5f), 5.0f);
}

TEST(ScalarMath, LerpBegin) {
    EXPECT_FLOAT_EQ(Lerp(0.0f, 10.0f, 0.0f), 0.0f);
}

TEST(ScalarMath, LerpEnd) {
    EXPECT_FLOAT_EQ(Lerp(0.0f, 10.0f, 1.0f), 10.0f);
}

TEST(ScalarMath, Clamp) {
    EXPECT_EQ(Clamp(5.0f, 0.0f, 10.0f), 5.0f);
    EXPECT_EQ(Clamp(-1.0f, 0.0f, 10.0f), 0.0f);
    EXPECT_EQ(Clamp(15.0f, 0.0f, 10.0f), 10.0f);
}

TEST(ScalarMath, Dot) {
    Vector2 a{1.0f, 0.0f};
    Vector2 b{0.0f, 1.0f};
    EXPECT_EQ(Dot(a, b), 0.0f);

    a = Vector2{1.0f, 1.0f};
    b = Vector2{1.0f, 1.0f};
    EXPECT_EQ(Dot(a, b), 2.0f);
}
