#include "config_string.h"
#include "config.h"

std::mutex& Config_StringMutex() {
    static std::mutex mutex;
    return mutex;
}

std::string ConfigOption_GetString(const ConfigOption& opt) {
    if (opt.type != CFG_STRING || opt.ptr == nullptr) return {};
    std::lock_guard<std::mutex> lock(Config_StringMutex());
    return *static_cast<const std::string*>(opt.ptr);
}

void ConfigOption_SetString(const ConfigOption& opt, std::string value) {
    if (opt.type != CFG_STRING || opt.ptr == nullptr) return;
    std::lock_guard<std::mutex> lock(Config_StringMutex());
    *static_cast<std::string*>(opt.ptr) = std::move(value);
}
