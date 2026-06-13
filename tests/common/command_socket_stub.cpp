#include "command_socket.h"
#include <mutex>

namespace {
    bool g_result = true;
    std::string g_response = "ok";
    std::string g_error;
    std::vector<std::vector<std::string>> g_received;
    std::mutex g_mutex;
}

void CommandSocketStub_Reset() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_result = true;
    g_response = "ok";
    g_error.clear();
    g_received.clear();
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
    std::lock_guard<std::mutex> lock(g_mutex);
    g_received.push_back(args);
    if (responseOut) *responseOut = g_response;
    if (errorOut) *errorOut = g_error;
    return g_result;
}
