// FoundationLLM.swift
// Swift wrapper for FoundationModels framework (macOS 26+)
// Exposes C-compatible functions for use from Objective-C++

import Foundation
#if canImport(FoundationModels)
import FoundationModels
#endif

// Log to stderr so messages appear alongside the app's other diagnostics
// (Swift's print() goes to stdout, which the app's logging doesn't capture).
private func fmLog(_ s: String) {
    FileHandle.standardError.write(("[FOUNDATION_LLM] " + s + "\n").data(using: .utf8)!)
}

// MARK: - C-compatible interface

@_cdecl("FoundationLLM_IsAvailable")
public func cIsAvailable() -> Int32 {
    return cAvailabilityCode() == 0 ? 1 : 0
}

// Returns a code describing why the FoundationModels backend is (un)available,
// so the UI can tell the user what to do rather than a generic failure:
//   0 = available
//   1 = framework not present (app built without the macOS 26 SDK; the OS
//       itself is never older, since 26 is the deployment floor, G0.14)
//   2 = device not eligible for Apple Intelligence
//   3 = Apple Intelligence not enabled in System Settings
//   4 = model still downloading / not ready
//   5 = unavailable for an unknown reason
@_cdecl("FoundationLLM_AvailabilityCode")
public func cAvailabilityCode() -> Int32 {
    #if canImport(FoundationModels)
    switch SystemLanguageModel.default.availability {
    case .available:
        return 0
    case .unavailable(let reason):
        switch reason {
        case .deviceNotEligible: return 2
        case .appleIntelligenceNotEnabled: return 3
        case .modelNotReady: return 4
        @unknown default: return 5
        }
    @unknown default:
        return 5
    }
    #else
    return 1
    #endif
}

@_cdecl("FoundationLLM_ContextSize")
public func cContextSize() -> Int32 {
    #if canImport(FoundationModels)
    return 4096  // default context size; actual model.contextSize not available on all SDKs
    #else
    return 0
    #endif
}

// Callback type: receives a C string and a context pointer
public typealias GenerateCallback = @convention(c) (UnsafePointer<CChar>?, UnsafeMutableRawPointer?) -> Void

@_cdecl("FoundationLLM_Generate")
public func cGenerate(prompt: UnsafePointer<CChar>, temperature: Float, callback: GenerateCallback?, context: UnsafeMutableRawPointer?) {
    #if canImport(FoundationModels)
    guard cIsAvailable() == 1 else {
        callback?("", context)
        return
    }

    let promptStr = String(cString: prompt)
    let model = SystemLanguageModel.default
    let session = LanguageModelSession(model: model)

    var options = GenerationOptions()
    options.temperature = Double(temperature)

    Task {
        do {
            let response = try await session.respond(to: promptStr, options: options)
            var content = response.content

            // Strip think...> blocks
            while let startRange = content.range(of: "<think>"),
                  let endRange = content.range(of: "</think>", range: startRange.upperBound..<content.endIndex) {
                content.removeSubrange(startRange.lowerBound..<endRange.upperBound)
            }

            // Trim whitespace
            content = content.trimmingCharacters(in: .whitespacesAndNewlines)

            // Truncate at first sentence-ending punctuation for meme texts
            let minLen = 10
            if let dot = content.firstIndex(of: "."), content.distance(from: content.startIndex, to: dot) > minLen {
                content = String(content[...dot])
            } else if let excl = content.firstIndex(of: "!"), content.distance(from: content.startIndex, to: excl) > minLen {
                content = String(content[...excl])
            } else if let qmark = content.firstIndex(of: "?"), content.distance(from: content.startIndex, to: qmark) > minLen {
                content = String(content[...qmark])
            }

            fmLog("Generated \(content.count) chars: \(content)")
            callback?(content, context)
        } catch {
            fmLog("Generation error: \(error)")
            callback?("", context)
        }
    }
    #else
    callback?("", context)
    #endif
}
