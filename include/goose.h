// ===========================
// goose.h
// ===========================
#ifndef GOOSE_H
#define GOOSE_H

#include <string>

#ifdef __linux__
#include <gtk/gtk.h>
struct cairo_t;
typedef struct cairo_t cairo_t;
#endif

#include "goose_math.h"
#include "assets.h"
#include "cursor_io.h"

enum GooseState { WANDER, FETCHING, RETURNING, CHASE_CURSOR, SNATCH_CURSOR };

struct FootState {
    Vector2 currentPos{};
    Vector2 moveOrigin{};
    Vector2 moveDir{};
    double moveStartTime = -1.0;
    float moveDuration = 0.2f;
};

struct Rig {
    Vector2 underbody, body, neckBase, neckHead, head1, head2;
    float neckLerp = 0;
    FootState lFoot, rFoot;
};

class Goose {
public:
    void PickNewTarget(int w, int h);
    
    int id;
    std::string name;
    Vector2 pos{200, 200};
    Vector2 target{500, 500};
    Vector2 vel{};
    Vector2 acceleration{};
    float dir = 90.0f;
    float maxForce = 350.0f;
    float parabolicCurvature = 0.0f; // Multiplier for tangential curve force

    // State
    GooseState state = WANDER;
    ItemData* heldItem = nullptr;
    int forceItemFetch = -1; // -1: Random, 0: Meme, 1: Text
    std::string forcedText;

    float currentSpeed = 0;
    float stepTime = 0.2f;
    Rig rig;

    Vector2 ISO_SCALE;

    Vector2 dragPos{};
    Vector2 dragVel{};
    float dragRot = 0.0f; // radians
    float dragRotVel = 0.0f;
    bool dragInit = false;

    // Cursor chase/snatch state
    double snatchStartTime = 0.0;
    Vector2 snatchOffset{};  // Cursor anchor stored in goose-local forward/right space during snatch
    Vector2 snatchAnchor{};  // Goose position at snatch start (fixed reference point)
    Vector2 snatchFwd{};     // Fixed forward direction at snatch start
    // How far the goose pulls the cursor behind it when snatching
    float snatchPullDistance = 140.0f;
    // Circular snatch motion parameters
    float snatchAngle = 0.0f;           // current angular phase (radians)
    float snatchRadius = 60.0f;         // radius of circular motion in pixels
    float snatchAngularSpeed = 2.5f;    // radians per second

    // Per-goose tendencies (0-100)
    int attackMouseBias = 0; // added to global cursor chase chance
    int noteFetchBias = 0;   // increases chance to fetch notes
    int memeFetchBias = 0;   // increases chance to fetch memes

    // Per-goose dynamic settings
    bool cursorChaseEnabled = true;
    int  cursorChaseChance = 5;
    float snatchDuration = 3.0f;
    bool mudEnabled = true;
    int  mudChance = 15;
    float mudLifetime = 15.0f;

    // Honk timing state
    struct HonkState {
        bool init = false;
        double lastAny = -1e9;
        double lastChase = -1e9;
        double lastFetch = -1e9;
        double lastGeneric = -1e9;
        double nextIdleHonk = 0.0;
    } honkState;

    // Step sound cooldown
    double lastStepSoundTime = -1e9;
    static constexpr double stepSoundCooldown = 0.08; // minimum time between step sounds

    // Debug logging
    bool debugSnatch = true;
    GooseState prevState = WANDER;
    double lastDebugLog = -1e9;
    static constexpr double debugLogInterval = 0.1;

    Goose(int _id, const std::string& _name, int screenW, int screenH);

    CursorAction Update(double dt, double time, int scrW, int scrH, const CursorState& cursor);
    void ForceFetch(int type, int w, int h);
    void ForceFetchText(const std::string& text, int w, int h);
    void ForceWander(int w, int h);

#ifdef __linux__
    void Draw(cairo_t* cr);
#endif

    // Coordinate helpers (using common utilities from goose_math.h)
    // GetBeakTipDevice returns beak tip in DEVICE coordinates (screen space)
    // Per BEHAVIOR.md line 228-229: cursor moved to beak tip every frame
    Vector2 GetBeakTipDevice();

    void StartSnatch(double time, const Vector2& cursorPos);
    void EndSnatch(double time, int w, int h);

private:
    void UpdateDrag(double dt);
    void StartFetch(int w, int h);
    CursorAction UpdateBehaviors(double dt, double time, int w, int h, const CursorState& cursor);
    void UpdateChaseCursor(double time, const Vector2& cursorPos);

#ifdef __linux__
    void DrawHeldItem(cairo_t* cr);
    void DrawEyes(cairo_t* cr, Vector2 fwd);
    Vector2 GetFootHome(float angleOffset);
    void SolveFeet(double time);
    void UpdateRig();
    void DrawEllipse(cairo_t* cr, Vector2 p, int rx, int ry, float r, float g, float b, float a=1.0);
    void DrawLine(cairo_t* cr, Vector2 a, Vector2 b, float w, const float color[]);
    void DrawLine(cairo_t* cr, Vector2 a, Vector2 b, float w, float r, float g, float bl);
#else
    Vector2 GetFootHome(float angleOffset);
    void SolveFeet(double time);
    void UpdateRig();
#endif
};

#endif // GOOSE_H
