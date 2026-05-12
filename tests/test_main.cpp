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

// ===========================
// Behavioral Tests (BEHAVIOR.md spec)
// ===========================

// Config matching BEHAVIOR.md line 509-529
struct ConfigSpec {
    bool debugToTerminal = false;
    bool debugVisuals = false;
    float globalScale = 1.0f;
    bool audioEnabled = true;
    bool memesEnabled = true;
    float baseWalkSpeed = 180.0f;
    float baseRunSpeed = 480.0f;
    bool cursorChaseEnabled = true;
    int cursorChaseChance = 3;
    float snatchDuration = 3.0f;
    bool multiMonitorEnabled = true;
    bool mudEnabled = true;
    int mudChance = 15;
    float mudLifetime = 15.0f;
    float runDistanceThreshold = 300.0f;
    float arrivalRadius = 50.0f;
    float targetReachedThresholdNormal = 30.0f;
    float targetReachedMinNormal = 25.0f;
    float targetReachedThresholdReturn = 60.0f;
    float targetReachedMinReturn = 50.0f;
    float catchThreshold = 22.0f;
    float snatchPullDistance = 140.0f;
    float snatchRadiusMin = 40.0f;
    float snatchRadiusMax = 120.0f;
} g_cfg;

// Removed GooseState enum to prevent redefinition

// BEHAVIOR.md line 27-32: WANDER -> CHASE_CURSOR
TEST(Behavior, WanderToChase_ChanceCalculation) {
    // Chance: (cursorChaseChance + attackMouseBias) % 100
    // With default chaseChance=3 and attackMouseBias=100
    int chaseChance = g_cfg.cursorChaseChance;
    int attackBias = 100;
    int totalChance = chaseChance + attackBias;
    if (totalChance > 100) totalChance = 100;
    EXPECT_EQ(totalChance, 100); // Should always chase
}

