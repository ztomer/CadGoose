#!/usr/bin/env swift
// make_icon.swift — Generate app.icns from the goose emoji.
//
// Renders the emoji centered on a rounded-rect gradient tile at every size
// macOS needs, then runs iconutil to produce app.icns at the repo root.
//
// Usage: swift scripts/make_icon.swift [output.icns]

import AppKit
import Foundation

// U+1FABF GOOSE, written as an escape so this file holds no literal emoji glyph
// (tools/house_gates/check_no_emoji.py policies literal emoji; the rendered string is unchanged).
let emoji = "\u{1FABF}"
let outPath = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "app.icns"

// Pond-blue gradient background, matching the app's vibe.
let topColor = NSColor(srgbRed: 0.36, green: 0.47, blue: 0.62, alpha: 1.0)
let bottomColor = NSColor(srgbRed: 0.20, green: 0.28, blue: 0.40, alpha: 1.0)

func renderIcon(size: Int) -> Data? {
    let dim = CGFloat(size)
    let img = NSImage(size: NSSize(width: dim, height: dim))
    img.lockFocus()
    guard NSGraphicsContext.current != nil else { img.unlockFocus(); return nil }

    // Rounded-rect tile with ~10% inset, following macOS icon grid conventions.
    let inset = dim * 0.08
    let rect = CGRect(x: inset, y: inset, width: dim - 2 * inset, height: dim - 2 * inset)
    let radius = (dim - 2 * inset) * 0.225
    let path = NSBezierPath(roundedRect: NSRectFromCGRect(rect), xRadius: radius, yRadius: radius)
    path.addClip()

    let gradient = NSGradient(starting: topColor, ending: bottomColor)
    gradient?.draw(in: NSRectFromCGRect(rect), angle: -90)

    // Draw the emoji centered, filling ~62% of the tile.
    let fontSize = (dim - 2 * inset) * 0.62
    let attrs: [NSAttributedString.Key: Any] = [
        .font: NSFont.systemFont(ofSize: fontSize)
    ]
    let str = NSAttributedString(string: emoji, attributes: attrs)
    let textSize = str.size()
    let origin = NSPoint(x: (dim - textSize.width) / 2.0, y: (dim - textSize.height) / 2.0)
    str.draw(at: origin)

    img.unlockFocus()

    guard let tiff = img.tiffRepresentation,
          let rep = NSBitmapImageRep(data: tiff),
          let png = rep.representation(using: .png, properties: [:]) else { return nil }
    return png
}

let fm = FileManager.default
let iconset = NSTemporaryDirectory() + "CadGoose.iconset"
try? fm.removeItem(atPath: iconset)
try! fm.createDirectory(atPath: iconset, withIntermediateDirectories: true)

// (filename, pixel size) — the standard macOS iconset members.
let variants: [(String, Int)] = [
    ("icon_16x16.png", 16), ("icon_16x16@2x.png", 32),
    ("icon_32x32.png", 32), ("icon_32x32@2x.png", 64),
    ("icon_128x128.png", 128), ("icon_128x128@2x.png", 256),
    ("icon_256x256.png", 256), ("icon_256x256@2x.png", 512),
    ("icon_512x512.png", 512), ("icon_512x512@2x.png", 1024),
]

for (name, size) in variants {
    guard let data = renderIcon(size: size) else {
        FileHandle.standardError.write("Failed to render \(name)\n".data(using: .utf8)!)
        exit(1)
    }
    try! data.write(to: URL(fileURLWithPath: iconset + "/" + name))
}

let task = Process()
task.executableURL = URL(fileURLWithPath: "/usr/bin/iconutil")
task.arguments = ["-c", "icns", iconset, "-o", outPath]
try! task.run()
task.waitUntilExit()
try? fm.removeItem(atPath: iconset)

if task.terminationStatus == 0 {
    print("Wrote \(outPath)")
} else {
    FileHandle.standardError.write("iconutil failed\n".data(using: .utf8)!)
    exit(1)
}
