// config_gui_colors.mm
// AppearanceTabView — appearance mode selector + color editor + goose preview
#import "config_gui_helpers.h"
#include "config.h"
#include <toml.hpp>
#include <fstream>

@implementation ColorSwatchView
- (void)drawRect:(NSRect)dirtyRect {
    [_color setFill];
    NSRectFill(dirtyRect);
    [[NSColor colorWithWhite:0.5 alpha:0.5] setStroke];
    NSFrameRectWithWidth(self.bounds, 1);
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
@property (nonatomic, strong) NSTextField* themeDescField;
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

    // Appearance mode selector
    NSTextField* appearanceLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y + 2, 80, 20)];
    appearanceLabel.stringValue = @"Appearance:";
    appearanceLabel.font = [NSFont systemFontOfSize:13];
    appearanceLabel.textColor = [NSColor whiteColor];
    appearanceLabel.backgroundColor = [NSColor clearColor];
    appearanceLabel.bordered = NO;
    appearanceLabel.editable = NO;
    [self addSubview:appearanceLabel];

    NSPopUpButton* appearancePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(96, y - 2, 130, 24)];
    [appearancePopup addItemWithTitle:@"Light"];
    [appearancePopup addItemWithTitle:@"Dark"];
    [appearancePopup addItemWithTitle:@"System"];
    [appearancePopup addItemWithTitle:@"Custom"];
    [appearancePopup selectItemAtIndex:g_config.general.appearanceMode];
    appearancePopup.target = self;
    appearancePopup.action = @selector(appearanceChanged:);
    [self addSubview:appearancePopup];

    y -= 40;

    // Description
    NSTextField* desc = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, w - 24, 20)];
    desc.stringValue = @"🎨 Customize goose colors for Custom appearance mode";
    desc.font = [NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont systemFontOfSize:12];
    desc.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    desc.backgroundColor = [NSColor clearColor];
    desc.bordered = NO;
    desc.editable = NO;
    [self addSubview:desc];

    y -= 10;

    y = [self addColorTripleWithLabel:@"Body" atY:y
                                rPtr:&g_config.color.customBody.r
                                gPtr:&g_config.color.customBody.g
                                bPtr:&g_config.color.customBody.b
                              prefix:@"customBody"];

    y = [self addColorTripleWithLabel:@"Neck" atY:y
                                rPtr:&g_config.color.customNeck.r
                                gPtr:&g_config.color.customNeck.g
                                bPtr:&g_config.color.customNeck.b
                              prefix:@"customNeck"];

    y = [self addColorTripleWithLabel:@"Head" atY:y
                                rPtr:&g_config.color.customHead.r
                                gPtr:&g_config.color.customHead.g
                                bPtr:&g_config.color.customHead.b
                              prefix:@"customHead"];

    y = [self addColorTripleWithLabel:@"Beak" atY:y
                                rPtr:&g_config.color.customBeak.r
                                gPtr:&g_config.color.customBeak.g
                                bPtr:&g_config.color.customBeak.b
                              prefix:@"customBeak"];

    y = [self addColorTripleWithLabel:@"Eyes" atY:y
                                rPtr:&g_config.color.customEye.r
                                gPtr:&g_config.color.customEye.g
                                bPtr:&g_config.color.customEye.b
                              prefix:@"customEye"];

    y = [self addColorTripleWithLabel:@"Outline" atY:y
                                rPtr:&g_config.color.customOutline.r
                                gPtr:&g_config.color.customOutline.g
                                bPtr:&g_config.color.customOutline.b
                              prefix:@"customOutline"];

    y -= 16;
    AppBarBorderView* themeSep = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(12, y, w - 24, 1)];
    [self addSubview:themeSep];
    y -= 22;

    NSTextField* themeLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 60, 16)];
    themeLabel.stringValue = @"Themes";
    themeLabel.font = [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold];
    themeLabel.textColor = [NSColor whiteColor];
    themeLabel.backgroundColor = [NSColor clearColor];
    themeLabel.bordered = NO;
    themeLabel.editable = NO;
    [self addSubview:themeLabel];
    y -= 24;

    _themeDescField = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, w - 24, 22)];
    _themeDescField.font = [NSFont systemFontOfSize:11];
    _themeDescField.textColor = [NSColor whiteColor];
    _themeDescField.backgroundColor = [NSColor clearColor];
    _themeDescField.bordered = NO;
    _themeDescField.editable = YES;
    _themeDescField.bezelStyle = NSTextFieldRoundedBezel;
    _themeDescField.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    _themeDescField.placeholderString = @"Theme description";
    _themeDescField.target = self;
    _themeDescField.action = @selector(themeDescChanged:);
    [self addSubview:_themeDescField];
    y -= 28;

    _themePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(12, y, 200, 24)];
    _themePopup.target = self;
    _themePopup.action = @selector(themeSelected:);
    [self addSubview:_themePopup];

    NSButton* saveThemeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(220, y, 110, 24)];
    [saveThemeBtn setTitle:@"Save As..."];
    [saveThemeBtn setFont:[NSFont systemFontOfSize:11]];
    [saveThemeBtn setTarget:self];
    [saveThemeBtn setAction:@selector(saveTheme:)];
    saveThemeBtn.bezelStyle = NSBezelStyleRounded;
    [self addSubview:saveThemeBtn];

    [self refreshThemeList];

    PreviewGooseView* preview = [[PreviewGooseView alloc] initWithFrame:NSMakeRect(12, 10, 400, 80)];
    preview.identifier = @"goosePreview";
    [self addSubview:preview];
}

