# Plan — Detail Panel & AI Tab Redesign

## Current State

- Detail panel (config_gui_detail.mm): sliders with label+value side-by-side on single row
- AI tab (config_gui_ai.mm): Foundation note above prompt preview; text meme + auto-save as checkbox toggles; temperature slider below auto-save; "Enable debug status bar" in Advanced section
- `kDetailWidth` = 182px in config_gui.mm (very narrow — side-by-side sliders barely fit)

---

## 1. Stack sliders below descriptive text (Behaviors & Play detail panels)

**File:** `src/platform/macos/config_gui_detail.mm` — `addSliderWithLabel:`

### Current layout (single row):
```
[Label (left) | Value (right)]
[Slider (full width)]
```

### Target layout (two rows):
```
[Label (left-aligned) | Value (right-aligned)]
[Slider (full width, below text)]
```

### Changes

- Remove `sliderX` calculation that puts slider next to label
- Position label at `leftPad`, value at `rightPad` on same Y
- Position slider at `leftPad` on next Y (y - 22), width = `pw - leftPad - rightPad`
- Adjust Y decrement: each slider now takes ~42px (2 rows + gap) instead of 20px

```objc
// New addSliderWithLabel:
CGFloat labelW = [label sizeWithAttributes:font12].width + 10;
CGFloat valW = [@"100.00" sizeWithAttributes:valFont].width + 14;
CGFloat sliderX = leftPad;
CGFloat sliderW = pw - leftPad - 12;  // full width minus right margin
// Label row at y
// Slider row at y - 22
```

- Update all call sites (each slider now needs `y -= 42` instead of `y -= 30`)

---

## 2. AI panel: Move Foundation note below connection dropdown + rewrite text

**File:** `src/platform/macos/config_gui_ai.mm` — `setupUI` and `FoundationPersonaCapNote` (in ai_prompt_builder.mm)

### Current:
```
[Provider dropdown] [Port] [Model] [Refresh] [Test]
[Status label]
[Custom endpoint] (hidden for Foundation)
[Custom model] (hidden for Foundation)
[Foundation note]  ← ABOVE prompt preview
[PERSONALITY section] → Evil slider
[Prompt preview]
```

### Target:
```
[Provider dropdown] [Port] [Model] [Refresh] [Test]
[Status label]
[Custom endpoint] (hidden for Foundation)
[Custom model] (hidden for Foundation)
[PERSONALITY section] → Evil slider
[Prompt preview]
[Foundation note]  ← BELOW prompt preview (renamed/reworded)
```

### Text change

**Old:** `FoundationPersonaCapNote()` returns:
```
"Apple Foundation caps evil at 72% (villainous). Above 72% on-device safety refuses; requests sent at 72%. Osaurus/Ollama have no cap."
```

**New:**
```
"Foundation caps evil at 72%, use Osaurus/Ollama for max evil"
```

### Changes

- In `setupUI`: move Foundation note creation AFTER prompt preview, remove `y -= kFoundationNoteHeight` before prompt preview
- In `ai_prompt_builder.mm`: change `FoundationPersonaCapNote()` to return the shorter string
- Adjust Y gaps accordingly

---

## 3. Move temperature below personality (in AI tab)

**File:** `src/platform/macos/config_gui_ai.mm`

### Current order in TEXT MEME section:
```
[TEXT MEME section title]
[Generate text memes via AI] (toggle)
[Temperature label | Temperature value]
[Temperature slider]
[Auto-save generated texts] (toggle)
```

### Target:
```
[PERSONALITY section] → Evil slider
[TEXT MEME section] → "Generate text memes" toggle
[Auto-save generated texts] (toggle)  ← moved up
[Temperature label | Temperature value]
[Temperature slider]  ← moved below auto-save
```

Or: move entire Temperature row (label+slider) to BELOW the personality section, before TEXT MEME section. User says "move temperature below personality" — most logical is right after Personality section, before Text Meme.

### Changes

- Cut Temperature label/value/slider block (lines 400-430)
- Paste it after Personality section (after foundation note, before TEXT MEME section)
- Adjust Y positioning

---

## 4. Convert text meme & auto-save to toggles, align right

**File:** `src/platform/macos/config_gui_ai.mm`

### Current:
```objc
// Text meme (line 389)
NSButton* textMemeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kTextMemeBtnWidth, kToggleHeight)];
[textMemeBtn setButtonType:NSButtonTypeSwitch];
[textMemeBtn setTitle:@"Generate text memes via AI"];

// Auto-save (line 434)
NSButton* autoSaveBtn = [[NSButton alloc] initWithFrame:NSMakeRect(marginX, y, kAutoSaveBtnWidth, kToggleHeight)];
[autoSaveBtn setButtonType:NSButtonTypeSwitch];
[autoSaveBtn setTitle:@"Auto-save generated texts"];
```

