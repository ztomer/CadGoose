#include "../test_framework.h"
#include "../../include/goose_math.h"

TEST(Vector2_DefaultConstructor) {
    Vector2 v;
    ASSERT_EQ(v.x, 0.0f);
    ASSERT_EQ(v.y, 0.0f);
}

TEST(Vector2_ParameterizedConstructor) {
    Vector2 v{3.0f, 4.0f};
    ASSERT_EQ(v.x, 3.0f);
    ASSERT_EQ(v.y, 4.0f);
}

TEST(Vector2_Addition) {
    Vector2 a{1.0f, 2.0f};
    Vector2 b{3.0f, 4.0f};
    Vector2 c = a + b;
    ASSERT_EQ(c.x, 4.0f);
    ASSERT_EQ(c.y, 6.0f);
}

TEST(Vector2_Subtraction) {
    Vector2 a{5.0f, 7.0f};
    Vector2 b{2.0f, 3.0f};
    Vector2 c = a - b;
    ASSERT_EQ(c.x, 3.0f);
    ASSERT_EQ(c.y, 4.0f);
}

TEST(Vector2_Multiplication) {
    Vector2 v{2.0f, 3.0f};
    Vector2 c = v * 2.0f;
    ASSERT_EQ(c.x, 4.0f);
    ASSERT_EQ(c.y, 6.0f);
}

TEST(Vector2_Normalize) {
    Vector2 v{3.0f, 4.0f};
    Vector2 n = Vector2::Normalize(v);
    ASSERT_FLOAT_EQ(n.x, 0.6f, 0.001f);
    ASSERT_FLOAT_EQ(n.y, 0.8f, 0.001f);
}

TEST(Vector2_Normalize_Zero) {
    Vector2 v{0.0f, 0.0f};
    Vector2 n = Vector2::Normalize(v);
    ASSERT_EQ(n.x, 0.0f);
    ASSERT_EQ(n.y, 0.0f);
}

TEST(Vector2_Distance) {
    Vector2 a{0.0f, 0.0f};
    Vector2 b{3.0f, 4.0f};
    float d = Vector2::Distance(a, b);
    ASSERT_FLOAT_EQ(d, 5.0f, 0.001f);
}

TEST(Vector2_Length) {
    Vector2 v{3.0f, 4.0f};
    float len = Vector2::Length(v);
    ASSERT_FLOAT_EQ(len, 5.0f, 0.001f);
}

TEST(Vector2_FromAngleDegrees) {
    Vector2 v = Vector2::FromAngleDegrees(0.0f);
    ASSERT_FLOAT_EQ(v.x, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(v.y, 0.0f, 0.001f);
    
    v = Vector2::FromAngleDegrees(90.0f);
    ASSERT_FLOAT_EQ(v.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(v.y, 1.0f, 0.001f);
}

TEST(Vector2_Rotate) {
    Vector2 v{1.0f, 0.0f};
    Vector2 r = v.Rotate(90.0f);
    ASSERT_FLOAT_EQ(r.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(r.y, 1.0f, 0.001f);
}

TEST(Scalar_Lerp) {
    float result = Lerp(0.0f, 10.0f, 0.5f);
    ASSERT_FLOAT_EQ(result, 5.0f, 0.001f);
}

TEST(Scalar_Clamp) {
    ASSERT_EQ(Clamp(5.0f, 0.0f, 10.0f), 5.0f);
    ASSERT_EQ(Clamp(-1.0f, 0.0f, 10.0f), 0.0f);
    ASSERT_EQ(Clamp(15.0f, 0.0f, 10.0f), 10.0f);
}

TEST(Scalar_Dot) {
    Vector2 a{1.0f, 0.0f};
    Vector2 b{0.0f, 1.0f};
    ASSERT_EQ(Dot(a, b), 0.0f);
    
    a = Vector2{1.0f, 1.0f};
    b = Vector2{1.0f, 1.0f};
    ASSERT_EQ(Dot(a, b), 2.0f);
}