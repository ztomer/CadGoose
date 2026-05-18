#include "goose_debug.h"
#include "world.h"
#include "goose.h"
#include "actor.h"

static FILE *s_debugLog = nullptr;

bool s_stateChanged = true;
double s_lastLogTime = 0;

FILE *GetDebugLog() {
  if (!s_debugLog) {
    s_debugLog = fopen("/tmp/goose_debug.log", "w");
    if (!s_debugLog)
      s_debugLog = stderr;
  }
  return s_debugLog;
}

void LogTick(double time, const CursorState &cursor) {
  FILE *f = GetDebugLog();
  if (!f)
    return;
  const char *ss[] = {"W", "F", "R", "C", "S"};
  fprintf(f, "[T%.1f] cur=%d", time, g_world.cursorGrabberId);
  if (cursor.hasPos())
    fprintf(f, " c(%.0f,%.0f)", cursor.position.x, cursor.position.y);
  else
    fprintf(f, " c(-,-)");
  fprintf(f, " geese:");
  for (auto* g : ActorManager::Instance().getGeese()) {
    const char* stateNames[] = {"W", "F", "R", "C", "S"};
    fprintf(f, " %d%s@(%d,%d)d%dv(%d,%d)s%d", g->id, stateNames[static_cast<int>(g->state)], (int)g->pos.x,
            (int)g->pos.y, (int)g->dir, (int)g->vel.x, (int)g->vel.y,
            (int)g->currentSpeed);
    if (g->state == GooseState::SNATCH_CURSOR)
      fprintf(f, " a=%.1f r=%.0f", g->snatchAngle, g->snatchRadius);
  }
  fprintf(f, "\n");
}

void CloseDebugLog() { s_debugLog = nullptr; }