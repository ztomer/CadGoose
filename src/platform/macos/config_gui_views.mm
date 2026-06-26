// config_gui_views.mm
// BehaviorRowView and PreviewGooseView implementations
#import "config_gui_helpers.h"
#include "config.h"

// BehaviorRowView layout constants
static constexpr float kNameLabelX = 12.0f;
static constexpr float kNameLabelY = 7.0f;
static constexpr float kNameLabelWidth = 140.0f;
static constexpr float kNameLabelHeight = 18.0f;
static constexpr float kNameFontSize = 14.0f;
static constexpr float kDescLabelX = 154.0f;
static constexpr float kDescLabelY = 9.0f;
static constexpr float kDescLabelWidth = 210.0f;
static constexpr float kDescLabelHeight = 14.0f;
static constexpr float kDescFontSize = 11.0f;
static constexpr float kToggleWidth = 44.0f;
static constexpr float kToggleHeight = 22.0f;
static constexpr float kToggleY = 3.0f;
static constexpr float kToggleRightPad = 28.0f;
static constexpr float kSeparatorX = 8.0f;
static constexpr float kSeparatorWidth = 1.0f;
static constexpr float kSeparatorInset = 16.0f;
static constexpr float kHighlightInset = 4.0f;
static constexpr float kHighlightRadius = 6.0f;
static constexpr float kHighlightAlpha = 0.08f;

@implementation BehaviorRowView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kNameLabelX, kNameLabelY, kNameLabelWidth, kNameLabelHeight)];
        _nameLabel.font = [NSFont fontWithName:@"Maple Mono" size:kNameFontSize] ?: [NSFont systemFontOfSize:kNameFontSize weight:NSFontWeightSemibold];
        _nameLabel.textColor = [NSColor whiteColor];
        _nameLabel.backgroundColor = [NSColor clearColor];
        _nameLabel.bordered = NO;
        _nameLabel.editable = NO;
        [self addSubview:_nameLabel];

        _descLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(kDescLabelX, kDescLabelY, kDescLabelWidth, kDescLabelHeight)];
        _descLabel.font = [NSFont fontWithName:@"Maple Mono" size:kDescFontSize] ?: [NSFont systemFontOfSize:kDescFontSize];
        _descLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
        _descLabel.backgroundColor = [NSColor clearColor];
        _descLabel.bordered = NO;
        _descLabel.editable = NO;
        _descLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        [self addSubview:_descLabel];

        // Toggle on the right side
        float toggleX = self.bounds.size.width - kToggleWidth - kToggleRightPad;
        _toggle = [[NSSwitch alloc] initWithFrame:NSMakeRect(toggleX, kToggleY, kToggleWidth, kToggleHeight)];
        _toggle.target = self;
        _toggle.action = @selector(toggled:);
        [self addSubview:_toggle];

        NSView* separator = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(kSeparatorX, 0, self.bounds.size.width - kSeparatorInset, kSeparatorWidth)];
        separator.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
        [self addSubview:separator];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    if (_selected) {
        NSRect highlightRect = NSInsetRect(self.bounds, kHighlightInset, 0);
        NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:highlightRect xRadius:kHighlightRadius yRadius:kHighlightRadius];
        [[NSColor colorWithWhite:1.0 alpha:kHighlightAlpha] setFill];
        [path fill];
    }
    [super drawRect:dirtyRect];
}

- (void)mouseUp:(NSEvent*)event {
    NSPoint pt = [self convertPoint:event.locationInWindow fromView:nil];
    // Only open detail panel if click is outside the toggle switch
    // The toggle handles its own clicks natively — don't double-fire
    if (!NSPointInRect(pt, _toggle.frame)) {
        [self openDetail];
    }
}

- (void)setSelected:(BOOL)selected {
    _selected = selected;
    [self setNeedsDisplay:YES];
}

- (void)setEnabled:(BOOL)enabled {
    _toggle.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)toggled:(id)sender {
    if (_configKey) {
        std::string key = std::string([_configKey UTF8String]);
        bool val = ((NSSwitch*)sender).state == NSControlStateValueOn;
        s_setBoolValue(key, val);
    }
}

