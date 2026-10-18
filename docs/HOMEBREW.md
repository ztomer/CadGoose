# Installing CadGoose with Homebrew Cask

The cask lives in [ztomer/homebrew-tap](https://github.com/ztomer/homebrew-tap)
(`Casks/cadgoose.rb`), not in this repository:

```bash
brew tap ztomer/tap
brew install --cask cadgoose
```

It downloads the release DMG, checks its SHA-256, copies `CadGoose.app` to
`/Applications` and removes the `com.apple.quarantine` attribute, so the
unsigned app opens on first launch without the Gatekeeper "damaged" prompt.
It requires Apple silicon and the macOS floor in `cmake/MacOSFloor.cmake`
(`depends_on arch: :arm64`, `depends_on macos:`).

There is no cask in this tree (G0.18): a copy here drifted to version 0.5.3
with a placeholder checksum, so the local install it was documented for
failed. `tools/check_macos_floor.py` refuses one, and the tap's own gate
(`tools/cask_floor.sh`) holds the cask's `depends_on macos:` to this repo's
floor at the tag the cask ships.

---

## How a release updates the tap

The release workflow (`.github/workflows/build_and_release.yml`) builds the
macOS DMG and the Linux tar.zst and attaches them to the GitHub release. It
does not touch the tap.

`scripts/push_release.sh` waits for that run, then bumps the cask with goh's
`tools/release-kit/tap_bump.sh`: it hashes the attached DMG, rewrites
`version` and `sha256` together, runs the tap's own `tools/gate.sh --full`
and pushes the commit pinned. A red tap gate pushes nothing. (Until
2026-10-17 a CI step pushed to the tap with a token, which skipped the tap's
gate; the `HOMEBREW_TAP_TOKEN` secret it used was deleted on 2026-10-17.)

### Manual Override (if needed)

```bash
"$GOH_DIR/tools/release-kit/tap_bump.sh" --tap ztomer/homebrew-tap --cask cadgoose \
  --tag v<version> --artifact https://github.com/ztomer/CadGoose/releases/download/v<version>/CadGoose-v<version>.dmg
```

---

## Uninstallation

To completely remove CadGoose along with all logs, configurations, and system plist preferences:

```bash
brew uninstall --cask cadgoose
```
