cask "cadgoose" do
  version "0.5.3"
  sha256 "UPDATE_ME_AFTER_CI_BUILDS_DMG"


  url "https://github.com/ztomer/CadGoose/releases/download/v#{version}/CadGoose-v#{version}.dmg"
  name "CadGoose"
  desc "Agentic overlay companion (Desktop Goose clone with multi-goose, custom behaviors, AI chat, etc.)"
  homepage "https://github.com/ztomer/CadGoose"

  # What the binary is built for: Apple silicon (G0.5) and the macOS floor in
  # cmake/MacOSFloor.cmake (G0.14). A bare symbol is a minimum (">="); the
  # string form ">= :tahoe" is deprecated in Homebrew.
  depends_on arch: :arm64
  depends_on macos: :tahoe

  # The downloaded DMG contains the self-contained CadGoose.app bundle
  app "CadGoose.app"

  # Automatically strips the quarantine extended attribute upon installation
  # to bypass macOS Gatekeeper blocks without manual terminal intervention.
  postflight do
    system_command "xattr",
                   args: ["-rd", "com.apple.quarantine", "#{appdir}/CadGoose.app"],
                   sudo: false
  end

  # Cleans up all local application configurations, logs, and preferences on uninstall
  zap trash: [
    "~/Library/Application Support/CadGoose",
    "~/Library/Logs/CadGoose",
    "~/Library/Preferences/com.desktoppad.CadGoose.plist",
  ]
end
