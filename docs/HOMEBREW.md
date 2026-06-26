# Installing CadGoose with Homebrew Cask

You can install CadGoose on macOS using a Homebrew Cask. This method automates downloading the DMG, copying the application bundle to your `/Applications` directory, and recursively stripping the macOS quarantine attribute so the app opens instantly on the first run without Gatekeeper blocks.

## How the Quarantine Removal Works

When you download unsigned applications on macOS, the OS applies an extended attribute called `com.apple.quarantine`, prompting a warning that the app "is damaged" or "comes from an unidentified developer."

The Homebrew Cask includes a `postflight` hook that executes recursively after copying the application to your machine:

```ruby
postflight do
  system_command "xattr",
                 args: ["-rd", "com.apple.quarantine", "#{appdir}/CadGoose.app"],
                 sudo: false
end
```

This automatically runs `/usr/bin/xattr -rd com.apple.quarantine /Applications/CadGoose.app` behind the scenes, whitelisting the app and letting you launch it instantly!

---

## Installation Methods

### Option 1: Direct Local Installation (Recommended for testing)

You can install CadGoose directly using the cask definition stored in this repository:

```bash
# Run from the root of the CadGoose repository
brew install --cask tools/homebrew/cadgoose.rb
```

### Option 2: Set Up a Custom GitHub Tap

If you want to host the installer on GitHub so anyone can install it with a simple `brew install`, follow these steps:

1. **Create a new GitHub repository** named `homebrew-tap` (or similar) under your username (e.g., `ztomer/homebrew-tap`). Homebrew taps must start with the prefix `homebrew-`.
2. **Add the cask file** to your repository under a `Casks` folder:
   - Create the directory: `Casks/`
   - Copy the cask file: `Casks/cadgoose.rb`
3. **Make the repository public**.
4. **Install it from anywhere** using the following commands:

```bash
# Tap your repository
brew tap ztomer/tap

# Install CadGoose
brew install --cask cadgoose
```

---

## Maintenance and Integrity Checks

By default, the Cask starts with `sha256 :no_check` to simplify initial testing and allow installing local custom builds. However, for formal public releases, it is highly recommended to enforce strict cryptographic SHA-256 integrity checks.

### Manual Update:
1. Download the release DMG.
2. Calculate the SHA-256 hash of the DMG file:
   ```bash
   shasum -a 256 CadGoose-v1.9.dmg
   ```
3. Open your tap's `Casks/cadgoose.rb` and replace `:no_check` or the old hash with the new hash string:
   ```ruby
   sha256 "your_generated_sha256_hash_here"
   ```

---

## Automated Updates via GitHub Actions

The release workflow (`.github/workflows/build_and_release.yml`) automatically updates the Homebrew tap on every release:

1. Builds macOS DMG + Linux tar.zst
2. Calculates DMG SHA-256
3. Clones `ztomer/homebrew-tap`
4. Updates `Casks/cadgoose.rb` with new version + SHA-256
5. Commits and pushes to tap

This runs on `release: published` and `workflow_dispatch` (for re-attaching artifacts). No manual steps needed.

### Requirements

The workflow expects:
- A `ztomer/homebrew-tap` repo with `Casks/cadgoose.rb`
- `HOMEBREW_TAP_TOKEN` secret in CadGoose repo (PAT with `repo` scope)

### Manual Override (if needed)

```bash
# Download DMG, compute hash
shasum -a 256 CadGoose-<version>.dmg

# Update Casks/cadgoose.rb in homebrew-tap:
#   version "<version>"
#   sha256 "<hash>"
```

---

## Uninstallation

To completely remove CadGoose along with all logs, configurations, and system plist preferences:

```bash
brew uninstall --cask cadgoose
```
