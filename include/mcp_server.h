#pragma once

#include <string>
#include <vector>

bool MCP_StartInternalServer();
void MCP_StopInternalServer();
bool MCP_IsInternalRunning();

int MCP_RunStdioServer();