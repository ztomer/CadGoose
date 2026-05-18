// config_gui_colors.mm
// AppearanceTabView — appearance mode selector + color editor + goose preview
#import "config_gui_helpers.h"
#include "config.h"
#include <filesystem>

bool LoadThemeFromFile(const std::string& path);
bool SaveThemeToFile(const std::string& path, const std::string& desc);

// Maps identifier (e.g. "customBodyR") to the corresponding ColorRGB field pointer
static float* ColorFieldForIdentifier(NSString* ident) {
    if (!ident || ![ident hasPrefix:@"custom"] || ident.length < 10) return nullptr;
    const char* s = ident.UTF8String;
    auto field = [&](ColorRGB& c, char channel) -> float* {
        if (s[ident.length - 1] != channel) return nullptr;
        return channel == 'R' ? &c.r : channel == 'G' ? &c.g : &c.b;
    };
    if ([ident hasSuffix:@"R"]) {
        if      ([ident hasPrefix:@"customBody"])    return &g_config.color.customBody.r;
        else if ([ident hasPrefix:@"customNeck"])    return &g_config.color.customNeck.r;
        else if ([ident hasPrefix:@"customHead"])    return &g_config.color.customHead.r;
        else if ([ident hasPrefix:@"customBeak"])    return &g_config.color.customBeak.r;
        else if ([ident hasPrefix:@"customEye"])     return &g_config.color.customEye.r;
        else if ([ident hasPrefix:@"customOutline"]) return &g_config.color.customOutline.r;
    } else if ([ident hasSuffix:@"G"]) {
        if      ([ident hasPrefix:@"customBody"])    return &g_config.color.customBody.g;
        else if ([ident hasPrefix:@"customNeck"])    return &g_config.color.customNeck.g;
        else if ([ident hasPrefix:@"customHead"])    return &g_config.color.customHead.g;
        else if ([ident hasPrefix:@"customBeak"])    return &g_config.color.customBeak.g;
        else if ([ident hasPrefix:@"customEye"])     return &g_config.color.customEye.g;
        else if ([ident hasPrefix:@"customOutline"]) return &g_config.color.customOutline.g;
    } else if ([ident hasSuffix:@"B"]) {
        if      ([ident hasPrefix:@"customBody"])    return &g_config.color.customBody.b;
        else if ([ident hasPrefix:@"customNeck"])    return &g_config.color.customNeck.b;
        else if ([ident hasPrefix:@"customHead"])    return &g_config.color.customHead.b;
        else if ([ident hasPrefix:@"customBeak"])    return &g_config.color.customBeak.b;
        else if ([ident hasPrefix:@"customEye"])     return &g_config.color.customEye.b;
        else if ([ident hasPrefix:@"customOutline"]) return &g_config.color.customOutline.b;
    }
    return nullptr;
}

#pragma mark -

@interface AppearanceTabView ()
@property (nonatomic, strong) NSPopUpButton* themePopup;
@property (nonatomic, strong) NSMutableArray* themePaths;
- (void)refreshThemeList;
@end