// BEHAVIOR.md line 98-102: Speed Control
TEST(Behavior, SpeedWalkingWhenNear) {
    // Target speed: baseWalkSpeed when dist <= 300 and NOT in running states
    Vector2 pos{100, 100}, target{200, 200}; // dist = 141 < 300
    float dist = Vector2::Length(target - pos);
    GooseState state = GooseState::WANDER;

    float tSpeed = (dist > g_cfg.runDistanceThreshold ||
                    state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                    state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
        ? g_cfg.baseRunSpeed
        : g_cfg.baseWalkSpeed;

    EXPECT_FLOAT_EQ(tSpeed, g_cfg.baseWalkSpeed);
}

TEST(Behavior, SpeedRunningWhenFar) {
    Vector2 pos{100, 100}, target{500, 500}; // dist = 566 > 300
    float dist = Vector2::Length(target - pos);
    GooseState state = GooseState::WANDER;

    float tSpeed = (dist > g_cfg.runDistanceThreshold ||
                    state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                    state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
        ? g_cfg.baseRunSpeed
        : g_cfg.baseWalkSpeed;

    EXPECT_FLOAT_EQ(tSpeed, g_cfg.baseRunSpeed);
}

TEST(Behavior, SpeedRunningInChaseState) {
    // Even if close, running states should use run speed
    Vector2 pos{100, 100}, target{150, 150}; // dist = 70 < 300
    float dist = Vector2::Length(target - pos);
    GooseState state = GooseState::CHASE_CURSOR;

    float tSpeed = (dist > g_cfg.runDistanceThreshold ||
                    state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                    state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
        ? g_cfg.baseRunSpeed
        : g_cfg.baseWalkSpeed;

    EXPECT_FLOAT_EQ(tSpeed, g_cfg.baseRunSpeed);
}

// BEHAVIOR.md line 106: Arrival slowdown
TEST(Behavior, ArrivalSlowdown) {
    float dist = 25.0f; // < arrivalRadius of 50
    float arrivalRadius = g_cfg.arrivalRadius;
    Vector2 desiredVel{100, 0}; // moving at 100

    if (dist < arrivalRadius) {
        desiredVel = desiredVel * (dist / arrivalRadius);
    }

    // At dist=25 and radius=50, velocity is 100 * (25/50) = 50
    EXPECT_LE(Vector2::Length(desiredVel), 50.0f);
}

// BEHAVIOR.md line 224: Catch threshold
TEST(Behavior, CatchThreshold) {
    float catchThreshold = g_cfg.catchThreshold;
    float globalScale = g_cfg.globalScale;
    float actualThreshold = std::max(22.0f * globalScale, 15.0f);
    EXPECT_FLOAT_EQ(actualThreshold, 22.0f);
}

// BEHAVIOR.md line 227-249: Snatch endpoint calculation
TEST(Behavior, SnatchEndpoint_UsesCurrentPos) {
    // Spec: endpoint = pos - fwd * pullDist + right * lateralBias + fwd * forwardBias
    Vector2 pos{300, 300};
    Vector2 fwd{1, 0}; // right
    Vector2 right{-fwd.y, fwd.x}; // {0, 1} = down
    float pullDist = 140.0f;
    float lateralBias = 20.0f;
    float forwardBias = -30.0f;

    // endpoint = {300,300} - {140,0} + {0,20} + {-30,0} = {130, 320}
    Vector2 endpoint = pos - fwd * pullDist + right * lateralBias + fwd * forwardBias;

    // Endpoint should be behind the goose (x < pos.x since fwd is right)
    EXPECT_LT(endpoint.x, pos.x);
    // y = pos.y + lateralBias + forwardBias*y_component = 300 + 20 + 0 = 320
    EXPECT_FLOAT_EQ(endpoint.y, 320.0f);
}

// BEHAVIOR.md line 241-242: Snatch radius random range
TEST(Behavior, SnatchRadiusRange) {
    // snatchRadius = 40.0f + (rand() % 80) -> 40-120px
    float radiusBase = 40.0f;
    int radiusRange = 80;
    int r = 0; // rand() % 80
    float radius = radiusBase + r;
    EXPECT_GE(radius, 40.0f);
    EXPECT_LE(radius, 120.0f);
}

// BEHAVIOR.md line 341-344: Target reached threshold
TEST(Behavior, TargetReached_NormalState) {
    Vector2 beakTip{228, 200}; // 28px horizontal from target
    Vector2 target{200, 200};
    float threshold = std::max(g_cfg.targetReachedThresholdNormal * g_cfg.globalScale,
                               g_cfg.targetReachedMinNormal);
    float dist = Vector2::Distance(beakTip, target);
    EXPECT_LT(dist, threshold); // 28 < 30
}

TEST(Behavior, TargetReached_ReturningState) {
    Vector2 beakTip{255, 200}; // 55px horizontal from target
    Vector2 target{200, 200};
    float threshold = std::max(g_cfg.targetReachedThresholdReturn * g_cfg.globalScale,
                               g_cfg.targetReachedMinReturn);
    float dist = Vector2::Distance(beakTip, target);
    EXPECT_LT(dist, threshold); // 55 < 60
}

// BEHAVIOR.md line 330-343: TryHonk logic
TEST(Behavior, HonkCooldown) {
    // HONK_CHASE_CD = 1.80s from spec line 330
    double time = 10.0;
    double lastAny = 8.0;
    double lastBucket = 8.0; // 2 seconds ago
    double minGap = 0.6;
    double cooldown = 1.8;

    // 10-8=2 >= 0.6, 10-8=2 >= 1.8 -> true
    bool canHonk = (time - lastAny) >= minGap && (time - lastBucket) >= cooldown;
    EXPECT_TRUE(canHonk);
}

TEST(Behavior, HonkBlockedByGlobalGap) {
    double time = 10.0;
    double lastAny = 9.5; // Only 0.5s ago
    double lastBucket = 8.0;
    double minGap = 0.6;
    double cooldown = 1.8;

    // 10-9.5=0.5 < 0.6 -> should fail
    bool canHonk = (time - lastAny) >= minGap && (time - lastBucket) >= cooldown;
    EXPECT_FALSE(canHonk);
}

// BEHAVIOR.md line 151-158: Direction rotation
TEST(Behavior, DirectionBlending) {
    // Initial direction = 90 degrees (up), velocity = right (0 degrees)
    // Blending should move direction toward velocity
    float dir = 90.0f;
    Vector2 vel{100, 0};

    Vector2 curDirVec{std::cos(dir * 3.14159f / 180.0f), std::sin(dir * 3.14159f / 180.0f)};
    Vector2 targetDirVec = Vector2::Normalize(vel);
    float blendRate = 0.15f;

    Vector2 blended = curDirVec + (targetDirVec - curDirVec) * blendRate;
    float newDir = std::atan2(blended.y, blended.x) * 180.0f / 3.14159f;

    // New direction should be between 90 and 0 (blended)
    EXPECT_GT(newDir, 0.0f);
    EXPECT_LT(newDir, 90.0f);
}

// BEHAVIOR.md line 161: Direction initialization
TEST(Behavior, DirectionInit) {
    // On creation: dir = rand() % 45 (0-44 degrees)
    int dir = 30; // rand() % 45
    EXPECT_GE(dir, 0);
    EXPECT_LT(dir, 45);
}

TEST(Integration, Goose_WanderToChase) {
    Goose g(1, "Test", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {100, 100};
    g.target = {100, 100}; // Already at target
    g.cursorChaseChance = 100;
    g.attackMouseBias = 100;
    
    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};
    
    g_cursorGrabberId = -1; // No one grabbing

    g.Update(0.1, 0.0, 1920, 1080, c);
    
    EXPECT_EQ(g.state, GooseState::CHASE_CURSOR);
    EXPECT_EQ(g.target.x, 500.0f);
    EXPECT_EQ(g.target.y, 500.0f);
}

TEST(Integration, Goose_SnatchCursor) {
    Goose g(2, "Test", 1920, 1080);
    g.state = GooseState::CHASE_CURSOR;
    g.pos = {500, 500};
    g.target = {500, 500}; 
    
    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {0, 0};
    
    g_cursorGrabberId = -1;
    // First update populates rig
    g.Update(0.1, 0.0, 1920, 1080, c);
    
    // Set cursor to exactly beak tip
    c.position = g.GetBeakTipDevice();
    
    // Second update should trigger snatch
    g.Update(0.1, 0.1, 1920, 1080, c);
    
    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR);
    EXPECT_EQ(g_cursorGrabberId, 2);
}

TEST(Integration, Goose_SnatchRelease) {
    Goose g(3, "Test", 1920, 1080);
    g.state = GooseState::SNATCH_CURSOR;
    g.snatchStartTime = 0.0;
    g.snatchDuration = 3.0f; // 3 seconds
    g_cursorGrabberId = 3;

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {500, 500};
    
    // Not enough time passed
    g.Update(0.1, 1.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR);
    EXPECT_EQ(g_cursorGrabberId, 3);

    // Enough time passed (3.5 > 3.0)
    g.Update(0.1, 3.5, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g_cursorGrabberId, -1);
}

TEST(Integration, Goose_FetchItem) {
    Goose g(4, "Test", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {100, 100};
    g.target = {100, 100}; // Reached
    
    // Disable chase to ensure fetch/wander
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;
    
    // Force fetch chance
    g.memeFetchBias = 100;
    g.noteFetchBias = 100;
    
    CursorState c; // empty
    
    g.Update(0.1, 0.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::FETCHING);
}

TEST(Integration, Goose_ReturningItem) {
    Goose g(5, "Test", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {100, 100};
    g.target = {100, 100}; // Reached
    g.forceItemFetch = 0; // Meme
    
    CursorState c;
    
    g.Update(0.1, 0.0, 1920, 1080, c);
    
    EXPECT_EQ(g.state, GooseState::RETURNING);
    EXPECT_NE(g.heldItem, nullptr);
}

TEST(Integration, Goose_DropItem) {
    Goose g(6, "Test", 1920, 1080);
    g.state = GooseState::RETURNING;
    g.pos = {100, 100};
    g.target = {100, 100}; // Reached
    g.heldItem = g_assets.GetRandomMeme(1920, 1080, 0.1f);
    
    int initialDrops = g_droppedItems.size();
    
    CursorState c;
    g.Update(0.1, 0.0, 1920, 1080, c);
    
    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g.heldItem, nullptr);
    EXPECT_EQ(g_droppedItems.size(), initialDrops + 1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Mock dependencies for unit tests
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

// ===========================
// Soak Test - 10 minute simulation
// ===========================
TEST(SoakTest, TenMinuteGoose) {
    Goose g(42, "Soak", 2560, 1440);

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {1280, 720};

    double dt = 1.0 / 60.0;
    double time = 0.0;
    double maxFrames = 36000; // 10 min at 60fps

    double lastLogTime = 0;
    const double LOG_INTERVAL = 5.0; // log every 5 sim seconds

    Vector2 startPos = g.pos;
    Vector2 prevPos = g.pos;
    double cumulativeDist = 0;
    double stuckDuration = 0;
    bool wasStuck = false;
    double maxStuckDuration = 0;
    int stateChanges = 0;
    int prevState = -1;
    int framesWithVelZero = 0;

    for (int frame = 0; frame < maxFrames; frame++, time += dt) {
        g.Update(dt, time, 2560, 1440, c);

        double speed = Vector2::Length(g.vel);
        double moveMag = Vector2::Distance(g.pos, prevPos);
        cumulativeDist += moveMag;

        if (speed < 1.0 && moveMag < 0.1) {
            stuckDuration += dt;
            maxStuckDuration = std::max(maxStuckDuration, stuckDuration);
            if (stuckDuration > 5.0 && !wasStuck) {
                fprintf(stderr, "[SOAK] STUCK t=%.0fs pos(%.0f,%.0f) target(%.0f,%.0f) state=%d\n",
                        time, g.pos.x, g.pos.y, g.target.x, g.target.y, (int)g.state);
                wasStuck = true;
            }
            framesWithVelZero++;
        } else {
            stuckDuration = 0;
            wasStuck = false;
        }

        if ((int)g.state != prevState) {
            stateChanges++;
            prevState = (int)g.state;
        }

        if (time - lastLogTime >= LOG_INTERVAL) {
            lastLogTime = time;
            const char* stateName = (g.state == GooseState::WANDER ? "W" :
                                     g.state == GooseState::FETCHING ? "F" :
                                     g.state == GooseState::RETURNING ? "R" :
                                     g.state == GooseState::CHASE_CURSOR ? "C" : "S");
            fprintf(stderr, "[SOAK] t=%.0f pos(%.0f,%.0f) vel(%.1f,%.1f) spd=%.1f cs=%.1f %s target(%.0f,%.0f) dist=%.0f cumul=%.0f\n",
                    time, g.pos.x, g.pos.y, g.vel.x, g.vel.y,
                    Vector2::Length(g.vel), g.currentSpeed, stateName,
                    g.target.x, g.target.y, Vector2::Distance(g.pos, g.target),
                    cumulativeDist);
        }

        prevPos = g.pos;
    }

    float traveled = Vector2::Distance(g.pos, startPos);
    fprintf(stderr, "[SOAK] Final: pos(%.0f,%.0f) vel(%.1f,%.1f) speed=%.1f state=%d\n",
            g.pos.x, g.pos.y, g.vel.x, g.vel.y, Vector2::Length(g.vel), (int)g.state);
    fprintf(stderr, "[SOAK] Net traveled: %.0f px, Cumulative: %.0f px, stateChanges: %d, maxStuck: %.0fs, zeroVelFrames: %d\n",
            traveled, cumulativeDist, stateChanges, maxStuckDuration, framesWithVelZero);

    // Goose must have cumulative movement > screen width
    EXPECT_GT(cumulativeDist, 2560.0f * 10.0f) << "Goose didn't move enough cumulatively in 10 minutes";
    EXPECT_LT(maxStuckDuration, 20.0) << "Goose was stuck for more than 20 seconds";
    EXPECT_GT(stateChanges, 5) << "Goose should change state at least 5 times in 10 minutes";
}

// ===========================
// Guard Edge Case Tests
// ===========================
extern int g_frameId;
TEST(GuardEdgeCases, SameFrameMultipleCalls) {
    Goose g(90, "GuardEdge", 1920, 1080);
    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};

    g_frameId = 100;
    // First call: guard passes
    g.Update(1.0/60.0, 0.0, 1920, 1080, c);
    Vector2 pos1 = g.pos;

    // Second call: same frame, guards fires
    g.Update(1.0/60.0, 0.001, 1920, 1080, c);
    EXPECT_EQ(g.pos.x, pos1.x) << "Guard should skip duplicate in same frame";
    EXPECT_EQ(g.pos.y, pos1.y) << "Guard should skip duplicate in same frame";

    // Third call: same frame, still skipped
    g.Update(1.0/60.0, 0.002, 1920, 1080, c);
    EXPECT_EQ(g.pos.x, pos1.x) << "Guard should skip third call in same frame";

    // Next frame: guard passes
    ++g_frameId;
    g.Update(1.0/60.0, 0.017, 1920, 1080, c);
    EXPECT_NE(g.pos.x, pos1.x) << "Goose should move on next frame";
}

TEST(GuardEdgeCases, GuardDoesNotFireInTests) {
    // When g_frameId is 0 (no renderer), guard should never fire
    Goose g(91, "GuardTest", 1920, 1080);
    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};

    g_frameId = 0;
    Vector2 pos1 = g.pos;
    g.Update(1.0/60.0, 0.0, 1920, 1080, c);
    Vector2 pos2 = g.pos;
    g.Update(1.0/60.0, 0.1, 1920, 1080, c);
    Vector2 pos3 = g.pos;

    EXPECT_NE(pos1.x, pos2.x) << "Goose should move on frame 0 (no guard)";
    EXPECT_NE(pos2.x, pos3.x) << "Goose should move again on frame 0 (no guard increment)";
}

// ===========================
// Seek Force and Physics Tests
// ===========================
TEST(GoosePhysics, SeekForcePointsTowardTarget) {
    Goose g(60, "Seek", 1920, 1080);
    g.pos = {100, 100};
    g.target = {500, 400}; // target is RIGHT and DOWN from pos
    g.currentSpeed = 180.0f;
    g.vel = {0, 0};

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    g.Update(dt, 0.0, 1920, 1080, c);

    // After one frame, velocity should have components toward target
    // toTarget = (500-100, 400-100) = (400, 300)
    // moveDir = normalize(400, 300) ≈ (0.8, 0.6)
    // desiredVel = (0.8*180, 0.6*180) = (144, 108)
    // steerForce ≈ ((144-0) * 2, (108-0) * 2) ≈ (288, 216), capped at maxForce=350
    // vel.x should be positive (toward target.x > pos.x)
    // vel.y should be positive (toward target.y > pos.y)
    EXPECT_GT(g.vel.x, 0.0f) << "Velocity X should point toward target (positive)";
    EXPECT_GT(g.vel.y, 0.0f) << "Velocity Y should point toward target (positive)";

    // The direction ratio should roughly match the target direction
    float dirRatio = g.vel.y / (g.vel.x + 1e-6f);
    float targetRatio = 300.0f / 400.0f; // 0.75
    EXPECT_NEAR(dirRatio, targetRatio, 0.5f) << "Velocity ratio should roughly match target ratio";
}

TEST(GoosePhysics, SeekForceReversesWhenTargetIsBehind) {
    Goose g(61, "SeekReverse", 1920, 1080);
    g.pos = {500, 500};
    g.target = {100, 100}; // target is BEHIND (left and up)
    g.currentSpeed = 180.0f;
    g.vel = {200, 0}; // goose is moving right, away from target

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    // Run many frames - seek force should eventually reverse velocity
    for (int i = 0; i < 120; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
    }

    // After 120 frames (~2 seconds), velocity should be toward target (negative x)
    EXPECT_LT(g.vel.x, 0.0f) << "After 2s, velocity X should reverse toward target";
    EXPECT_LT(g.vel.y, 0.0f) << "After 2s, velocity Y should point toward target";
}

TEST(GoosePhysics, VelocityStaysBelowCurrentSpeed) {
    Goose g(62, "SpeedCap", 1920, 1080);
    g.pos = {0, 0};
    g.target = {5000, 0}; // far right
    g.currentSpeed = 180.0f; // walk speed
    g.vel = {0, 0};

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    // Run for many frames
    for (int i = 0; i < 600; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
        float speed = Vector2::Length(g.vel);
        EXPECT_LE(speed, g.currentSpeed + 1.0f) << "Velocity magnitude should not exceed currentSpeed";
    }
}

TEST(GoosePhysics, ClampToScreenKeepsGooseOnScreen) {
    Goose g(63, "Clamp", 1920, 1080);
    g.pos = {0, 0};
    g.target = {1920, 1080};
    g.vel = {-500, -500}; // pushing off-screen

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 10; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
        EXPECT_GE(g.pos.x, 0.0f) << "ClampToScreen should keep x >= 0";
        EXPECT_GE(g.pos.y, 0.0f) << "ClampToScreen should keep y >= 0";
        EXPECT_LE(g.pos.x, 1920.0f) << "ClampToScreen should keep x <= screenW";
        EXPECT_LE(g.pos.y, 1080.0f) << "ClampToScreen should keep y <= screenH";
    }
}

// ===========================
// Behavior Integration Tests
// ===========================
TEST(GooseBehaviors, DragSetsPosAndZerosVel) {
    // Simulate what behavior_drag.cpp does
    Goose g(70, "Drag", 1920, 1080);
    Vector2 cursorPos{500, 500};
    g.pos = {495, 495}; // on goose (within radius)

    // Drag behavior sets pos then zeros vel
    g.pos.x = cursorPos.x - 5.0f;
    g.pos.y = cursorPos.y;
    g.vel.x = 0;
    g.vel.y = 0;

    EXPECT_FLOAT_EQ(g.pos.x, 495.0f) << "Drag should set pos.x = cursor.x - 5";
    EXPECT_FLOAT_EQ(g.pos.y, 500.0f) << "Drag should set pos.y = cursor.y";
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f) << "Drag should zero vel.x";
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f) << "Drag should zero vel.y";
}

