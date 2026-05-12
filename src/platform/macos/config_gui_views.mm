// config_gui_views.mm
// BehaviorRowView and PreviewGooseView implementations
#import "config_gui_helpers.h"
#include "config.h"

@implementation BehaviorRowView

+ (NSString*)iconForConfigKey:(NSString*)key {
    if ([key hasSuffix:@"ball"]) return @"⚽";
    if ([key hasSuffix:@"breadCrumbs"]) return @"🍞";
    if ([key hasSuffix:@"hats"]) return @"🎩";
    if ([key hasSuffix:@"rainbow"]) return @"🌈";
    if ([key hasSuffix:@"acid"]) return @"🧪";
    if ([key hasSuffix:@"anger"]) return @"😠";
    if ([key hasSuffix:@"autumnLeaves"]) return @"🍂";
    if ([key hasSuffix:@"honcker"]) return @"📯";
    if ([key hasSuffix:@"jail"]) return @"🔒";
    if ([key hasSuffix:@"portals"]) return @"🌀";
    if ([key hasSuffix:@"drag"]) return @"🖱️";
    if ([key hasSuffix:@"banish"]) return @"👻";
    if ([key hasSuffix:@"nametag"]) return @"🏷️";
    if ([key hasSuffix:@"health"]) return @"❤️";
    if ([key hasSuffix:@"ai"]) return @"🤖";
    if ([key hasSuffix:@"pomodoro"]) return @"⏰";
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
        _nameLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:14] ?: [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold];
        _nameLabel.textColor = [NSColor labelColor];
        _nameLabel.backgroundColor = [NSColor clearColor];
        _nameLabel.bordered = NO;
        _nameLabel.editable = NO;
        [self addSubview:_nameLabel];

        _descLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(230, 8, 200, 14)];
        _descLabel.font = [NSFont fontWithName:@"Comic Sans MS" size:11] ?: [NSFont systemFontOfSize:11];
        _descLabel.textColor = [NSColor secondaryLabelColor];
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
    if (NSPointInRect(pt, _toggle.frame)) {
        [_toggle performClick:nil];
        return;
    }
    [self openDetail];
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

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    if (!ctx) return;

    CGFloat cx = self.bounds.size.width / 2;
    CGFloat cy = self.bounds.size.height / 2 + 10;

    ColorRGB& body = g_config.color.customBody;
    ColorRGB& neck = g_config.color.customNeck;
    ColorRGB& head = g_config.color.customHead;
    ColorRGB& beak = g_config.color.customBeak;
    ColorRGB& eye = g_config.color.customEye;
    ColorRGB& outline = g_config.color.customOutline;

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