@implementation AppearanceTabView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    float y = self.bounds.size.height - 30;
    CGFloat w = self.bounds.size.width;

    // --- Appearance mode row ---
    NSTextField* appearanceLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y - 2, 80, 24)];
    appearanceLabel.stringValue = @"Appearance:";
    appearanceLabel.font = [NSFont fontWithName:@"Maple Mono" size:13] ?: [NSFont systemFontOfSize:13];
    appearanceLabel.textColor = [NSColor whiteColor];
    appearanceLabel.backgroundColor = [NSColor clearColor];
    appearanceLabel.bordered = NO;
    appearanceLabel.editable = NO;
    appearanceLabel.alignment = NSTextAlignmentLeft;
    [self addSubview:appearanceLabel];

    NSSegmentedControl* modeControl = [[NSSegmentedControl alloc] initWithFrame:NSMakeRect(96, y - 2, 180, 24)];
    modeControl.segmentCount = 3;
    [modeControl setLabel:@"System" forSegment:0];
    [modeControl setLabel:@"Dark" forSegment:1];
    [modeControl setLabel:@"Light" forSegment:2];
    int mode = g_config.general.appearanceMode;
    modeControl.selectedSegment = (mode == APPEARANCE_LIGHT) ? 2 :
                                  (mode == APPEARANCE_DARK)  ? 1 : 0;
    modeControl.target = self;
    modeControl.action = @selector(modeChanged:);
    [self addSubview:modeControl];

    _themePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(284, y - 2, 164, 24)];
    _themePopup.target = self;
    _themePopup.action = @selector(themeSelected:);
    [self addSubview:_themePopup];

    y -= 32;

    // Description
    NSTextField* desc = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, w - 24, 20)];
    desc.stringValue = @"Edit custom colors below; use themes to save/load color presets.";
    desc.font = [NSFont fontWithName:@"Maple Mono" size:12] ?: [NSFont systemFontOfSize:12];
    desc.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    desc.backgroundColor = [NSColor clearColor];
    desc.bordered = NO;
    desc.editable = NO;
    [self addSubview:desc];

    y -= 20;

    // --- RGB column headers (positioned above first color row) ---
    float headerY = y + 2;
    float labelW = 44;
    float colW = 128; // slider(90) + value(36) + gap(2)
    float sliderW = 90;
    float valW = 36;
    float col1X = 60;
    float col2X = col1X + colW;
    float col3X = col2X + colW;
    float swatchX = col3X + colW + 4;

    auto addColHeader = [&](float x, NSString* title, NSColor* color) {
        NSTextField* h = [[NSTextField alloc] initWithFrame:NSMakeRect(x, headerY, colW, 14)];
        h.stringValue = title; h.font = [NSFont fontWithName:@"Maple Mono" size:10] ?: [NSFont systemFontOfSize:10 weight:NSFontWeightSemibold];
        h.textColor = color; h.backgroundColor = [NSColor clearColor];
        h.bordered = NO; h.editable = NO; h.alignment = NSTextAlignmentCenter;
        [self addSubview:h];
    };
    addColHeader(col1X, @"R", [NSColor colorWithRed:1 green:0.4 blue:0.4 alpha:1]);
    addColHeader(col2X, @"G", [NSColor colorWithRed:0.4 green:1 blue:0.4 alpha:1]);
    addColHeader(col3X, @"B", [NSColor colorWithRed:0.4 green:0.4 blue:1 alpha:1]);

    y -= 22;

    // --- Goose preview positioned to the RIGHT of color selectors ---
    float previewX = swatchX + 30; // Right of swatches
    float previewW = w - previewX - 12; // Remaining width with margin
    float colorRowsHeight = 6 * 26 + 22; // 6 color rows + headers
    float previewY = y - colorRowsHeight + 20; // Align with top of color rows
    float previewH = colorRowsHeight - 30; // Slightly shorter than color section

    PreviewGooseView* preview = [[PreviewGooseView alloc] initWithFrame:NSMakeRect(previewX, previewY, previewW, previewH)];
    preview.identifier = @"goosePreview";
    [self addSubview:preview];

    // --- Color editor rows ---
    auto addColorRow = [&](NSString* label, float* rPtr, float* gPtr, float* bPtr, NSString* prefix, float yPos) -> float {
        NSTextField* lf = [[NSTextField alloc] initWithFrame:NSMakeRect(12, yPos, 60, 16)];
        lf.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
        lf.textColor = [NSColor whiteColor];
        lf.backgroundColor = [NSColor clearColor];
        lf.bordered = NO; lf.editable = NO;
        lf.stringValue = label;
        lf.alignment = NSTextAlignmentLeft;
        [self addSubview:lf];

        auto addSlider = [&](float colX, float* ptr, NSString* sid) {
            NSSlider* sl = [[NSSlider alloc] initWithFrame:NSMakeRect(colX, yPos, sliderW, 16)];
            sl.minValue = 0; sl.maxValue = 1; sl.doubleValue = *ptr;
            sl.identifier = sid;
            sl.target = self; sl.action = @selector(colorSliderChanged:);
            [self addSubview:sl];

            NSTextField* vf = [[NSTextField alloc] initWithFrame:NSMakeRect(colX + sliderW + 2, yPos, valW, 16)];
            vf.font = [NSFont fontWithName:@"Maple Mono" size:9] ?: [NSFont systemFontOfSize:9];
            vf.stringValue = [NSString stringWithFormat:@"%.2f", *ptr];
            vf.textColor = [NSColor whiteColor];
            vf.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0];
            vf.bordered = NO; vf.editable = YES;
            vf.identifier = sid;
            vf.target = self; vf.action = @selector(colorFieldChanged:);
            [self addSubview:vf];
        };

        addSlider(col1X, rPtr, [prefix stringByAppendingString:@"R"]);
        addSlider(col2X, gPtr, [prefix stringByAppendingString:@"G"]);
        addSlider(col3X, bPtr, [prefix stringByAppendingString:@"B"]);

        ColorSwatchView* swatch = [[ColorSwatchView alloc] initWithFrame:NSMakeRect(swatchX, yPos + 1, 14, 14)];
        swatch.color = [NSColor colorWithRed:*rPtr green:*gPtr blue:*bPtr alpha:1];
        swatch.colorPrefix = prefix;
        swatch.identifier = [@"swatch" stringByAppendingString:prefix];
        [self addSubview:swatch];

        return yPos - 26;
    };

    y = addColorRow(@"Body",    &g_config.color.customBody.r,    &g_config.color.customBody.g,    &g_config.color.customBody.b,    @"customBody",    y);
    y = addColorRow(@"Neck",    &g_config.color.customNeck.r,    &g_config.color.customNeck.g,    &g_config.color.customNeck.b,    @"customNeck",    y);
    y = addColorRow(@"Head",    &g_config.color.customHead.r,    &g_config.color.customHead.g,    &g_config.color.customHead.b,    @"customHead",    y);
    y = addColorRow(@"Beak",    &g_config.color.customBeak.r,    &g_config.color.customBeak.g,    &g_config.color.customBeak.b,    @"customBeak",    y);
    y = addColorRow(@"Eyes",    &g_config.color.customEye.r,     &g_config.color.customEye.g,     &g_config.color.customEye.b,     @"customEye",     y);
    y = addColorRow(@"Outline", &g_config.color.customOutline.r, &g_config.color.customOutline.g, &g_config.color.customOutline.b, @"customOutline", y);
    y -= 10;

    // --- Themes section separator ---
    NSView* themeSep = [[NSView alloc] initWithFrame:NSMakeRect(12, y, w - 24, 1)];
    themeSep.wantsLayer = YES;
    themeSep.layer.backgroundColor = [[NSColor separatorColor] CGColor];
    [self addSubview:themeSep];
    y -= 22;

    NSTextField* themeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 60, 16)];
    themeLabel.stringValue = @"Themes";
    themeLabel.font = [NSFont fontWithName:@"Maple Mono" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    themeLabel.textColor = [NSColor whiteColor];
    themeLabel.backgroundColor = [NSColor clearColor];
    themeLabel.bordered = NO;
    themeLabel.editable = NO;
    [self addSubview:themeLabel];
    y -= 28;

    NSButton* saveThemeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 170, 24)];
    [saveThemeBtn setTitle:@"Save Current as Theme\u2026"];
    [saveThemeBtn setFont:[NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11]];
    [saveThemeBtn setTarget:self];
    [saveThemeBtn setAction:@selector(saveTheme:)];
    saveThemeBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:saveThemeBtn];

    NSButton* openThemeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(190, y, 140, 24)];
    [openThemeBtn setTitle:@"Open Themes Folder"];
    [openThemeBtn setFont:[NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11]];
    [openThemeBtn setTarget:self];
    [openThemeBtn setAction:@selector(openThemesFolder:)];
    openThemeBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:openThemeBtn];
    y -= 30;

    [self refreshThemeList];
    Config_UpdateActiveTheme();
    [self updateSwatches];
    [self redrawGoosePreview];
}

