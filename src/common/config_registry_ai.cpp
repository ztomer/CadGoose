#include "config.h"

void RegisterAI(std::vector<ConfigOption>& r) {
    r.push_back(CONFIG_INT("AI", "provider_type", "Provider Type",
        &g_config.ai.providerType, 0, 2, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "osaurus_port", "Osaurus Port",
        &g_config.ai.osaurusPort, 0, 65535, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "ollama_port", "Ollama Port",
        &g_config.ai.ollamaPort, 0, 65535, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "custom_port", "Custom Port",
        &g_config.ai.customPort, 0, 65535, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "osaurus_model", "Osaurus Model",
        &g_config.ai.osaurusModel, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "ollama_model", "Ollama Model",
        &g_config.ai.ollamaModel, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "custom_endpoint", "Custom Endpoint",
        &g_config.ai.customEndpoint, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "custom_model", "Custom Model",
        &g_config.ai.customModel, OnConfigChange));
    r.push_back(CONFIG_FLOAT("AI", "evil_level", "Evil Level",
        &g_config.ai.evilLevel, 0.0f, 1.0f, 0.01f, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "show_status_bar", "Show Status Bar",
        &g_config.ai.showStatusBar, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "enable_mcp", "Enable MCP Server",
        &g_config.ai.enableMCP, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "mcp_port", "MCP HTTP Port",
        &g_config.ai.mcpPort, 1024, 65535, OnConfigChange));

}
