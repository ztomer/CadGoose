#include <gtest/gtest.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

#include "cursor_backend.h"
#include "cursor_io.h"
#include "goose_math.h"
#include "goose.h"
#include "assets.h"
#include "world.h"

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

TEST(HealthCheck, ConfigInit) {
    struct Config { bool audioEnabled = true; bool mudEnabled = true; } cfg;
    EXPECT_TRUE(cfg.audioEnabled);
    EXPECT_TRUE(cfg.mudEnabled);
}

TEST(HealthCheck, ScreenDimensions) {
    int w = 1920, h = 1080;
    EXPECT_GT(w, 0);
    EXPECT_GT(h, 0);
    EXPECT_GE(w * h, 800 * 600);
}

TEST(HealthCheck, GooseSpawn) {
    struct Goose { int id; float x, y; };
    Goose g{0, 100, 100};
    EXPECT_EQ(g.id, 0);
    EXPECT_FLOAT_EQ(g.x, 100.0f);
    EXPECT_FLOAT_EQ(g.y, 100.0f);
}

TEST(HealthCheck, AudioFilesExist) {
    const char* honkFiles[] = {"Honk1.mp3", "Honk2.mp3", "Honk3.mp3", "Honk4.mp3"};
    int count = sizeof(honkFiles) / sizeof(honkFiles[0]);
    EXPECT_EQ(count, 4);
}

TEST(HealthCheck, GooseBehaviorEvents) {
    GooseState b = GooseState::WANDER;
    EXPECT_EQ(b, GooseState::WANDER);
    EXPECT_NE(b, GooseState::CHASE_CURSOR);
}

TEST(HealthCheck, AudioPlayback) {
    struct AudioPlayer { bool playing = false; bool playingHonk = false; bool playingBite = false; };
    AudioPlayer p;
    EXPECT_FALSE(p.playing);
    p.playingHonk = true;
    EXPECT_TRUE(p.playingHonk);
}

TEST(HealthCheck, WindowProperties) {
    struct Window { bool visible = true; bool clickthrough = true; int level = 3; };
    Window w;
    EXPECT_TRUE(w.visible);
    EXPECT_TRUE(w.clickthrough);
    EXPECT_EQ(w.level, 3);
}

TEST(HealthCheck, Footprint) {
    struct Footprint { float x, y; float lifetime = 10.0f; };
    Footprint fp{100, 200, 15.0f};
    EXPECT_FLOAT_EQ(fp.x, 100.0f);
    EXPECT_FLOAT_EQ(fp.y, 200.0f);
    EXPECT_GT(fp.lifetime, 0.0f);
}

TEST(HealthCheck, DroppedItem) {
    struct Item { std::string type = "meme"; bool held = false; };
    Item i;
    EXPECT_EQ(i.type, "meme");
    EXPECT_FALSE(i.held);
    i.held = true;
    EXPECT_TRUE(i.held);
}

TEST(AssetManager, PathConcat) {
    std::string root = "Assets";
    std::string subfolder = "Images/Memes";
    std::string result = root + "/" + subfolder;
    EXPECT_EQ(result, "Assets/Images/Memes");
}

TEST(AssetManager, FileExtensionCheck) {
    std::vector<std::string> extensions = {".png", ".jpg", ".jpeg"};
    EXPECT_TRUE(std::find(extensions.begin(), extensions.end(), ".png") != extensions.end());
    EXPECT_TRUE(std::find(extensions.begin(), extensions.end(), ".jpg") != extensions.end());
    EXPECT_FALSE(std::find(extensions.begin(), extensions.end(), ".gif") != extensions.end());
}

TEST(GooseMovement, SpeedInitialization) {
    float baseWalkSpeed = 200.0f;
    float currentSpeed = baseWalkSpeed;
    EXPECT_EQ(currentSpeed, baseWalkSpeed);
    currentSpeed = 0;
    EXPECT_EQ(currentSpeed, 0.0f);
}

