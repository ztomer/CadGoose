// FoundationLLM.swift
// Swift wrapper for FoundationModels framework (macOS 26+)
// Exposes C-compatible functions for use from Objective-C++

import Foundation
#if canImport(FoundationModels)
import FoundationModels
#endif

// MARK: - C-compatible interface

@_cdecl("FoundationLLM_IsAvailable")
public func cIsAvailable() -> Int32 {
    #if canImport(FoundationModels)
    if #available(macOS 26.0, *) {
        do {
            let model = try SystemLanguageModel.default
            return model.availability == .available ? 1 : 0
        } catch {
            return 0
        }
    }
    #endif
    return 0
}

@_cdecl("FoundationLLM_ContextSize")
public func cContextSize() -> Int32 {
    #if canImport(FoundationModels)
    if #available(macOS 26.0, *) {
        do {
            let model = try SystemLanguageModel.default
            return Int32(model.contextSize)
        } catch {
            return 0
        }
    }
    #endif
    return 0
}

// Callback type: receives a C string and a context pointer
public typealias GenerateCallback = @convention(c) (UnsafePointer<CChar>?, UnsafeMutableRawPointer?) -> Void

@_cdecl("FoundationLLM_Generate")
@available(macOS 26.0, *)
public func cGenerate(prompt: UnsafePointer<CChar>, temperature: Float, callback: GenerateCallback?, context: UnsafeMutableRawPointer?) {
    #if canImport(FoundationModels)
    guard cIsAvailable() == 1 else {
        callback?("", context)
        return
    }

    let promptStr = String(cString: prompt)

    do {
        let model = try SystemLanguageModel.default
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

                print("[FOUNDATION_LLM] Generated: \(content)")
                callback?(content, context)
            } catch {
                print("[FOUNDATION_LLM] Generation error: \(error.localizedDescription)")
                callback?("", context)
            }
        }
    } catch {
        print("[FOUNDATION_LLM] Failed to get model: \(error.localizedDescription)")
        callback?("", context)
    }
    #else
    callback?("", context)
    #endif
}