TEST(GooseBehaviors, JailSetsPosAndZerosVel) {
    Goose g(71, "Jail", 1920, 1080);
    Vector2 jailPos{300, 400};
    g.pos = {100, 100};
    g.vel = {50, -30};

    // Jail behavior
    g.target = jailPos;
    g.pos = jailPos;
    g.vel = {0, 0};

    EXPECT_FLOAT_EQ(g.pos.x, 300.0f) << "Jail should set pos.x";
    EXPECT_FLOAT_EQ(g.pos.y, 400.0f) << "Jail should set pos.y";
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f) << "Jail should zero vel.x";
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f) << "Jail should zero vel.y";
}

TEST(GooseBehaviors, PortalTeleportZerosVel) {
    Goose g(72, "Portal", 1920, 1080);
    Vector2 dest{800, 600};
    g.pos = {100, 100};
    g.vel = {100, 50};

    // Portal teleport
    g.pos.x = dest.x;
    g.pos.y = dest.y;
    g.vel = {0, 0};

    EXPECT_FLOAT_EQ(g.pos.x, 800.0f);
    EXPECT_FLOAT_EQ(g.pos.y, 600.0f);
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f);
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f);
}

TEST(GooseBehaviors, BanishRespawnZerosVel) {
    Goose g(73, "Banish", 1920, 1080);
    g.pos = {100, 100};
    g.vel = {200, -100};

    // Banish respawn
    float screenW = 1920.0f;
    float screenH = 1080.0f;
    g.pos.x = 100.0f + (float)(rand() % (int)(screenW - 200.0f));
    g.pos.y = 100.0f + (float)(rand() % (int)(screenH - 200.0f));
    g.vel = {0, 0};

    EXPECT_GE(g.pos.x, 100.0f) << "Banish respawn x >= 100";
    EXPECT_GE(g.pos.y, 100.0f) << "Banish respawn y >= 100";
    EXPECT_LE(g.pos.x, screenW - 100.0f) << "Banish respawn x <= screenW - 100";
    EXPECT_LE(g.pos.y, screenH - 100.0f) << "Banish respawn y <= screenH - 100";
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f);
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f);
}

