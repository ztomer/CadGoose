#include "cursor_backend.h"

#if defined(__APPLE__)
#include "mac_cursor_backend.h"
#elif defined(__linux__)
#include "hyprland.h"
#include "x11_backend.h"
#include "wlroots_backend.h"
#endif

#include <algorithm>
#include <iostream>

CursorBackendManager g_backendManager;

CursorBackendManager::CursorBackendManager() {
    class NullBackend : public CursorBackend {
    public:
        std::string Name() const override { return "None"; }
        uint32_t Caps() const override { return CAP_NONE; }
        bool Init() override { return true; }
    };
    static auto nullBackend = std::make_shared<NullBackend>();
    m_activeBackend = nullBackend.get();
}

void CursorBackendManager::RegisterBackend(std::shared_ptr<CursorBackend> backend) {
    m_backends.push_back(backend);
}

void CursorBackendManager::Init() {
#if defined(__APPLE__)
    RegisterBackend(std::make_shared<MacCursorBackend>());
#elif defined(__linux__)
    RegisterBackend(std::make_shared<HyprlandBackend>());
    RegisterBackend(std::make_shared<WlrootsBackend>());
    RegisterBackend(std::make_shared<X11Backend>());
#endif

    std::cout << "Initializing Cursor Backends..." << std::endl;

    for (auto& backend : m_backends) {
        if (backend->Init()) {
            std::cout << "Selected Cursor Backend: " << backend->Name() << std::endl;
            m_activeBackend = backend.get();
            return;
        }
    }

    std::cerr << "Warning: No suitable cursor backend found!" << std::endl;
}