- (void)openDetail {
    if (_detailView && _configKey) {
        [_detailView configureForBehavior:_configKey];
    }
}

@end

@implementation PreviewGooseView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.wantsLayer = YES;
    }
    return self;
}

- (void)updatePreview {
    if (!self.layer) return;
    NSSize size = self.bounds.size;
    if (size.width <= 0 || size.height <= 0) return;

    NSImage* img = [[NSImage alloc] initWithSize:size];
    [img lockFocus];
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    if (ctx) [self renderGooseInContext:ctx size:size];
    [img unlockFocus];
    self.layer.contents = img;
}

- (void)viewDidMoveToWindow {
    [self updatePreview];
}

- (void)renderGooseInContext:(CGContextRef)ctx size:(NSSize)size {
    CGFloat cx = size.width / 2;
    CGFloat cy = size.height / 2 + 10;

    ColorRGB& body = g_config.color.currentBody;
    ColorRGB& neck = g_config.color.currentNeck;
    ColorRGB& head = g_config.color.currentHead;
    ColorRGB& beak = g_config.color.currentBeak;
    ColorRGB& eye = g_config.color.currentEye;
    ColorRGB& outline = g_config.color.currentOutline;

    CGContextSetRGBFillColor(ctx, 0, 0, 0, 0.15);
    CGContextFillEllipseInRect(ctx, CGRectMake(cx - 28, cy - 4, 56, 18));

    CGContextSetRGBFillColor(ctx, beak.r, beak.g, beak.b, 1);
    CGContextFillEllipseInRect(ctx, CGRectMake(cx - 15, cy - 14, 10, 6));
    CGContextFillEllipseInRect(ctx, CGRectMake(cx + 5, cy - 14, 10, 6));

    CGContextSetRGBStrokeColor(ctx, outline.r, outline.g, outline.b, 1);
    CGContextSetLineWidth(ctx, 3);
    CGContextStrokeEllipseInRect(ctx, CGRectMake(cx - 28, cy - 22, 56, 28));

    CGContextSetRGBFillColor(ctx, body.r, body.g, body.b, 1);
    CGContextFillEllipseInRect(ctx, CGRectMake(cx - 28, cy - 22, 56, 28));

    CGContextSetRGBStrokeColor(ctx, outline.r, outline.g, outline.b, 1);
    CGContextSetLineWidth(ctx, 3);
    CGContextMoveToPoint(ctx, cx + 22, cy + 2);
    CGContextAddLineToPoint(ctx, cx + 28, cy + 20);
    CGContextStrokePath(ctx);

    CGContextSetRGBStrokeColor(ctx, neck.r, neck.g, neck.b, 1);
    CGContextSetLineWidth(ctx, 8);
    CGContextSetLineCap(ctx, kCGLineCapRound);
    CGContextMoveToPoint(ctx, cx + 22, cy + 2);
    CGContextAddLineToPoint(ctx, cx + 28, cy + 20);
    CGContextStrokePath(ctx);

    CGContextSetRGBStrokeColor(ctx, outline.r, outline.g, outline.b, 1);
    CGContextSetLineWidth(ctx, 3);
    CGContextStrokeEllipseInRect(ctx, CGRectMake(cx + 20, cy + 16, 18, 16));

    CGContextSetRGBFillColor(ctx, head.r, head.g, head.b, 1);
    CGContextFillEllipseInRect(ctx, CGRectMake(cx + 20, cy + 16, 18, 16));

    CGContextSetRGBFillColor(ctx, beak.r, beak.g, beak.b, 1);
    CGContextBeginPath(ctx);
    CGContextMoveToPoint(ctx, cx + 38, cy + 26);
    CGContextAddLineToPoint(ctx, cx + 52, cy + 24);
    CGContextAddLineToPoint(ctx, cx + 38, cy + 22);
    CGContextClosePath(ctx);
    CGContextFillPath(ctx);

    CGContextSetRGBFillColor(ctx, eye.r, eye.g, eye.b, 1);
    CGContextFillEllipseInRect(ctx, CGRectMake(cx + 24, cy + 24, 4, 4));
}

@end
