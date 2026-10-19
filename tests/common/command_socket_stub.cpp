#include "command_socket.h"
#include <mutex>

namespace {
    bool g_result = true;
    std::string g_response = "ok";
    std::string g_error;
    std::vector<std::vector<std::string>> g_received;
    // When set, Send hands the command to it and returns its answer, the way
    // the real socket hands it to the app's handler on the main thread. Tests
    // of a tool whose effect the app applies (set_config, ROADMAP M1.6) set
    // AppActions_HandleCommand; the rest keep the canned response.
    CommandHandler g_handler;
    std::mutex g_mutex;
}

void CommandSocketStub_SetHandler(CommandHandler handler) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_handler = std::move(handler);
}

void CommandSocketStub_Reset() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_result = true;
    g_response = "ok";
    g_error.clear();
    g_received.clear();
    g_handler = nullptr;
}

void CommandSocketStub_SetResult(bool success) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_result = success;
}

void CommandSocketStub_SetResponse(const std::string& response) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_response = response;
}

void CommandSocketStub_SetError(const std::string& error) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_error = error;
}

std::vector<std::vector<std::string>> CommandSocketStub_GetCommands() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_received;
}

// --- Stub implementations of command_socket.h ---

std::string CommandSocket_GetPath() {
    return "/tmp/test-goose.sock";
}

bool CommandSocket_StartServer(CommandHandler, std::string*) {
    return true;
}

void CommandSocket_StopServer() {
}

bool CommandSocket_Send(const std::vector<std::string>& args, std::string* responseOut, std::string* errorOut) {
    CommandHandler handler;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_received.push_back(args);
        if (!g_handler) {
            if (responseOut) *responseOut = g_response;
            if (errorOut) *errorOut = g_error;
            return g_result;
        }
        handler = g_handler;
    }
    const std::string response = handler(args);
    if (responseOut) *responseOut = response;
    return true;
}
