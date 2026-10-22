#ifndef APP_IDENTITY_H
#define APP_IDENTITY_H

// The app's bundle identifier, written down once.
//
// Two things need it and they must agree: the macOS bundle's Info.plist
// (scripts/create_bundle.sh reads this file) and the app's own settings
// directory (ConfigDirPath). Two apps that share one settings directory share
// one config.toml, and the schemas of the two apps have diverged (ROADMAP
// D1.1), so the directory is named after the bundle id: every app gets its
// own.
//
// Keep this line in the shape the shell and the gate read:
//     #define CADGOOSE_BUNDLE_ID "..."
// tools/check_config_dir_isolation.py fails if it changes shape, if the id is
// not reverse-DNS (a bare "CadGoose" would be the shared directory again), or
// if the bundler stops reading it from here.
#define CADGOOSE_BUNDLE_ID "com.desktoppad.CadGoose"

#endif // APP_IDENTITY_H