These are already `NSButtonTypeSwitch` (checkboxes). But user wants them as **toggles (NSSwitch)** aligned right like the behavior list.

### Changes

- Change `setButtonType:NSButtonTypeSwitch` → use `NSSwitch`
- Position at right edge: `float toggleX = w - marginX - kToggleWidth;` (or similar)
- Remove title from NSSwitch (NSSwitch has no title — use separate label or keep checkbox style)
- Or: keep as checkbox but right-align — but user specifically said "convert to toggles"

Actually, looking at current code: both are already `NSButtonTypeSwitch` (checkbox style). The request is to use `NSSwitch` (pill toggle) like the main behavior list. But NSSwitch has no title. So either:
- Use NSSwitch at right edge, no title (label stays left)
- Or right-align the checkbox

Given the main list uses NSSwitch at right, I'll do: label left, NSSwitch right.

```objc
// Label on left
NSTextField* label = [[NSTextField alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2 - kToggleWidth - 8, kToggleHeight)];
label.stringValue = @"Generate text memes via AI";
// NSSwitch on right
float toggleX = w - marginX - kToggleWidth;
_toggle = [[NSSwitch alloc] initWithFrame:NSMakeRect(toggleX, y, kToggleWidth, kToggleHeight)];
```

---

## 5. Remove "Enable debug status bar" from panel

**File:** `src/platform/macos/config_gui_ai.mm` — lines 456-463

### Changes

- Remove the entire `showStatusBtn` block (lines 456-463)
- Keep `g_config.ai.showStatusBar` as a command-line option / config file setting only
- **Default value: OFF** (config registry default `false`)

Config option `ai.showStatusBar` exists in `config_registry_ai.cpp` — leave it but don't expose in UI. Ensure default is `false` so status bar is hidden unless explicitly enabled via CLI or config file edit.

---

## 6. Add glass panel behind prompt preview text

**File:** `src/platform/macos/config_gui_ai.mm` — line 364-374

### Current:
```objc
_promptBody = [[NSTextView alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kPromptBodyHeight)];
_promptBody.backgroundColor = [NSColor colorWithWhite:0.1 alpha:0.4];
_promptBody.wantsLayer = YES;
_promptBody.layer.cornerRadius = kPromptBodyCornerRadius;
```

### Target: Add NSVisualEffectView behind it (glass/material)

```objc
NSVisualEffectView* glass = [[NSVisualEffectView alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kPromptBodyHeight)];
glass.material = NSVisualEffectMaterialHudWindow; // or Sidebar, Popover
glass.blendingMode = NSVisualEffectBlendingModeBehindWindow;
glass.state = NSVisualEffectStateActive;
glass.wantsLayer = YES;
glass.layer.cornerRadius = kPromptBodyCornerRadius;
glass.layer.masksToBounds = YES;
[self addSubview:glass];

_promptBody = [[NSTextView alloc] initWithFrame:NSMakeRect(marginX, y, w - marginX*2, kPromptBodyHeight)];
_promptBody.backgroundColor = [NSColor clearColor];  // transparent, shows glass
_promptBody.wantsLayer = YES;
_promptBody.layer.cornerRadius = kPromptBodyCornerRadius;
_promptBody.layer.masksToBounds = YES;
[self addSubview:_promptBody];
```

---

## Verification

```bash
./build.sh
./build/CadGooseTests --gtest_filter="-WindowTrailTest.*:MCPIntegration*:LocalLLMTest*:AccessibilityGUITest.*"
```

Visual check:
1. Detail panels: sliders stacked below labels, full width
2. AI tab: Foundation note below prompt preview, reworded
3. Temperature slider below personality (or in TEXT MEME section moved up)
4. Text meme / auto-save: NSSwitch at right edge
5. No "Enable debug status bar" toggle in UI
6. Prompt preview has glass/translucent background

---

## Files Changed

| File | Scope |
|------|-------|
| `src/platform/macos/config_gui_detail.mm` | `addSliderWithLabel:` layout → stacked |
| `src/platform/macos/config_gui_ai.mm` | Foundation note move + reword; temperature move; toggles → NSSwitch right-aligned; remove debug status bar; glass behind prompt |
| `src/common/behaviors/ai_prompt_builder.mm` | `FoundationPersonaCapNote()` reworded |