#pragma once
#include "goose_math.h"
#include "cursor_io.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// Abstract base class for all backends
class CursorBackend : public ICursorProvider {
public:
    virtual ~CursorBackend() = default;

    virtual std::string Name() const = 0;
    virtual uint32_t Caps() const = 0;
    virtual bool Init() = 0;

    virtual Vector2 GetCursorPos() = 0;
    virtual void MoveCursorAbs(int x, int y) = 0;
    virtual void MoveCursorRel(int dx, int dy) = 0;

    CursorState Read() override;
    void Execute(const CursorAction& action) override;
};

// Manager to handle selection and global access
class CursorBackendManager {
public:
    CursorBackendManager();
    ~CursorBackendManager() = default;

    void Init();
    CursorBackend* GetActiveBackend() { return m_activeBackend; }

private:
    void RegisterBackend(std::shared_ptr<CursorBackend> backend);
    
    std::vector<std::shared_ptr<CursorBackend>> m_backends;
    CursorBackend* m_activeBackend = nullptr;
};

extern CursorBackendManager g_backendManager;