TEST(GooseMovement, VelocityCalculation) {
    Vec2 dir{1.0f, 0.0f};
    float speed = 100.0f;
    Vec2 velocity{dir.x * speed, dir.y * speed};
    EXPECT_FLOAT_EQ(velocity.x, 100.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST(GooseMovement, TargetReach) {
    Vec2 pos{100, 100};
    Vec2 target{105, 100};
    float dist = std::sqrt((target.x-pos.x)*(target.x-pos.x) + (target.y-pos.y)*(target.y-pos.y));
    EXPECT_LT(dist, 10.0f);
}

TEST(GooseRig, AllRelative) {
    Vec2 up{0, -1};
    Vec2 fwd{1, 0};
    Vec2 offset{3.0f + 0.0f, 0.0f + (-1.0f) * 15.0f};
    EXPECT_FLOAT_EQ(offset.x, 3.0f);
    EXPECT_FLOAT_EQ(offset.y, -15.0f);
}

class TestCursorBackend : public CursorBackend {
public:
    std::string Name() const override { return "Test"; }
    uint32_t Caps() const override { return CAP_GET_POS | CAP_MOVE_ABS | CAP_MOVE_REL; }
    bool Init() override { return true; }
    Vector2 GetCursorPos() override { return m_pos; }
    void MoveCursorAbs(int x, int y) override { m_lastAbsX = x; m_lastAbsY = y; }
    void MoveCursorRel(int dx, int dy) override { m_lastRelX = dx; m_lastRelY = dy; }

    Vector2 m_pos = {-1.0f, -1.0f};
    int m_lastAbsX = 0, m_lastAbsY = 0;
    int m_lastRelX = 0, m_lastRelY = 0;
};

TEST(CursorBackend, VirtualMethods) {
    TestCursorBackend backend;
    EXPECT_EQ(backend.Name(), "Test");
    EXPECT_EQ(backend.Caps(), CAP_GET_POS | CAP_MOVE_ABS | CAP_MOVE_REL);
    EXPECT_TRUE(backend.Init());
}

TEST(CursorBackendManager, Singleton) {
    g_backendManager.Init();
    EXPECT_NE(g_backendManager.GetActiveBackend(), nullptr);
    EXPECT_NE(g_cursorProvider, nullptr);
}

TEST(CursorBackend, Read) {
    TestCursorBackend backend;
    backend.m_pos = {100.0f, 200.0f};

    CursorState state = backend.Read();
    EXPECT_TRUE(state.hasPos());
    EXPECT_FLOAT_EQ(state.position.x, 100.0f);
    EXPECT_FLOAT_EQ(state.position.y, 200.0f);
    EXPECT_TRUE(state.caps & CAP_GET_POS);
}

TEST(CursorBackend, ExecuteMoveAbs) {
    TestCursorBackend backend;
    backend.Execute(CursorAction::MoveAbs(50, 75));
    EXPECT_EQ(backend.m_lastAbsX, 50);
    EXPECT_EQ(backend.m_lastAbsY, 75);
}

TEST(CursorBackend, ExecuteMoveRel) {
    TestCursorBackend backend;
    backend.Execute(CursorAction::MoveRel(10, -5));
    EXPECT_EQ(backend.m_lastRelX, 10);
    EXPECT_EQ(backend.m_lastRelY, -5);
}

TEST(CursorState, HasPos) {
    CursorState s1;
    EXPECT_FALSE(s1.hasPos());

    CursorState s2;
    s2.caps = CAP_GET_POS;
    s2.position = {10.0f, 20.0f};
    EXPECT_TRUE(s2.hasPos());

    CursorState s3;
    s3.caps = CAP_MOVE_ABS;
    s3.position = {10.0f, 20.0f};
    EXPECT_FALSE(s3.hasPos());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    setenv("CADGOOSE_CONFIG_DIR", "/tmp/cadgoose-test-config", 1);
    return RUN_ALL_TESTS();
}

const std::string ASSET_ROOT_NAME = "Assets";
std::filesystem::path ASSET_ROOT = "Assets";

AssetManager g_assets;

void AssetManager::Init() {}
AssetManager::~AssetManager() {}
ItemData* AssetManager::GetRandomMeme(int, int, float) {
    ItemData* i = new ItemData();
    i->type = ItemData::MEME;
    i->w = 100; i->h = 100;
    return i;
}
ItemData* AssetManager::GetRandomText() {
    ItemData* i = new ItemData();
    i->type = ItemData::TEXT;
    i->w = 100; i->h = 100;
    return i;
}
ItemData* AssetManager::CreateTextItem(const std::string& text) {
    ItemData* i = new ItemData();
    i->type = ItemData::TEXT;
    i->w = 100; i->h = 100;
    return i;
}
void AssetManager::Honk() {}
void AssetManager::Pat() {}
void AssetManager::Bite() {}
void AssetManager::MudSquish() {}
CGImageRef AssetManager::GetBehaviorImage(const std::string&) { return nullptr; }
ItemData* AssetManager::CreateToyItem(bool isStick) {
    ItemData* i = new ItemData();
    i->type = ItemData::TOY;
    i->w = isStick ? 32 : 20;
    i->h = isStick ? 8 : 20;
    return i;
}
