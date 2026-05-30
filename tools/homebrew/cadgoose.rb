cask "cadgoose" do
  version "0.5.2"
  sha256 "bea891bea99ec99b10ac95c4921b8292fddc8c9c334ce07f0b3613d0ae2235e3"


  url "https://github.com/ztomer/CadGoose/releases/download/v#{version}/CadGoose-v#{version}.dmg"
  name "CadGoose"
  desc "Agentic overlay companion (Desktop Goose clone with multi-goose, custom behaviors, AI chat, etc.)"
  homepage "https://github.com/ztomer/CadGoose"

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