- (void)modeChanged:(NSSegmentedControl*)sender {
    NSInteger idx = sender.selectedSegment;
    int mode = (idx == 2) ? APPEARANCE_LIGHT : (idx == 1) ? APPEARANCE_DARK : APPEARANCE_SYSTEM;
    g_config.general.appearanceMode = mode;
    Config_UpdateActiveTheme();
    [self redrawGoosePreview];
}

- (void)colorSliderChanged:(NSSlider*)sender {
    NSString* ident = sender.identifier;
    float* valPtr = ColorFieldForIdentifier(ident);
    if (!valPtr) return;

    float val = (float)sender.doubleValue;
    *valPtr = val;

    for (NSView* subview in self.subviews) {
        if ([subview.identifier isEqualToString:ident] && [subview isKindOfClass:[NSTextField class]]) {
            ((NSTextField*)subview).stringValue = [NSString stringWithFormat:@"%.2f", val];
            break;
        }
    }

    [self applyCustomToPreview];
}

- (void)colorFieldChanged:(NSTextField*)sender {
    NSString* ident = sender.identifier;
    float* valPtr = ColorFieldForIdentifier(ident);
    if (!valPtr) return;

    float val = (float)sender.doubleValue;
    *valPtr = val;

    for (NSView* subview in self.subviews) {
        if ([subview.identifier isEqualToString:ident] && [subview isKindOfClass:[NSSlider class]]) {
            ((NSSlider*)subview).doubleValue = val;
        }
    }
    [self applyCustomToPreview];
}

- (void)applyCustomToPreview {
    g_config.color.currentBody    = g_config.color.customBody;
    g_config.color.currentNeck    = g_config.color.customNeck;
    g_config.color.currentHead    = g_config.color.customHead;
    g_config.color.currentBeak    = g_config.color.customBeak;
    g_config.color.currentEye     = g_config.color.customEye;
    g_config.color.currentOutline = g_config.color.customOutline;
    [self updateSwatches];
    [self redrawGoosePreview];
}

