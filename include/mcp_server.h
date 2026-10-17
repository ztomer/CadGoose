#pragma once

#include <string>
#include <vector>

bool MCP_StartInternalServer();
void MCP_StopInternalServer();
bool MCP_IsInternalRunning();
// The unix socket the internal server binds (default
// /tmp/desktop-goose-mcp.sock). Set returns the previous path; an empty path
// restores the default. Tests point it at a per-process path so they never
// collide with each other or unlink a running app's socket (ROADMAP G0.3).
std::string MCP_InternalSocketPath();
std::string MCP_SetInternalSocketPath(std::string path);

int MCP_RunStdioServer();

bool MCP_StartHTTPServer();
void MCP_StopHTTPServer();
bool MCP_IsHTTPRunning();
// The port the HTTP server is bound to, 0 when stopped. g_config.ai.mcpPort
// 0 binds a free port chosen by the kernel; read it back here.
int MCP_HTTPPort();

// Internal bridge: call an MCP tool directly from C++ (bypasses socket).
// name: tool name (e.g. "goose_status", "set_config")
// argsJson: JSON object with tool arguments (e.g. "{\"key\":\"behaviors.fun.ball\",\"value\":true}")
// Returns the tool result text.
std::string MCP_CallTool(const std::string& name, const std::string& argsJson);

// Handle a full JSON-RPC request and return the response JSON.
// Used internally and can be called from the AI chat.
std::string MCP_HandleRequest(const std::string& requestJson);

// Return OpenAI-compatible tool definitions array (JSON) for function calling.
// Each entry has {type:"function",function:{name,description,parameters}}.
std::string MCP_GetOpenAITools();