- (void)appearanceChanged:(NSPopUpButton*)sender {
    g_config.general.appearanceMode = (int)sender.indexOfSelectedItem;
}

- (float)addColorTripleWithLabel:(NSString*)label atY:(float)y
                            rPtr:(float*)rPtr gPtr:(float*)gPtr bPtr:(float*)bPtr
                          prefix:(NSString*)prefix {
    NSTextField* labelField = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 44, 16)];
    labelField.font = [NSFont systemFontOfSize:11];
    labelField.textColor = [NSColor whiteColor];
    labelField.backgroundColor = [NSColor clearColor];
    labelField.bordered = NO;
    labelField.editable = NO;
    labelField.stringValue = label;
    labelField.alignment = NSTextAlignmentRight;
    [self addSubview:labelField];

    NSSlider* rSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(60, y, 90, 16)];
    rSlider.minValue = 0; rSlider.maxValue = 1; rSlider.doubleValue = *rPtr;
    rSlider.identifier = [prefix stringByAppendingString:@"R"];
    rSlider.target = self; rSlider.action = @selector(colorSliderChanged:);
    [self addSubview:rSlider];

    NSTextField* rVal = [[NSTextField alloc] initWithFrame:NSMakeRect(150, y, 30, 16)];
    rVal.font = [NSFont systemFontOfSize:9]; rVal.stringValue = [NSString stringWithFormat:@"%.2f", *rPtr];
    rVal.textColor = [NSColor whiteColor]; rVal.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0]; rVal.bordered = NO; rVal.editable = YES;
    rVal.identifier = rSlider.identifier; rVal.target = self; rVal.action = @selector(colorFieldChanged:);
    [self addSubview:rVal];

    NSSlider* gSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(184, y, 90, 16)];
    gSlider.minValue = 0; gSlider.maxValue = 1; gSlider.doubleValue = *gPtr;
    gSlider.identifier = [prefix stringByAppendingString:@"G"];
    gSlider.target = self; gSlider.action = @selector(colorSliderChanged:);
    [self addSubview:gSlider];

    NSTextField* gVal = [[NSTextField alloc] initWithFrame:NSMakeRect(274, y, 30, 16)];
    gVal.font = [NSFont systemFontOfSize:9]; gVal.stringValue = [NSString stringWithFormat:@"%.2f", *gPtr];
    gVal.textColor = [NSColor whiteColor]; gVal.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0]; gVal.bordered = NO; gVal.editable = YES;
    gVal.identifier = gSlider.identifier; gVal.target = self; gVal.action = @selector(colorFieldChanged:);
    [self addSubview:gVal];

    NSSlider* bSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(308, y, 90, 16)];
    bSlider.minValue = 0; bSlider.maxValue = 1; bSlider.doubleValue = *bPtr;
    bSlider.identifier = [prefix stringByAppendingString:@"B"];
    bSlider.target = self; bSlider.action = @selector(colorSliderChanged:);
    [self addSubview:bSlider];

    NSTextField* bVal = [[NSTextField alloc] initWithFrame:NSMakeRect(398, y, 30, 16)];
    bVal.font = [NSFont systemFontOfSize:9]; bVal.stringValue = [NSString stringWithFormat:@"%.2f", *bPtr];
    bVal.textColor = [NSColor whiteColor]; bVal.backgroundColor = [NSColor colorWithWhite:0.15 alpha:1.0]; bVal.bordered = NO; bVal.editable = YES;
    bVal.identifier = bSlider.identifier; bVal.target = self; bVal.action = @selector(colorFieldChanged:);
    [self addSubview:bVal];

    ColorSwatchView* swatch = [[ColorSwatchView alloc] initWithFrame:NSMakeRect(434, y + 1, 14, 14)];
    swatch.color = [NSColor colorWithRed:*rPtr green:*gPtr blue:*bPtr alpha:1];
    swatch.identifier = [@"swatch" stringByAppendingString:prefix];
    [self addSubview:swatch];

    return y - 28;
}