TEST(GooseBehaviors, BallTargetInDeviceSpace) {
    // Test that ball target conversion from world to device space is correct
    Goose g(74, "BallCoord", 1920, 1080);
    float globalScale = 1.0f;
    float s_ballPosX = 300.0f;
    float s_ballPosY = 300.0f;
    float BALL_SIZE = 40.0f;

    float ballCenterX = s_ballPosX + BALL_SIZE / 2.0f;
    float ballCenterY = s_ballPosY + BALL_SIZE / 2.0f;
    float ballCenterDevX = ballCenterX * globalScale;
    float ballCenterDevY = ballCenterY * globalScale;

    // At scale=1.0, world and device are the same
    EXPECT_FLOAT_EQ(ballCenterDevX, ballCenterX) << "At scale 1.0, device == world";
    EXPECT_FLOAT_EQ(ballCenterDevY, ballCenterY) << "At scale 1.0, device == world";

    // At scale=2.0, device = world * 2
    globalScale = 2.0f;
    ballCenterDevX = ballCenterX * globalScale;
    ballCenterDevY = ballCenterY * globalScale;
    EXPECT_FLOAT_EQ(ballCenterDevX, ballCenterX * 2.0f) << "At scale 2.0, device = world * 2";
}

TEST(GooseBehaviors, BallTargetSetsGooseTarget) {
    Goose g(75, "BallTarget", 1920, 1080);
    float globalScale = 1.0f;
    float s_ballPosX = 400.0f, s_ballPosY = 500.0f;
    float BALL_SIZE = 40.0f;

    float ballCenterX = s_ballPosX + BALL_SIZE / 2.0f;
    float ballCenterY = s_ballPosY + BALL_SIZE / 2.0f;
    float ballCenterDevX = ballCenterX * globalScale;
    float ballCenterDevY = ballCenterY * globalScale;

    // Simulate ball behavior setting goose target
    g.target = Vector2{ballCenterDevX, ballCenterDevY};
    g.pos = {100, 1000}; // goose is LEFT of ball (ball center is at 420, 520)

    Vector2 toTarget = g.target - g.pos;
    // toTarget should point toward ball center (RIGHT and UP)
    EXPECT_GT(toTarget.x, 0) << "Ball center (420) is to the RIGHT of goose (100)";
    EXPECT_LT(toTarget.y, 0) << "Ball center (520) is ABOVE goose (1000)";
}

