# CadGoose

![CadGoose](Assets/Images/OtherGfx/CadGooseEvil.jpeg)

CadGoose is the more polite, publicly health-insured cousin of the Desktop Goose — a cheeky bird that lives on your desktop, waddles between your monitors, honks, steals your cursor, drops memes, and leaves muddy footprints. He's still a little bit belligerent.

Each goose lives in a transparent, click-through overlay above your normal windows. Run as many as you like — each has its own name and its own behavior.

---

## Platforms

| Platform | Status |
|---|---|
| **macOS** | **Supported** — native AppKit + Core Graphics. macOS 10.15+ (Apple Intelligence features need macOS 26+). |
| **Linux** | **Experimental** — GTK4 (Wayland / X11). May not build or run correctly; not recommended for daily use. Patches welcome. |

---

## Features

- **One or many geese** roaming transparent overlays across all your monitors, with smooth directional animation and fading footprint trails.
- **Cursor mischief** — geese chase your cursor and occasionally snatch and drag it.
- **Steals & drops** meme images and sticky-note messages, and pushes balls around.
- **Honks** on a timer (mutable from the menu bar).
- **AI chat** — click a goose to chat. Works with Apple's on-device Foundation model (no setup, macOS 26+) or your own local server (Osaurus / Ollama / custom endpoint).
- **Adjustable personality** — from a cuddly gosling to a villainous goose, via an "evil level" slider.
- **Multi-monitor** — geese discover all your displays at startup and roam across them.
- **21 toggleable behaviors** across five categories (below), each individually tunable.

### Behaviors

Toggle and tune each from **Preferences → Behaviors**:

- **Fun** — Ball, Breadcrumbs (hold RightShift to drop), Hats, Rainbow, Acid, Anger, Autumn Leaves
- **Joy** — Avoidance, Boredom Sigh, Window Peeking, Custom Affirmations, Interactive Drops, Toys
- **Control** — Honcker (press **F**), Jail (**O**/**P**), Portals (**1**/**2**/**0**), Drag
- **Info** — Nametag
- **Systems** — Health bar, AI chat, Pomodoro timer

---

## Install (macOS)

1. Download `CadGoose-<version>.dmg` from the [latest release](https://github.com/ztomer/CadGoose/releases/latest).
2. Open it and drag **CadGoose** into your Applications folder.
3. The first launch is blocked by Gatekeeper because the app isn't notarized by Apple yet. Clear the quarantine flag once:
   ```bash
   xattr -dr com.apple.quarantine /Applications/CadGoose.app
   ```
   Then open it normally (double-click). You only need to do this once. *(A future notarized build will remove this step.)*

### Alternatively: Install via Homebrew Cask (Automated quarantine removal)

If you prefer using Homebrew, you can install CadGoose directly using our custom Cask, which automatically strips the quarantine flag for you so the app launches instantly:

```bash
# Install directly from the repository cask file
brew install --cask tools/homebrew/cadgoose.rb
```

For setting up a custom GitHub Tap or hosting the formula publicly, refer to the [Homebrew Cask Guide](docs/HOMEBREW.md).

CadGoose runs in your **menu bar**. Use the menu to spawn geese, open Preferences, start a chat, mute honks, or quit.

---

## Using CadGoose

Open **Preferences** from the menu-bar icon. There are three tabs:

- **Behaviors** — enable/disable the behaviors listed above; each has a detail panel for size, speed, hotkeys, etc.
- **Appearance** — light / dark / system / custom mode; recolor the goose (body, neck, head, beak, eyes, outline) with a live preview; save and load color themes.
- **AI** — choose a provider (Foundation / Osaurus / Ollama / custom), pick a model, test the connection, and set the personality with the evil-level slider.

> **About the on-device AI:** Apple's Foundation model has a built-in safety filter that caps the persona around **"villainous" (≈72%)**. Above that it refuses, and CadGoose automatically dials the persona down so you still get a reply. For the full villain/dictator range with no cap, point the AI tab at **Osaurus** or **Ollama**. The app explains this inline.

---

## Reporting a bug

CadGoose writes logs to `~/Library/Application Support/CadGoose/logs/`:

- `crash-<timestamp>.log` — a backtrace, if the app crashed.
- `session-<timestamp>.log` — diagnostic output from the run.

Attach the most recent of these when [filing an issue](https://github.com/ztomer/CadGoose/issues).

---
---

# For developers

## Building from source

**macOS:**

```bash
./build.sh      # checks/installs Homebrew deps, builds build/CadGoose
./run.sh        # builds, then runs build/CadGoose
```

`build.sh` installs any missing dependencies via Homebrew (`cmake`, `ninja`, `googletest`, `mimalloc`); **toml11** is fetched at configure time, so a plain `git clone` builds — no submodules. Set `SKIP_DEPS=1` to skip the dependency check.

Run tests with `./build/CadGooseTests`, or `ctest` from the build dir (CI runs the same gate). **Linux:** see [docs/README_LINUX.md](docs/README_LINUX.md).

## Configuration file

Settings are normally edited through the Preferences window, and persisted to a `config.toml`:

- **macOS:** `~/Library/Application Support/CadGoose/config.toml`
- **Linux:** `$XDG_CONFIG_HOME/desktop-goose/config.toml` (falls back to `~/.config/desktop-goose/`)
- A `config/config.toml` in the working directory takes precedence (handy for development).

Selected keys:

| Key | Default | Description |
|---|---|---|
| `global_scale` | `1.0` | Base render scale for geese and dropped items |
| `audio_enabled` | `1` | Enable or disable honks |
| `cursor_chase_enabled` | `1` | Allow new geese to chase the cursor |
| `cursor_chase_chance` | `3` | Default chance for cursor-chase behavior |
| `mud_lifetime` | `15` | Footprint lifetime in seconds |
| `debug_visuals` | `0` | Draw debug hitboxes and state labels |

## Adding your own assets

- **Meme images** — drop `.png` files in `Assets/Images/Memes/`. Geese pick from these when fetching.
- **Notepad messages** — plain `.txt` files in `Assets/Text/NotepadMessages/`; each becomes a floating note a goose can carry.
- **Sound effects** — `Assets/Sound/NotEmbedded/` (WAV/OGG/MP3 via SDL2_mixer on Linux; AVFoundation on macOS).

## macOS bundle notes

Release `.app` bundles are **ad-hoc signed** (not yet notarized), which is why the `xattr` step above is needed. Ad-hoc signing also lacks the entitlements for Metal JIT shader compilation, so on some macOS versions a double-clicked bundle can fail with `Unable to reach MTLCompilerService`. A proper fix requires Developer ID signing + hardened runtime + the `com.apple.security.cs.allow-jit` entitlement. As a workaround you can run the raw binary directly:

```bash
./build/CadGoose
```

## Architecture & documentation

- [AGENTS.md](AGENTS.md) — current project state, build/run, architecture notes
- [docs/ARCH.md](docs/ARCH.md) — internal architecture (state machine, project structure, bundle)
- [docs/HOMEBREW.md](docs/HOMEBREW.md) — Homebrew Cask installation & tap instructions
- [docs/MCP.md](docs/MCP.md) — MCP protocol & AI chat command reference
- [docs/PROTOCOL.md](docs/PROTOCOL.md) — Unix-socket command protocol
- [docs/PLAN.md](docs/PLAN.md) — backlog and planned features
- [docs/README_LINUX.md](docs/README_LINUX.md) — Linux build instructions
- [.github/workflows/build_and_release.yml](.github/workflows/build_and_release.yml) — CI/CD

## Contributing

Contributions are welcome — see [AGENTS.md](AGENTS.md) for build instructions and project state.

## License

Released under the MIT License — see [LICENSE](LICENSE). Third-party assets under `Assets/` may carry their own licenses; review individual files before redistribution.

## Attribution & acknowledgements

- Highly modified port of [CppGoose](https://github.com/jeffthepineapple/desktop-goose-linux-port).
- Behavior system inspired by mods from the [Desktop Goose ResourceHub](https://desktopgooseunofficial.github.io/ResourceHub/mods/explore/mods.html).
- [Maple Mono](https://github.com/subframe7536/maple-font) — bundled UI font (SIL Open Font License 1.1).
- [toml11](https://github.com/ToruNiina/toml11) — C++ TOML parser (MIT License).