- (void)colorSliderChanged:(NSSlider*)sender {
    NSString* ident = sender.identifier;
    if (!ident) return;
    float val = (float)sender.doubleValue;

    if      ([ident isEqualToString:@"customBodyR"]) g_config.color.customBody.r = val;
    else if ([ident isEqualToString:@"customBodyG"]) g_config.color.customBody.g = val;
    else if ([ident isEqualToString:@"customBodyB"]) g_config.color.customBody.b = val;
    else if ([ident isEqualToString:@"customNeckR"]) g_config.color.customNeck.r = val;
    else if ([ident isEqualToString:@"customNeckG"]) g_config.color.customNeck.g = val;
    else if ([ident isEqualToString:@"customNeckB"]) g_config.color.customNeck.b = val;
    else if ([ident isEqualToString:@"customHeadR"]) g_config.color.customHead.r = val;
    else if ([ident isEqualToString:@"customHeadG"]) g_config.color.customHead.g = val;
    else if ([ident isEqualToString:@"customHeadB"]) g_config.color.customHead.b = val;
    else if ([ident isEqualToString:@"customBeakR"]) g_config.color.customBeak.r = val;
    else if ([ident isEqualToString:@"customBeakG"]) g_config.color.customBeak.g = val;
    else if ([ident isEqualToString:@"customBeakB"]) g_config.color.customBeak.b = val;
    else if ([ident isEqualToString:@"customEyeR"]) g_config.color.customEye.r = val;
    else if ([ident isEqualToString:@"customEyeG"]) g_config.color.customEye.g = val;
    else if ([ident isEqualToString:@"customEyeB"]) g_config.color.customEye.b = val;
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

    [self updateColorSwatches];
    [self redrawGoosePreview];
}

- (void)colorFieldChanged:(NSTextField*)sender {
    NSString* ident = sender.identifier;
    if (!ident) return;
    float val = (float)sender.doubleValue;

    if      ([ident isEqualToString:@"customBodyR"]) g_config.color.customBody.r = val;
    else if ([ident isEqualToString:@"customBodyG"]) g_config.color.customBody.g = val;
    else if ([ident isEqualToString:@"customBodyB"]) g_config.color.customBody.b = val;
    else if ([ident isEqualToString:@"customNeckR"]) g_config.color.customNeck.r = val;
    else if ([ident isEqualToString:@"customNeckG"]) g_config.color.customNeck.g = val;
    else if ([ident isEqualToString:@"customNeckB"]) g_config.color.customNeck.b = val;
    else if ([ident isEqualToString:@"customHeadR"]) g_config.color.customHead.r = val;
    else if ([ident isEqualToString:@"customHeadG"]) g_config.color.customHead.g = val;
    else if ([ident isEqualToString:@"customHeadB"]) g_config.color.customHead.b = val;
    else if ([ident isEqualToString:@"customBeakR"]) g_config.color.customBeak.r = val;
    else if ([ident isEqualToString:@"customBeakG"]) g_config.color.customBeak.g = val;
    else if ([ident isEqualToString:@"customBeakB"]) g_config.color.customBeak.b = val;
    else if ([ident isEqualToString:@"customEyeR"]) g_config.color.customEye.r = val;
    else if ([ident isEqualToString:@"customEyeG"]) g_config.color.customEye.g = val;
    else if ([ident isEqualToString:@"customEyeB"]) g_config.color.customEye.b = val;
    else if ([ident isEqualToString:@"customOutlineR"]) g_config.color.customOutline.r = val;
    else if ([ident isEqualToString:@"customOutlineG"]) g_config.color.customOutline.g = val;
    else if ([ident isEqualToString:@"customOutlineB"]) g_config.color.customOutline.b = val;
    else return;

    for (NSView* subview in self.subviews) {
        if ([subview.identifier isEqualToString:ident] && [subview isKindOfClass:[NSSlider class]]) {
            ((NSSlider*)subview).doubleValue = val;
        }
    }
    [self updateColorSwatches];
    [self redrawGoosePreview];
}

