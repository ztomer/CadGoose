#pragma once

#include <string>
#include <vector>

bool MCP_StartInternalServer();
void MCP_StopInternalServer();
bool MCP_IsInternalRunning();

int MCP_RunStdioServer();

bool MCP_StartHTTPServer();
void MCP_StopHTTPServer();
bool MCP_IsHTTPRunning();

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