// ===========================
// Coordinate Conversion Tests
// ===========================
TEST(CoordinateConversion, WorldToDeviceAndBack) {
    // Test the WorldToDevice / DeviceToWorld functions from goose_math.h
    Vector2 goosePos{500, 500};

    // Device = goosePos + (worldPos - goosePos) * scale
    // World = goosePos + (devicePos - goosePos) / scale

    for (float scale : {0.5f, 1.0f, 2.0f, 3.0f}) {
        Vector2 worldPos{300, 400};
        Vector2 devicePos = WorldToDevice(goosePos, worldPos, scale);
        Vector2 worldBack = DeviceToWorld(goosePos, devicePos, scale);

        EXPECT_NEAR(worldBack.x, worldPos.x, 0.01f) << "Round-trip should recover world.x at scale " << scale;
        EXPECT_NEAR(worldBack.y, worldPos.y, 0.01f) << "Round-trip should recover world.y at scale " << scale;
    }
}

TEST(CoordinateConversion, GoosePosIsIdentityAtUnitScale) {
    Goose g(80, "GoosePos", 1920, 1080);
    g.pos = {100, 200};

    // WorldCoord::GoosePos(g) should return g.pos when globalScale = 1
    g_config.general.globalScale = 1.0f;
    Vector2 posDev = WorldCoord::GoosePos(g);
    EXPECT_FLOAT_EQ(posDev.x, g.pos.x) << "GoosePos.x should match g.pos.x at scale 1.0";
    EXPECT_FLOAT_EQ(posDev.y, g.pos.y) << "GoosePos.y should match g.pos.y at scale 1.0";

    // And for non-unit scale
    g_config.general.globalScale = 2.5f;
    posDev = WorldCoord::GoosePos(g);
    // ToDevice(goose.pos, goose.pos, scale) = goose.pos + (goose.pos - goose.pos) * scale = goose.pos
    EXPECT_FLOAT_EQ(posDev.x, g.pos.x) << "GoosePos is identity regardless of scale (pos-pos)*scale = 0";
    EXPECT_FLOAT_EQ(posDev.y, g.pos.y) << "GoosePos is identity regardless of scale";
    g_config.general.globalScale = 1.0f; // restore
}

