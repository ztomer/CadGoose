// config_gui_colors.mm
// AppearanceTabView — appearance mode selector + color editor + goose preview
#import "config_gui_helpers.h"
#include "config.h"
#include <toml.hpp>
#include <fstream>

#pragma mark - ColorSwatchView with click -> NSColorPanel

@implementation ColorSwatchView

- (void)setColor:(NSColor*)color {
    _color = color;
    self.wantsLayer = YES;
    self.layer.backgroundColor = color.CGColor;
    self.layer.borderWidth = 1.0;
    self.layer.borderColor = [[NSColor colorWithWhite:0.5 alpha:0.5] CGColor];
}

- (void)mouseUp:(NSEvent*)event {
    [[NSColorPanel sharedColorPanel] setColor:self.color];
    [[NSColorPanel sharedColorPanel] setTarget:self];
    [[NSColorPanel sharedColorPanel] setAction:@selector(colorPickDone:)];
    [[NSColorPanel sharedColorPanel] orderFront:nil];
}

- (void)colorPickDone:(NSColorPanel*)sender {
    if (!self.colorPrefix) return;
    NSColor* c = [sender.color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
    CGFloat r, g, b, a;
    [c getRed:&r green:&g blue:&b alpha:&a];

    NSString* prefix = self.colorPrefix;
    NSString* rId = [prefix stringByAppendingString:@"R"];
    NSString* gId = [prefix stringByAppendingString:@"G"];
    NSString* bId = [prefix stringByAppendingString:@"B"];

    for (NSView* sv in self.superview.subviews) {
        if ([sv.identifier isEqualToString:rId]) {
            if ([sv isKindOfClass:[NSSlider class]]) {
                [(NSSlider*)sv setDoubleValue:r];
                [(id)sv sendActionOn:NSEventTypeLeftMouseUp];
                [(NSSlider*)sv sendAction:((NSSlider*)sv).action to:((NSSlider*)sv).target];
            } else if ([sv isKindOfClass:[NSTextField class]]) {
                [(NSTextField*)sv setStringValue:[NSString stringWithFormat:@"%.2f", r]];
            }
        } else if ([sv.identifier isEqualToString:gId]) {
            if ([sv isKindOfClass:[NSSlider class]]) {
                [(NSSlider*)sv setDoubleValue:g];
                [(id)sv sendActionOn:NSEventTypeLeftMouseUp];
                [(NSSlider*)sv sendAction:((NSSlider*)sv).action to:((NSSlider*)sv).target];
            } else if ([sv isKindOfClass:[NSTextField class]]) {
                [(NSTextField*)sv setStringValue:[NSString stringWithFormat:@"%.2f", g]];
            }
        } else if ([sv.identifier isEqualToString:bId]) {
            if ([sv isKindOfClass:[NSSlider class]]) {
                [(NSSlider*)sv setDoubleValue:b];
                [(id)sv sendActionOn:NSEventTypeLeftMouseUp];
                [(NSSlider*)sv sendAction:((NSSlider*)sv).action to:((NSSlider*)sv).target];
            } else if ([sv isKindOfClass:[NSTextField class]]) {
                [(NSTextField*)sv setStringValue:[NSString stringWithFormat:@"%.2f", b]];
            }
        }
    }
    self.color = [NSColor colorWithRed:r green:g blue:b alpha:1];
}
@end

#pragma mark - Theme file helpers

static NSString* ThemeDescriptionForFile(const std::string& path) {
    try {
        auto data = toml::parse(path);
        auto& theme = toml::find(data, "theme");
        return @(toml::find<std::string>(theme, "description").c_str());
    } catch (...) { return @""; }
}

static bool LoadThemeFromFile(const std::string& path) {
    try {
        auto data = toml::parse(path);
        auto& colors = toml::find(data, "colors");
        g_config.color.customBody.r    = toml::find<float>(colors, "body", "r");
        g_config.color.customBody.g    = toml::find<float>(colors, "body", "g");
        g_config.color.customBody.b    = toml::find<float>(colors, "body", "b");
        g_config.color.customNeck.r    = toml::find<float>(colors, "neck", "r");
        g_config.color.customNeck.g    = toml::find<float>(colors, "neck", "g");
        g_config.color.customNeck.b    = toml::find<float>(colors, "neck", "b");
        g_config.color.customHead.r    = toml::find<float>(colors, "head", "r");
        g_config.color.customHead.g    = toml::find<float>(colors, "head", "g");
        g_config.color.customHead.b    = toml::find<float>(colors, "head", "b");
        g_config.color.customBeak.r    = toml::find<float>(colors, "beak", "r");
        g_config.color.customBeak.g    = toml::find<float>(colors, "beak", "g");
        g_config.color.customBeak.b    = toml::find<float>(colors, "beak", "b");
        g_config.color.customEye.r     = toml::find<float>(colors, "eye", "r");
        g_config.color.customEye.g     = toml::find<float>(colors, "eye", "g");
        g_config.color.customEye.b     = toml::find<float>(colors, "eye", "b");
        g_config.color.customOutline.r = toml::find<float>(colors, "outline", "r");
        g_config.color.customOutline.g = toml::find<float>(colors, "outline", "g");
        g_config.color.customOutline.b = toml::find<float>(colors, "outline", "b");
        return true;
    } catch (...) { return false; }
}

static bool SaveThemeToFile(const std::string& path, const std::string& desc) {
    try {
        toml::table theme;
        theme["theme"]["description"] = desc;

        auto setColor = [&](const std::string& name, const ColorRGB& c) {
            theme["colors"][name] = toml::table{{"r", c.r}, {"g", c.g}, {"b", c.b}};
        };
        setColor("body",    g_config.color.customBody);
        setColor("neck",    g_config.color.customNeck);
        setColor("head",    g_config.color.customHead);
        setColor("beak",    g_config.color.customBeak);
        setColor("eye",     g_config.color.customEye);
        setColor("outline", g_config.color.customOutline);

        std::ofstream ofs(path);
        if (!ofs.is_open()) return false;
        ofs << toml::value(theme) << "\n";
        return true;
    } catch (...) { return false; }
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
    NSTextField* appearanceLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y + 2, 80, 20)];
    appearanceLabel.stringValue = @"Appearance:";
    appearanceLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13];
    appearanceLabel.textColor = [NSColor whiteColor];
    appearanceLabel.backgroundColor = [NSColor clearColor];
    appearanceLabel.bordered = NO;
    appearanceLabel.editable = NO;
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
    desc.font = [NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont systemFontOfSize:12];
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
        h.stringValue = title; h.font = [NSFont fontWithName:@"Comic Sans MS" size:10] ?: [NSFont systemFontOfSize:10 weight:NSFontWeightSemibold];
        h.textColor = color; h.backgroundColor = [NSColor clearColor];
        h.bordered = NO; h.editable = NO; h.alignment = NSTextAlignmentCenter;
        [self addSubview:h];
    };
    addColHeader(col1X, @"R", [NSColor colorWithRed:1 green:0.4 blue:0.4 alpha:1]);
    addColHeader(col2X, @"G", [NSColor colorWithRed:0.4 green:1 blue:0.4 alpha:1]);
    addColHeader(col3X, @"B", [NSColor colorWithRed:0.4 green:0.4 blue:1 alpha:1]);

    y -= 22;

    // --- Color editor rows ---
    auto addColorRow = [&](NSString* label, float* rPtr, float* gPtr, float* bPtr, NSString* prefix, float yPos) -> float {
        NSTextField* lf = [[NSTextField alloc] initWithFrame:NSMakeRect(12, yPos, labelW, 16)];
        lf.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
        lf.textColor = [NSColor whiteColor];
        lf.backgroundColor = [NSColor clearColor];
        lf.bordered = NO; lf.editable = NO;
        lf.stringValue = label;
        lf.alignment = NSTextAlignmentRight;
        [self addSubview:lf];

        auto addSlider = [&](float colX, float* ptr, NSString* sid) {
            NSSlider* sl = [[NSSlider alloc] initWithFrame:NSMakeRect(colX, yPos, sliderW, 16)];
            sl.minValue = 0; sl.maxValue = 1; sl.doubleValue = *ptr;
            sl.identifier = sid;
            sl.target = self; sl.action = @selector(colorSliderChanged:);
            [self addSubview:sl];

            NSTextField* vf = [[NSTextField alloc] initWithFrame:NSMakeRect(colX + sliderW + 2, yPos, valW, 16)];
            vf.font = [NSFont fontWithName:@"Comic Sans MS" size:9] ?: [NSFont systemFontOfSize:9];
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
    y -= 6;

    // --- Themes section separator ---
    NSView* themeSep = [[NSView alloc] initWithFrame:NSMakeRect(12, y, w - 24, 1)];
    themeSep.wantsLayer = YES;
    themeSep.layer.backgroundColor = [[NSColor separatorColor] CGColor];
    [self addSubview:themeSep];
    y -= 22;

    NSTextField* themeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 60, 16)];
    themeLabel.stringValue = @"Themes";
    themeLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:13] ?: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    themeLabel.textColor = [NSColor whiteColor];
    themeLabel.backgroundColor = [NSColor clearColor];
    themeLabel.bordered = NO;
    themeLabel.editable = NO;
    [self addSubview:themeLabel];
    y -= 28;

    NSButton* saveThemeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(12, y, 170, 24)];
    [saveThemeBtn setTitle:@"Save Current as Theme\u2026"];
    [saveThemeBtn setFont:[NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11]];
    [saveThemeBtn setTarget:self];
    [saveThemeBtn setAction:@selector(saveTheme:)];
    saveThemeBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:saveThemeBtn];
    y -= 30;

    PreviewGooseView* preview = [[PreviewGooseView alloc] initWithFrame:NSMakeRect(12, 10, 400, 100)];
    preview.identifier = @"goosePreview";
    [self addSubview:preview];

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
    if (!ident) return;
    float val = (float)sender.doubleValue;

    if      ([ident isEqualToString:@"customBodyR"])    g_config.color.customBody.r = val;
    else if ([ident isEqualToString:@"customBodyG"])    g_config.color.customBody.g = val;
    else if ([ident isEqualToString:@"customBodyB"])    g_config.color.customBody.b = val;
    else if ([ident isEqualToString:@"customNeckR"])    g_config.color.customNeck.r = val;
    else if ([ident isEqualToString:@"customNeckG"])    g_config.color.customNeck.g = val;
    else if ([ident isEqualToString:@"customNeckB"])    g_config.color.customNeck.b = val;
    else if ([ident isEqualToString:@"customHeadR"])    g_config.color.customHead.r = val;
    else if ([ident isEqualToString:@"customHeadG"])    g_config.color.customHead.g = val;
    else if ([ident isEqualToString:@"customHeadB"])    g_config.color.customHead.b = val;
    else if ([ident isEqualToString:@"customBeakR"])    g_config.color.customBeak.r = val;
    else if ([ident isEqualToString:@"customBeakG"])    g_config.color.customBeak.g = val;
    else if ([ident isEqualToString:@"customBeakB"])    g_config.color.customBeak.b = val;
    else if ([ident isEqualToString:@"customEyeR"])     g_config.color.customEye.r = val;
    else if ([ident isEqualToString:@"customEyeG"])     g_config.color.customEye.g = val;
    else if ([ident isEqualToString:@"customEyeB"])     g_config.color.customEye.b = val;
    else if ([ident isEqualToString:@"customOutlineR"]) g_config.color.customOutline.r = val;
    else if ([ident isEqualToString:@"customOutlineG"]) g_config.color.customOutline.g = val;
    else if ([ident isEqualToString:@"customOutlineB"]) g_config.color.customOutline.b = val;
    else return;

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
    if (!ident) return;
    float val = (float)sender.doubleValue;

    if      ([ident isEqualToString:@"customBodyR"])    g_config.color.customBody.r = val;
    else if ([ident isEqualToString:@"customBodyG"])    g_config.color.customBody.g = val;
    else if ([ident isEqualToString:@"customBodyB"])    g_config.color.customBody.b = val;
    else if ([ident isEqualToString:@"customNeckR"])    g_config.color.customNeck.r = val;
    else if ([ident isEqualToString:@"customNeckG"])    g_config.color.customNeck.g = val;
    else if ([ident isEqualToString:@"customNeckB"])    g_config.color.customNeck.b = val;
    else if ([ident isEqualToString:@"customHeadR"])    g_config.color.customHead.r = val;
    else if ([ident isEqualToString:@"customHeadG"])    g_config.color.customHead.g = val;
    else if ([ident isEqualToString:@"customHeadB"])    g_config.color.customHead.b = val;
    else if ([ident isEqualToString:@"customBeakR"])    g_config.color.customBeak.r = val;
    else if ([ident isEqualToString:@"customBeakG"])    g_config.color.customBeak.g = val;
    else if ([ident isEqualToString:@"customBeakB"])    g_config.color.customBeak.b = val;
    else if ([ident isEqualToString:@"customEyeR"])     g_config.color.customEye.r = val;
    else if ([ident isEqualToString:@"customEyeG"])     g_config.color.customEye.g = val;
    else if ([ident isEqualToString:@"customEyeB"])     g_config.color.customEye.b = val;
    else if ([ident isEqualToString:@"customOutlineR"]) g_config.color.customOutline.r = val;
    else if ([ident isEqualToString:@"customOutlineG"]) g_config.color.customOutline.g = val;
    else if ([ident isEqualToString:@"customOutlineB"]) g_config.color.customOutline.b = val;
    else return;

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
            ColorRGB* c = nullptr;
            if      ([prefix isEqualToString:@"customBody"])    c = &g_config.color.customBody;
            else if ([prefix isEqualToString:@"customNeck"])    c = &g_config.color.customNeck;
            else if ([prefix isEqualToString:@"customHead"])    c = &g_config.color.customHead;
            else if ([prefix isEqualToString:@"customBeak"])    c = &g_config.color.customBeak;
            else if ([prefix isEqualToString:@"customEye"])     c = &g_config.color.customEye;
            else if ([prefix isEqualToString:@"customOutline"]) c = &g_config.color.customOutline;
            if (c && [sv isKindOfClass:[ColorSwatchView class]]) {
                ((ColorSwatchView*)sv).color = [NSColor colorWithRed:c->r green:c->g blue:c->b alpha:1];
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
        if (![ident hasPrefix:@"custom"] || ident.length < 10) continue;
        float val = 0;
        if      ([ident isEqualToString:@"customBodyR"])    val = g_config.color.customBody.r;
        else if ([ident isEqualToString:@"customBodyG"])    val = g_config.color.customBody.g;
        else if ([ident isEqualToString:@"customBodyB"])    val = g_config.color.customBody.b;
        else if ([ident isEqualToString:@"customNeckR"])    val = g_config.color.customNeck.r;
        else if ([ident isEqualToString:@"customNeckG"])    val = g_config.color.customNeck.g;
        else if ([ident isEqualToString:@"customNeckB"])    val = g_config.color.customNeck.b;
        else if ([ident isEqualToString:@"customHeadR"])    val = g_config.color.customHead.r;
        else if ([ident isEqualToString:@"customHeadG"])    val = g_config.color.customHead.g;
        else if ([ident isEqualToString:@"customHeadB"])    val = g_config.color.customHead.b;
        else if ([ident isEqualToString:@"customBeakR"])    val = g_config.color.customBeak.r;
        else if ([ident isEqualToString:@"customBeakG"])    val = g_config.color.customBeak.g;
        else if ([ident isEqualToString:@"customBeakB"])    val = g_config.color.customBeak.b;
        else if ([ident isEqualToString:@"customEyeR"])     val = g_config.color.customEye.r;
        else if ([ident isEqualToString:@"customEyeG"])     val = g_config.color.customEye.g;
        else if ([ident isEqualToString:@"customEyeB"])     val = g_config.color.customEye.b;
        else if ([ident isEqualToString:@"customOutlineR"]) val = g_config.color.customOutline.r;
        else if ([ident isEqualToString:@"customOutlineG"]) val = g_config.color.customOutline.g;
        else if ([ident isEqualToString:@"customOutlineB"]) val = g_config.color.customOutline.b;
        else continue;
        if ([sv isKindOfClass:[NSSlider class]])   ((NSSlider*)sv).doubleValue = val;
        if ([sv isKindOfClass:[NSTextField class]] && ![sv isKindOfClass:[ColorSwatchView class]])
            ((NSTextField*)sv).stringValue = [NSString stringWithFormat:@"%.2f", val];
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

@end
