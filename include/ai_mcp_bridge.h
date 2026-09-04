#pragma once

#include <string>
#include <vector>

// Try to handle a chat message as an MCP command (e.g. "enable ball",
// "honk", "spawn goose"). Returns true and fills response if handled,
// false if the message is not a recognized command pattern.
bool AI_TryMCPCommand(const std::string& message, std::string& response);

// Normalize and tokenize a message: lowercase, split on whitespace,
// strip leading/trailing punctuation from each token.
std::vector<std::string> AI_TokenizeMessage(const std::string& message);

// Check if a set of normalized tokens matches a specific command pattern.
bool AI_MatchTokens(const std::vector<std::string>& tokens,
                    const std::vector<std::string>& pattern);