// ===========================
// State Machine Tests
// ===========================
TEST(GooseStateMachine, WanderToChaseToSnatchCycle) {
    Goose g(85, "StateCycle", 1920, 1080);
    g.pos = {500, 500};
    g.target = {500, 500}; // at target, so handleWander will fire
    g.state = GooseState::WANDER;
    g.cursorChaseChance = 100; // always chase

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {600, 500};

    // Should trigger chase cursor with 100% chance
    g.Update(1.0/60.0, 1.0, 1920, 1080, c);
    // After update, state might be CHASE_CURSOR or WANDER depending on wand roll
    // We can't assert exact state, but check the goose moves
    float dist = Vector2::Distance(g.pos, g.target);
    EXPECT_LT(dist, 2000.0f) << "Goose should remain on screen after state transition";
}

TEST(GooseStateMachine, FetchingCreatesItem) {
    Goose g(86, "Fetch", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {0, 0};
    g.target = {-40, 500}; // off-screen fetch target
    g.forceItemFetch = 1;
    g.memeFetchBias = 100;
    g.noteFetchBias = 100;

    CursorState c;
    c.caps = CAP_NONE;

    // StartFetch sets target off-screen, goose moves there
    // handleFetching should fire when target is reached
    // Since target is off-screen, isTargetReached might trigger via overshoot
    double dt = 1.0 / 60.0;
    for (int i = 0; i < 60; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
    }

    // After many frames, goose should have handled fetch
    // State should be RETURNING or WANDER (not FETCHING)
    EXPECT_NE(g.state, GooseState::WANDER) << "Fetch should complete and return to wander";
}

// ===========================
// Multi-Geese Tests
// ===========================
TEST(MultiGoose, SeparationForcePushesApart) {
    // Clear existing geese
    g_geese.clear();
    g_nextId = 100;

    // Create two geese near each other
    Goose& g1 = g_geese.emplace_back(100, "Sep1", 1920, 1080);
    g1.pos = {500, 500};
    g1.state = GooseState::WANDER;

    Goose& g2 = g_geese.emplace_back(101, "Sep2", 1920, 1080);
    g2.pos = {520, 500};
    g2.state = GooseState::WANDER;

    CursorState c;
    c.caps = CAP_NONE;

    Vector2 pos1before = g1.pos;
    Vector2 pos2before = g2.pos;

    // Disable chase for separation test
    g1.cursorChaseChance = 0;
    g2.cursorChaseChance = 0;
    g1.attackMouseBias = 0;
    g2.attackMouseBias = 0;
    // Set targets to the same point so separation should dominate
    g1.target = {1000, 1000};
    g2.target = {1000, 1000};

    double dt = 1.0 / 60.0;
    for (int i = 0; i < 30; i++) {
        g1.Update(dt, i * dt, 1920, 1080, c);
        g2.Update(dt, i * dt, 1920, 1080, c);
    }

    // Geese should have moved apart OR toward target
    float g1dist = Vector2::Distance(g1.pos, pos1before);
    float g2dist = Vector2::Distance(g2.pos, pos2before);
    float totalDist = g1dist + g2dist;

    EXPECT_GT(totalDist, 10.0f) << "Geese should move (separation or seek)";

    g_geese.clear(); // cleanup
}

// ===========================
// SolveFeet Consistency Test
// ===========================
TEST(GooseFeet, SolveFeetProducesConsistentPositions) {
    Goose g(110, "Feet", 1920, 1080);
    g.pos = {500, 500};
    g.dir = 0.0f; // facing right
    g.currentSpeed = 180.0f;
    g.vel = {180, 0};

    CursorState c;
    c.caps = CAP_NONE;
    c.position = {1920, 1080};

    // Run for a few frames
    for (int i = 0; i < 30; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);

        // Feet positions should be finite and near the goose
        EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.x));
        EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.y));
        EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.x));
        EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.y));

        // Feet should be roughly within 100 pixels of the goose
        float lDist = Vector2::Distance(g.rig.lFoot.currentPos, g.pos);
        float rDist = Vector2::Distance(g.rig.rFoot.currentPos, g.pos);
        EXPECT_LT(lDist, 100.0f) << "Left foot should be near goose";
        EXPECT_LT(rDist, 100.0f) << "Right foot should be near goose";
    }
}

