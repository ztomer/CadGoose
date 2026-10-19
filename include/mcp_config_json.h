#pragma once

#include <string>

// The MCP view of the behavior settings, as JSON (ROADMAP M1.7). Reads the
// plain bool fields of g_config, which the main thread owns: call it ONLY from
// the main thread (the command socket's handler does, for `get_config`). The
// MCP server thread must ask for it over the socket, never call it.
//
// group: "" for everything (toggles and hotkeys), or one of "fun", "control",
// "info", "systems" for that toggle group alone. An unknown group returns "".
std::string McpConfigJson(const std::string& group);
