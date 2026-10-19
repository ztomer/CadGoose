#pragma once
#include <mutex>
#include <string>

struct ConfigOption;

// Every CFG_STRING field in the config registry is read and written ONLY here,
// under one mutex (ROADMAP M1.2). The fields are std::string objects that more
// than one thread touches: the MCP server thread rewrites hotkeys
// (set_hotkey) and string settings (set_config), while the main thread saves
// and loads config.toml. A std::string read while another thread reassigns it
// is a data race, so a per-call-site lock is the wrong shape (one site always
// forgets it, which is how Config_SaveAll came to read the hotkeys bare).
//
// The hotkey cache (hotkey_cache.cpp) takes the same mutex, so a hotkey
// written through Hotkey_SetName and one written through the registry are
// serialized against each other and against every save.

// The one lock for registry string storage. Never held across a call that
// could take it again.
std::mutex& Config_StringMutex();

// Copy of the option's string value, taken under the lock.
std::string ConfigOption_GetString(const ConfigOption& opt);

// Replace the option's string value under the lock.
void ConfigOption_SetString(const ConfigOption& opt, std::string value);