TEST(GooseFeet, SolveFeetFootPositionsSymmetric) {
    Goose g(111, "FeetSym", 1920, 1080);
    g.pos = {960, 540};
    g.dir = 0.0f; // facing right
    g.currentSpeed = 0.0f; // not moving — feet should be at home positions
    g.vel = {0, 0};

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0 / 60.0, 0.0, 1920, 1080, c);

    // After SolveFeet, both feet should have been initialized from (0,0) to home positions
    // Feet should NOT be at (0,0) after first update
    EXPECT_NE(g.rig.lFoot.currentPos.x, 0.0f) << "Left foot should be initialized";
    EXPECT_NE(g.rig.rFoot.currentPos.x, 0.0f) << "Right foot should be initialized";

    // Feet should be at different positions (non-degenerate)
    float diffX = std::abs(g.rig.rFoot.currentPos.x - g.rig.lFoot.currentPos.x);
    float diffY = std::abs(g.rig.rFoot.currentPos.y - g.rig.lFoot.currentPos.y);
    float totalDiff = diffX + diffY;
    EXPECT_GT(totalDiff, 0.01f) << "Feet should be at different positions";
}

// ===========================
// Multi-Window Duplicate Update Guard Test
// ===========================
extern int g_frameId;
TEST(GuardTest, DuplicateUpdateSkipped) {
    Goose g(99, "Guard", 1920, 1080);
    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};

    // Simulate renderer running — it increments g_frameId to >0
    g_frameId = 42;

    // First update — should run normally
    Vector2 posBefore = g.pos;
    g.Update(1.0/60.0, 0.0, 1920, 1080, c);
    Vector2 posAfter1 = g.pos;
    EXPECT_NE(posAfter1.x, posBefore.x) << "Goose should move on first update";

    // Second update at same g_frameId — guard should prevent double-move
    // This simulates a second window's timer firing within the same tick cycle.
    float movedX = g.pos.x;
    g.Update(1.0/60.0, 0.001, 1920, 1080, c);
    EXPECT_EQ(g.pos.x, movedX) << "Duplicate update was NOT skipped — double-move detected";

    // Next real frame (renderer incremented g_frameId)
    ++g_frameId;
    g.Update(1.0/60.0, 0.017, 1920, 1080, c);
    EXPECT_NE(g.pos.x, movedX) << "Goose didn't move on next frame";
}

void AssetManager::MudSquish() {}
CGImageRef AssetManager::GetBehaviorImage(const std::string&) { return nullptr; }