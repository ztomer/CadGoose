# CadGoose - Linux

Linux-specific documentation for CadGoose using GTK4 and Wayland/X11.

---

## Requirements

### Build tools

- CMake 3.17 or newer
- A C++17-capable compiler (GCC 9+ or Clang 10+ recommended)
- `pkg-config`

### Runtime libraries

| Library | Purpose |
|---|---|
| GTK4 | UI toolkit and rendering |
| gtk4-layer-shell | Wayland layer-shell overlay windows |
| SDL2 | Audio playback |
| SDL2_mixer | Sound effect mixing |
| gdk-pixbuf-2.0 | Image loading for meme assets |
| wayland-client | Wayland protocol base |
| libcurl | (retained for future AI integration) |
| X11 | X11 cursor position queries |
| Xtst | X11 cursor movement injection |

### Optional compositor support

- **Hyprland**: full cursor control via IPC socket
- **wlroots-based compositors**: cursor movement via the `wlr-virtual-pointer-unstable-v1` Wayland protocol
- **X11**: cursor queries and movement via XTest

One of the above is required for cursor chase and snatch behavior. The application runs without cursor control but those features will be disabled.

---

## Building from Source

### 1. Install dependencies

On Arch Linux:

```bash
sudo pacman -S cmake gtk4 gtk4-layer-shell sdl2 sdl2_mixer gdk-pixbuf2 wayland libcurl xorg-server-devel libxtst
```

On Fedora:

```bash
sudo dnf install cmake gtk4-devel gtk4-layer-shell-devel SDL2-devel SDL2_mixer-devel gdk-pixbuf2-devel wayland-devel libcurl-devel libX11-devel libXtst-devel
```

On Ubuntu 24.04 or later:

```bash
sudo apt install cmake libgtk-4-dev libgtk4-layer-shell-dev libsdl2-dev libsdl2-mixer-dev libgdk-pixbuf-2.0-dev libwayland-dev libcurl4-openssl-dev libx11-dev libxtst-dev
```

> **Note:** `gtk4-layer-shell` may not be available in older Ubuntu/Debian repositories. Build it from source from [github.com/wmww/gtk4-layer-shell](https://github.com/wmww/gtk4-layer-shell) if your package manager does not provide it.

### 2. Configure and build

```bash
git clone https://github.com/yourname/CadGoose.git
cd CadGoose
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The compiled binary will be at `build/CadGoose`.

### 3. In-tree builds

The repository ships with some pre-generated Wayland protocol binding files. CMake will regenerate these via `wayland-scanner` if the tool is available. If `wayland-scanner` is not installed, the pre-generated files under `protocols/` will be used as-is.

---

## Running

Run the binary from the repository root so that the `Assets/` directory is resolved correctly.

The default command starts Desktop Goose in the background so you can close your terminal afterward:

```bash
./build/CadGoose
```

You can also start it explicitly:

```bash
./build/CadGoose start
```

Or keep it attached to the current terminal for debugging:

```bash
./build/CadGoose start --foreground
```

Common CLI commands:

```bash
./build/CadGoose start
./build/CadGoose spawn Pip
./build/CadGoose clear
./build/CadGoose ram
./build/CadGoose status
./build/CadGoose quit
```

### Wayland notes

The application uses `gtk4-layer-shell` to create overlay windows. Your compositor must support the `wlr-layer-shell-unstable-v1` protocol. Most wlroots-based compositors (Sway, Hyprland, river, niri) and KDE Plasma 6 support this. GNOME does not expose this protocol by default.

### X11 notes

The application can run under X11 via XWayland or a native X11 session. Overlay transparency and always-on-top behaviour depend on a compositing window manager being active (e.g., Picom, Compton, or a compositor built into your desktop environment).

---

## Cursor Backend Selection

At startup, `cursor_backend.cpp` attempts to initialise backends in this order:

1. **Hyprland** — detected via the `HYPRLAND_INSTANCE_SIGNATURE` environment variable. Communicates through the Hyprland IPC socket to read and set cursor position.
2. **wlroots** — uses the `zwlr_virtual_pointer_manager_v1` Wayland global to inject pointer motion events. Available on most wlroots compositors that expose the protocol.
3. **X11** — uses `XQueryPointer` to read position and `XTestFakeMotionEvent` to move the cursor. Works on native X11 sessions and XWayland.

If none of these backends initialise successfully, the application continues without cursor interaction features. All other goose behaviors remain active.

Only one goose can hold cursor control at a time, tracked globally via `g_cursorGrabberId`.

---

## Known Limitations

- **GNOME / Mutter**: The `wlr-layer-shell` protocol is not supported on stock GNOME. The application will not display overlay windows on GNOME without a third-party shell extension that exposes the protocol.
- **Multi-GPU / mixed DPI**: Monitor layout discovery relies on GTK4 monitor enumeration. Fractional scaling and mixed-DPI setups may produce minor positional drift in goose movement near monitor edges.
- **Wayland cursor snatching**: The wlroots virtual-pointer backend can inject relative motion but cannot read absolute cursor position without a secondary input mechanism. Absolute position queries fall back to the compositor IPC (Hyprland) or X11 on mixed sessions.
- **No Wayland screencopy**: Geese do not interact with window content. They walk over the top of windows without any awareness of what is underneath.

---

## Platform-Specific Files

- `src/platform/linux/` — Linux entry point, UI, cursor backends (Hyprland, wlroots, X11)
- `src/platform/macos/` — macOS entry point, AppKit windows, preferences UI
- `src/common/` — Shared game logic, behaviors, MCP, AI, config
- `src/common/behaviors/ai_http_client.mm` — AI chat HTTP client + function calling
- `src/common/mcp_server.cpp` — MCP server (Unix socket + HTTP)
- `src/common/mcp_handlers.cpp` — MCP tool/resource handlers
- `src/common/mcp_http_server.cpp` — HTTP transport for MCP
- `scripts/create_bundle.sh` — macOS app bundle builder
