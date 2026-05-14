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
    r.push_back(CONFIG_BOOL("AI", "text_meme_enabled", "AI Text Meme Generation",
        &g_config.ai.textMemeEnabled, OnConfigChange));
    r.push_back(CONFIG_FLOAT("AI", "text_meme_temperature", "Text Meme Temperature",
        &g_config.ai.textMemeTemperature, 0.0f, 2.0f, 0.1f, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "text_meme_auto_save", "Auto-Save Generated Texts",
        &g_config.ai.textMemeAutoSave, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "text_meme_max_queue", "Max Pending Text Memes",
        &g_config.ai.textMemeMaxQueue, 1, 50, OnConfigChange));
    r.push_back(CONFIG_INT("AI", "chat_max_history", "Max Chat History Messages",
        &g_config.ai.chatMaxHistory, 10, 1000, OnConfigChange));
    r.push_back(CONFIG_BOOL("AI", "local_llm_enabled", "Local CoreML LLM",
        &g_config.ai.localLlmEnabled, OnConfigChange));
    r.push_back(CONFIG_STRING("AI", "local_llm_model_path", "Local CoreML Model Path",
        &g_config.ai.localLlmModelPath, OnConfigChange));

}
