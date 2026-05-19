#ifndef RENDER_COLORS_H
#define RENDER_COLORS_H

#include "renderer_interface.h"

// Shared color constants for rendering across all behaviors and platforms.
// Eliminates duplicate hardcoded RGB values throughout the codebase.

// Toy colors
static constexpr float kStickBrownR = 0.55f;
static constexpr float kStickBrownG = 0.35f;
static constexpr float kStickBrownB = 0.15f;

static constexpr float kToyBallRedR = 0.8f;
static constexpr float kToyBallRedG = 0.2f;
static constexpr float kToyBallRedB = 0.2f;

// AI-generated item colors
static constexpr float kAIPaperCreamR = 0.96f;
static constexpr float kAIPaperCreamG = 0.94f;
static constexpr float kAIPaperCreamB = 0.88f;

// UI element colors
static constexpr float kNametagBgR = 0.15f;
static constexpr float kNametagBgG = 0.15f;
static constexpr float kNametagBgB = 0.15f;

static constexpr float kHealthBarBgR = 0.2f;
static constexpr float kHealthBarBgG = 0.2f;
static constexpr float kHealthBarBgB = 0.2f;

// Nature colors
static constexpr float kStemGreenR = 0.2f;
static constexpr float kStemGreenG = 0.6f;
static constexpr float kStemGreenB = 0.2f;

static constexpr float kFlowerCenterR = 1.0f;
static constexpr float kFlowerCenterG = 0.9f;
static constexpr float kFlowerCenterB = 0.3f;

static constexpr float kBreadcrumbGoldenR = 0.9f;
static constexpr float kBreadcrumbGoldenG = 0.7f;
static constexpr float kBreadcrumbGoldenB = 0.4f;

// Effect colors
static constexpr float kJailOrangeR = 1.0f;
static constexpr float kJailOrangeG = 0.6f;
static constexpr float kJailOrangeB = 0.0f;

static constexpr float kPeekEyeSkinR = 0.8f;
static constexpr float kPeekEyeSkinG = 0.7f;
static constexpr float kPeekEyeSkinB = 0.6f;

static constexpr float kSurpriseMarkR = 1.0f;
static constexpr float kSurpriseMarkG = 0.8f;
static constexpr float kSurpriseMarkB = 0.0f;

// Leaf color palette (4 seasonal colors)
static constexpr float kLeafColor1R = 0.4f;
static constexpr float kLeafColor1G = 0.7f;
static constexpr float kLeafColor1B = 0.2f;

static constexpr float kLeafColor2R = 0.8f;
static constexpr float kLeafColor2G = 0.6f;
static constexpr float kLeafColor2B = 0.1f;

static constexpr float kLeafColor3R = 0.9f;
static constexpr float kLeafColor3G = 0.3f;
static constexpr float kLeafColor3B = 0.1f;

static constexpr float kLeafColor4R = 0.6f;
static constexpr float kLeafColor4G = 0.4f;
static constexpr float kLeafColor4B = 0.2f;

// Helper to create RenderColor from constants
inline RenderColor MakeStickBrown(float alpha = 1.0f) { return RenderColor{kStickBrownR, kStickBrownG, kStickBrownB, alpha}; }
inline RenderColor MakeToyBallRed(float alpha = 1.0f) { return RenderColor{kToyBallRedR, kToyBallRedG, kToyBallRedB, alpha}; }
inline RenderColor MakeAIPaperCream(float alpha = 1.0f) { return RenderColor{kAIPaperCreamR, kAIPaperCreamG, kAIPaperCreamB, alpha}; }
inline RenderColor MakeNametagBg(float alpha = 1.0f) { return RenderColor{kNametagBgR, kNametagBgG, kNametagBgB, alpha}; }
inline RenderColor MakeHealthBarBg(float alpha = 1.0f) { return RenderColor{kHealthBarBgR, kHealthBarBgG, kHealthBarBgB, alpha}; }
inline RenderColor MakeStemGreen(float alpha = 1.0f) { return RenderColor{kStemGreenR, kStemGreenG, kStemGreenB, alpha}; }
inline RenderColor MakeFlowerCenter(float alpha = 1.0f) { return RenderColor{kFlowerCenterR, kFlowerCenterG, kFlowerCenterB, alpha}; }
inline RenderColor MakeBreadcrumbGolden(float alpha = 1.0f) { return RenderColor{kBreadcrumbGoldenR, kBreadcrumbGoldenG, kBreadcrumbGoldenB, alpha}; }
inline RenderColor MakeJailOrange(float alpha = 1.0f) { return RenderColor{kJailOrangeR, kJailOrangeG, kJailOrangeB, alpha}; }
inline RenderColor MakePeekEyeSkin(float alpha = 1.0f) { return RenderColor{kPeekEyeSkinR, kPeekEyeSkinG, kPeekEyeSkinB, alpha}; }
inline RenderColor MakeSurpriseMark(float alpha = 1.0f) { return RenderColor{kSurpriseMarkR, kSurpriseMarkG, kSurpriseMarkB, alpha}; }

#endif // RENDER_COLORS_H
