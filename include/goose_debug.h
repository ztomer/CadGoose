#ifndef GOOSE_DEBUG_H
#define GOOSE_DEBUG_H

#include "cursor_io.h"
#include <cstdio>

FILE* GetDebugLog();
void LogTick(double time, const CursorState &cursor);
void CloseDebugLog();

extern bool s_stateChanged;
extern double s_lastLogTime;

#endif // GOOSE_DEBUG_H