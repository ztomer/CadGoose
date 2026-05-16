// config_gui_views.mm
// BehaviorRowView and PreviewGooseView implementations
#import "config_gui_helpers.h"
#include "config.h"

@implementation BehaviorRowView

+ (NSString*)iconForConfigKey:(NSString*)key {
    if ([key isEqualToString:@"ball_enabled"]) return @"⚽";
    if ([key isEqualToString:@"breadcrumbs_enabled"]) return @"🍞";
    if ([key isEqualToString:@"hats_enabled"]) return @"🎩";
    if ([key isEqualToString:@"rainbow_enabled"]) return @"🌈";
    if ([key isEqualToString:@"acid_enabled"]) return @"🧪";
    if ([key isEqualToString:@"anger_enabled"]) return @"😠";
    if ([key isEqualToString:@"autumn_leaves_enabled"]) return @"🍂";
    if ([key isEqualToString:@"honcker_enabled"]) return @"📯";
    if ([key isEqualToString:@"jail_enabled"]) return @"🔒";
    if ([key isEqualToString:@"portals_enabled"]) return @"🌀";
    if ([key isEqualToString:@"drag_enabled"]) return @"🖱️";
    if ([key isEqualToString:@"nametag_enabled"]) return @"🏷️";
    if ([key isEqualToString:@"health_enabled"]) return @"❤️";
    if ([key isEqualToString:@"ai_enabled"]) return @"🤖";
    if ([key isEqualToString:@"pomodoro_enabled"]) return @"⏰";
    if ([key isEqualToString:@"toys_enabled"]) return @"🧸";
    if ([key isEqualToString:@"avoidance_enabled"]) return @"🏃";
    if ([key isEqualToString:@"boredom_enabled"]) return @"😮‍💨";
    if ([key isEqualToString:@"peeking_enabled"]) return @"👀";
    if ([key isEqualToString:@"affirmations_enabled"]) return @"💬";
    if ([key isEqualToString:@"interactive_drops_enabled"]) return @"💧";
    if ([key isEqualToString:@"appearance.colors"]) return @"🎨";
    return @"🦆";
}

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _toggle = [[NSButton alloc] initWithFrame:NSMakeRect(10, 6, 20, 20)];
        _toggle.buttonType = NSButtonTypeSwitch;
        _toggle.title = @"";
        _toggle.target = self;
        _toggle.action = @selector(toggled:);
        [self addSubview:_toggle];

        _iconLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(34, 4, 28, 22)];
        _iconLabel.font = [NSFont systemFontOfSize:16];
        _iconLabel.backgroundColor = [NSColor clearColor];
        _iconLabel.bordered = NO;
        _iconLabel.editable = NO;
        _iconLabel.alignment = NSTextAlignmentCenter;
        [self addSubview:_iconLabel];

        _nameLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(64, 7, 140, 18)];
        _nameLabel.font = [NSFont fontWithName:@"Maple Mono" size:14] ?: [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold];
        _nameLabel.textColor = [NSColor whiteColor];
        _nameLabel.backgroundColor = [NSColor clearColor];
        _nameLabel.bordered = NO;
        _nameLabel.editable = NO;
        [self addSubview:_nameLabel];

        _descLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(230, 8, 200, 14)];
        _descLabel.font = [NSFont fontWithName:@"Maple Mono" size:11] ?: [NSFont systemFontOfSize:11];
        _descLabel.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
        _descLabel.backgroundColor = [NSColor clearColor];
        _descLabel.bordered = NO;
        _descLabel.editable = NO;
        _descLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        [self addSubview:_descLabel];

        NSView* separator = [[AppBarBorderView alloc] initWithFrame:NSMakeRect(8, 0, self.bounds.size.width - 16, 1)];
        separator.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
        [self addSubview:separator];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    if (_selected) {
        [[NSColor colorWithWhite:1.0 alpha:0.08] setFill];
        NSRectFill(self.bounds);
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
        bool val = ((NSButton*)sender).state == NSControlStateValueOn;
        s_setBoolValue(key, val);
    }
}

- (void)openDetail {
    if (_target && _detailAction) {
        [_target performSelector:_detailAction withObject:self.configKey];
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