- (void)updateColorSwatches {
    NSDictionary* swatchMap = @{
        @"swatchcustomBody":    [NSColor colorWithRed:g_config.color.customBody.r green:g_config.color.customBody.g blue:g_config.color.customBody.b alpha:1],
        @"swatchcustomNeck":    [NSColor colorWithRed:g_config.color.customNeck.r green:g_config.color.customNeck.g blue:g_config.color.customNeck.b alpha:1],
        @"swatchcustomHead":    [NSColor colorWithRed:g_config.color.customHead.r green:g_config.color.customHead.g blue:g_config.color.customHead.b alpha:1],
        @"swatchcustomBeak":    [NSColor colorWithRed:g_config.color.customBeak.r green:g_config.color.customBeak.g blue:g_config.color.customBeak.b alpha:1],
        @"swatchcustomEye":     [NSColor colorWithRed:g_config.color.customEye.r green:g_config.color.customEye.g blue:g_config.color.customEye.b alpha:1],
        @"swatchcustomOutline": [NSColor colorWithRed:g_config.color.customOutline.r green:g_config.color.customOutline.g blue:g_config.color.customOutline.b alpha:1],
    };
    for (NSView* subview in self.subviews) {
        NSColor* color = swatchMap[subview.identifier];
        if (color && [subview isKindOfClass:[ColorSwatchView class]]) {
            ((ColorSwatchView*)subview).color = color;
            [subview setNeedsDisplay:YES];
        }
    }
}

- (void)redrawGoosePreview {
    for (NSView* subview in self.subviews) {
        if ([subview.identifier isEqualToString:@"goosePreview"] && [subview isKindOfClass:[PreviewGooseView class]]) {
            [subview setNeedsDisplay:YES];
            break;
        }
    }
}

#pragma mark - Themes

- (void)refreshThemeList {
    if (!_themePaths) _themePaths = [NSMutableArray array];
    [_themePaths removeAllObjects];
    [_themePopup removeAllItems];

    std::string dir = Config_GetThemesDir().string();
    for (auto& entry : std::filesystem::directory_iterator(Config_GetThemesDir())) {
        if (entry.path().extension() == ".toml") {
            NSString* desc = ThemeDescriptionForFile(entry.path().string());
            NSString* name = [NSString stringWithUTF8String:entry.path().stem().c_str()];
            NSString* label = desc.length > 0 ? [NSString stringWithFormat:@"%@ — %@", name, desc] : name;
            [_themePopup addItemWithTitle:label];
            [_themePaths addObject:[NSString stringWithUTF8String:entry.path().c_str()]];
        }
    }
}

- (void)themeSelected:(NSPopUpButton*)sender {
    NSInteger idx = sender.indexOfSelectedItem;
    if (idx < 0 || idx >= (NSInteger)_themePaths.count) return;
    NSString* path = _themePaths[idx];
    if (!LoadThemeFromFile(std::string([path UTF8String]))) return;

    _themeDescField.stringValue = ThemeDescriptionForFile(std::string([path UTF8String]));
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
    [self updateColorSwatches];
    [self redrawGoosePreview];
}

- (void)saveTheme:(id)sender {
    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.title = @"Save Color Theme";
    panel.nameFieldStringValue = @"my_theme.toml";
    panel.directoryURL = [NSURL fileURLWithPath:@(Config_GetThemesDir().c_str())];
    panel.allowedFileTypes = @[@"toml"];

    if ([panel runModal] == NSModalResponseOK) {
        NSString* path = panel.URL.path;
        NSString* desc = _themeDescField.stringValue ?: @"";
        if (SaveThemeToFile(std::string([path UTF8String]), std::string([desc UTF8String]))) {
            [self refreshThemeList];
        }
    }
}

- (void)themeDescChanged:(NSTextField*)sender {
    // Description is stored in file on next save; changes are held in memory
}

@end
