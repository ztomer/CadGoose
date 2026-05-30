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

By default, the cask uses `sha256 :no_check` to simplify updates and allow installing custom builds without rigid checksum validation. If you want to enforce strict integrity checks for public releases:

1. Download the release DMG.
2. Calculate the SHA-256 hash of the DMG file:
   ```bash
   shasum -a 256 CadGoose-v1.9.dmg
   ```
3. Open `cadgoose.rb` and replace `:no_check` with the generated hash string:
   ```ruby
   sha256 "your_generated_sha256_hash_here"
   ```

---

## Uninstallation

To completely remove CadGoose along with all logs, configurations, and system plist preferences:

```bash
brew uninstall --cask cadgoose
```