- (void)updateSwatches {
    for (NSView* sv in self.subviews) {
        NSString* ident = sv.identifier;
        if ([ident hasPrefix:@"swatch"]) {
            NSString* prefix = [ident substringFromIndex:6];
            float* rPtr = ColorFieldForIdentifier([prefix stringByAppendingString:@"R"]);
            float* gPtr = ColorFieldForIdentifier([prefix stringByAppendingString:@"G"]);
            float* bPtr = ColorFieldForIdentifier([prefix stringByAppendingString:@"B"]);
            if (rPtr && gPtr && bPtr && [sv isKindOfClass:[ColorSwatchView class]]) {
                ((ColorSwatchView*)sv).color = [NSColor colorWithRed:*rPtr green:*gPtr blue:*bPtr alpha:1];
                [sv setNeedsDisplay:YES];
            }
        }
    }
}

- (void)redrawGoosePreview {
    for (NSView* subview in self.subviews) {
        if ([subview.identifier isEqualToString:@"goosePreview"] && [subview isKindOfClass:[PreviewGooseView class]]) {
            [(PreviewGooseView*)subview updatePreview];
            break;
        }
    }
}

#pragma mark - Themes

- (void)refreshThemeList {
    if (!_themePaths) _themePaths = [NSMutableArray array];
    [_themePaths removeAllObjects];
    [_themePopup removeAllItems];

    std::error_code ec;
    if (!std::filesystem::exists(Config_GetThemesDir(), ec)) return;

    for (auto& entry : std::filesystem::directory_iterator(Config_GetThemesDir(), ec)) {
        if (entry.path().extension() == ".toml") {
            NSString* name = [NSString stringWithUTF8String:entry.path().stem().c_str()];
            [_themePopup addItemWithTitle:name];
            [_themePaths addObject:[NSString stringWithUTF8String:entry.path().c_str()]];
        }
    }

    if (_themePaths.count == 0) {
        [_themePopup addItemWithTitle:@"No themes"];
        [[[_themePopup menu] itemAtIndex:[[_themePopup menu] numberOfItems] - 1] setEnabled:NO];
    }
}

- (void)themeSelected:(NSPopUpButton*)sender {
    NSInteger idx = sender.indexOfSelectedItem;
    if (idx < 0 || idx >= (NSInteger)_themePaths.count) return;
    NSString* path = _themePaths[idx];
    if (!LoadThemeFromFile(std::string([path UTF8String]))) return;

    // Update all sliders and value fields from loaded custom* values
    for (NSView* sv in self.subviews) {
        NSString* ident = sv.identifier;
        float* valPtr = ColorFieldForIdentifier(ident);
        if (!valPtr) continue;
        if ([sv isKindOfClass:[NSSlider class]])   ((NSSlider*)sv).doubleValue = *valPtr;
        if ([sv isKindOfClass:[NSTextField class]] && ![sv isKindOfClass:[ColorSwatchView class]])
            ((NSTextField*)sv).stringValue = [NSString stringWithFormat:@"%.2f", *valPtr];
    }
    [self applyCustomToPreview];
}

- (void)saveTheme:(id)sender {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Save Color Theme";
    alert.informativeText = @"Enter a name for this theme:";
    [alert addButtonWithTitle:@"Save"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* nameField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 250, 22)];
    nameField.stringValue = @"my_theme";
    [alert setAccessoryView:nameField];

    if ([alert runModal] != NSAlertFirstButtonReturn) return;

    NSString* themeName = nameField.stringValue ?: @"untitled";
    NSString* safeName = [[themeName componentsSeparatedByCharactersInSet:
                           [[NSCharacterSet alphanumericCharacterSet] invertedSet]]
                          componentsJoinedByString:@"_"];
    if (safeName.length == 0) safeName = @"theme";

    // Save directly to themes directory (NSSavePanel directoryURL is unreliable on modern macOS)
    auto themesDir = Config_GetThemesDir();
    auto filePath = themesDir / (std::string([safeName UTF8String]) + ".toml");
    if (SaveThemeToFile(filePath.string(), std::string([themeName UTF8String]))) {
        [self refreshThemeList];
        // Auto-select the newly saved theme
        for (NSInteger i = 0; i < (NSInteger)_themePaths.count; i++) {
            std::string p{[(NSString*)_themePaths[i] UTF8String]};
            if (p == filePath.string()) {
                [_themePopup selectItemAtIndex:i];
                break;
            }
        }
    }
}

- (void)openThemesFolder:(id)sender {
    std::error_code ec;
    auto dir = Config_GetThemesDir();
    if (!std::filesystem::exists(dir, ec)) {
        fprintf(stderr, "[Appearance] Themes dir does not exist: %s (ec=%s)\n",
                dir.string().c_str(), ec.message().c_str());
        return;
    }
    NSString* path = [NSString stringWithUTF8String:dir.string().c_str()];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:path]];
}

@end
