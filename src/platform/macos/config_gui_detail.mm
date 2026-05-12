// config_gui_detail.mm
// BehaviorDetailView — main implementation (init, behavior config UI, AI provider, geese list)
#import "config_gui_helpers.h"
#include "config.h"
#include "world.h"

@implementation BehaviorDetailView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _titleLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(14, frame.size.height - 28, frame.size.width - 20, 20)];
        _titleLabel.font = [NSFont boldSystemFontOfSize:15];
        _titleLabel.textColor = [NSColor whiteColor];
        _titleLabel.backgroundColor = [NSColor clearColor];
        _titleLabel.bordered = NO;
        _titleLabel.editable = NO;
        [self addSubview:_titleLabel];

        _contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height - 36)];
        [self addSubview:_contentView];
    }
    return self;
}

- (void)configureForBehavior:(NSString*)key {
    _configKey = key;

    for (NSView* subview in _contentView.subviews) {
        [subview removeFromSuperview];
    }

    _titleLabel.stringValue = [NSString stringWithFormat:@"Settings for %@", [key lastPathComponent]];

    float y = _contentView.bounds.size.height - 40;

    if ([key isEqualToString:@"behaviors.fun.ball"]) {
        _titleLabel.stringValue = @"Ball Behavior";
        [self addSliderWithLabel:@"Ball Size" min:5.0f max:50.0f value:g_config.behaviors.ball.size atY:y key:@"behaviors.fun.ball.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.breadCrumbs"]) {
        _titleLabel.stringValue = @"Breadcrumbs Behavior";
        [self addSliderWithLabel:@"Max Crumbs" min:10.0f max:200.0f value:g_config.behaviors.breadCrumbs.maxCrumbs atY:y key:@"behaviors.fun.breadCrumbs.max"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.hats"]) {
        _titleLabel.stringValue = @"Hats Behavior";
        [self addSliderWithLabel:@"Hat Size" min:4.0f max:128.0f value:g_config.behaviors.hats.sizeX atY:y key:@"behaviors.fun.hats.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.rainbow"]) {
        _titleLabel.stringValue = @"Rainbow Behavior";
        [self addSliderWithLabel:@"Hue Speed" min:0.1f max:5.0f value:g_config.behaviors.rainbow.hueSpeed atY:y key:@"behaviors.fun.rainbow.speed"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.acid"]) {
        _titleLabel.stringValue = @"Acid Behavior";
        [self addSliderWithLabel:@"Spin Speed" min:0.1f max:5.0f value:g_config.behaviors.acid.spinSpeed atY:y key:@"behaviors.fun.acid.speed"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.fun.anger"]) {
        _titleLabel.stringValue = @"Anger Behavior";
        [self addSliderWithLabel:@"Max Anger" min:0.0f max:200.0f value:g_config.behaviors.anger.maxAnger atY:y key:@"behaviors.fun.anger.max"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.honcker"]) {
        _titleLabel.stringValue = @"Honcker Behavior";
        [self addInstructionLabel:@"🦆 Press F to honk at cursor location" atY:y];
        y -= 25;
        [self addSliderWithLabel:@"Honk Cooldown" min:0.1f max:10.0f value:g_config.behaviors.honcker.cooldown atY:y key:@"behaviors.control.honcker.cooldown"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.jail"]) {
        _titleLabel.stringValue = @"Jail Behavior";
        [self addInstructionLabel:@"🔒 O = set cursor as jail position\n   P = toggle jail on/off" atY:y];
        y -= 42;
        [self addSliderWithLabel:@"Jail Size" min:50.0f max:300.0f value:g_config.behaviors.jail.size atY:y key:@"behaviors.control.jail.size"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.portals"]) {
        _titleLabel.stringValue = @"Portal Behavior";
        [self addInstructionLabel:@"🌀 P+1 = place portal A\n   P+2 = place portal B\n   P+0 = toggle portals" atY:y];
        y -= 60;
        [self addSliderWithLabel:@"Portal Width" min:30.0f max:200.0f value:g_config.portal.width atY:y key:@"behaviors.control.portals.width"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.drag"]) {
        _titleLabel.stringValue = @"Drag Behavior";
        [self addInstructionLabel:@"🖱️ Click and drag the goose" atY:y];
        y -= 25;
        [self addSliderWithLabel:@"Drag Radius" min:50.0f max:300.0f value:g_config.behaviors.drag.radius atY:y key:@"behaviors.control.drag.radius"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.control.banish"]) {
        _titleLabel.stringValue = @"Banish Behavior";
        [self addInstructionLabel:@"👻 Ctrl+Alt+Middle Click to banish\n   Respawns after the duration" atY:y];
        y -= 42;
        [self addSliderWithLabel:@"Duration (s)" min:1.0f max:60.0f value:g_config.behaviors.banish.duration atY:y key:@"behaviors.control.banish.duration"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.info.nametag"]) {
        _titleLabel.stringValue = @"Nametag & Geese";
        [self addSliderWithLabel:@"Font Size" min:8.0f max:40.0f value:g_config.behaviors.nametag.size atY:y key:@"behaviors.info.nametag.size"];
        y -= 35;
        y = [self addGeeseListAtY:y];
    } else if ([key isEqualToString:@"behaviors.systems.health"]) {
        _titleLabel.stringValue = @"Health System";
        [self addSliderWithLabel:@"Opacity" min:0.2f max:1.0f value:g_config.behaviors.health.opacity atY:y key:@"behaviors.systems.health.opacity"];
        y -= 35;
    } else if ([key isEqualToString:@"behaviors.systems.pomodoro"]) {
        _titleLabel.stringValue = @"Pomodoro Timer";
        [self addSliderWithLabel:@"Work (min)" min:1.0f max:60.0f value:g_config.behaviors.pomodoro.workMinutes atY:y key:@"behaviors.systems.pomodoro.workDuration"];
        y -= 35;
        [self addSliderWithLabel:@"Break (min)" min:1.0f max:30.0f value:g_config.behaviors.pomodoro.breakMinutes atY:y key:@"behaviors.systems.pomodoro.breakDuration"];
        y -= 35;
    } else {
        NSTextField* desc = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, self.bounds.size.width - 24, 30)];
        desc.font = [NSFont systemFontOfSize:12];
        desc.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
        desc.backgroundColor = [NSColor clearColor];
        desc.bordered = NO;
        desc.editable = NO;
        desc.stringValue = [NSString stringWithFormat:@"No settings for %@", [key lastPathComponent]];
        [_contentView addSubview:desc];
    }
}

- (void)addInstructionLabel:(NSString*)text atY:(float)y {
    NSTextField* label = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, self.bounds.size.width - 24, 40)];
    label.font = [NSFont fontWithName:@"Comic Sans MS" size:12] ?: [NSFont systemFontOfSize:12];
    label.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    label.backgroundColor = [NSColor clearColor];
    label.bordered = NO;
    label.editable = NO;
    [_contentView addSubview:label];

    NSMutableParagraphStyle* para = [[NSMutableParagraphStyle alloc] init];
    para.lineSpacing = 2;
    NSDictionary* attrs = @{NSFontAttributeName: label.font ?: [NSFont systemFontOfSize:12],
                            NSParagraphStyleAttributeName: para};
    label.attributedStringValue = [[NSAttributedString alloc] initWithString:text attributes:attrs];
}

- (void)addSliderWithLabel:(NSString*)label min:(float)min max:(float)max value:(float)value atY:(float)y key:(NSString*)key {
    CGFloat pw = _contentView.bounds.size.width;
    CGFloat leftPad = 12;
    CGFloat gap = 6;
    NSDictionary* font12 = @{NSFontAttributeName: [NSFont systemFontOfSize:12]};
    CGFloat labelW = [label sizeWithAttributes:font12].width + 10;
    CGFloat valW = [@"100.00" sizeWithAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:11]}].width + 14;
    CGFloat sliderW = pw - leftPad - labelW - gap - gap - valW - 12;
    CGFloat sliderX = leftPad + labelW + gap;
    CGFloat valX = sliderX + sliderW + gap;

    NSTextField* labelField = [[NSTextField alloc] initWithFrame:NSMakeRect(leftPad, y, labelW, 16)];
    labelField.font = [NSFont systemFontOfSize:12];
    labelField.textColor = [NSColor whiteColor];
    labelField.backgroundColor = [NSColor clearColor];
    labelField.bordered = NO;
    labelField.editable = NO;
    labelField.stringValue = label;
    [_contentView addSubview:labelField];

    NSSlider* slider = [[NSSlider alloc] initWithFrame:NSMakeRect(sliderX, y, sliderW, 20)];
    slider.minValue = min;
    slider.maxValue = max;
    slider.doubleValue = value;
    slider.identifier = key;
    slider.target = self;
    slider.action = @selector(sliderChanged:);
    [_contentView addSubview:slider];

    NSTextField* valueField = [[NSTextField alloc] initWithFrame:NSMakeRect(valX, y, valW, 16)];
    valueField.font = [NSFont systemFontOfSize:11];
    valueField.textColor = [NSColor colorWithWhite:0.85 alpha:1.0];
    valueField.backgroundColor = [NSColor clearColor];
    valueField.bordered = NO;
    valueField.editable = YES;
    valueField.stringValue = [NSString stringWithFormat:@"%.2f", value];
    valueField.identifier = key;
    valueField.target = self;
    valueField.action = @selector(valueFieldChanged:);
    [_contentView addSubview:valueField];
}

- (float)addGeeseListAtY:(float)y {
    NSTextField* sectionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, y, 200, 16)];
    sectionLabel.stringValue = @"Geese";
    sectionLabel.font = [NSFont systemFontOfSize:12 weight:NSFontWeightSemibold];
    sectionLabel.textColor = [NSColor labelColor];
    sectionLabel.backgroundColor = [NSColor clearColor];
    sectionLabel.bordered = NO;
    sectionLabel.editable = NO;
    [_contentView addSubview:sectionLabel];
    y -= 24;

    for (auto it = g_geese.begin(); it != g_geese.end(); ++it) {
        NSTextField* idLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16, y, 30, 20)];
        idLabel.stringValue = [NSString stringWithFormat:@"#%d", it->id];
        idLabel.font = [NSFont systemFontOfSize:11];
        idLabel.textColor = [NSColor secondaryLabelColor];
        idLabel.backgroundColor = [NSColor clearColor];
        idLabel.bordered = NO;
        idLabel.editable = NO;
        idLabel.alignment = NSTextAlignmentRight;
        [_contentView addSubview:idLabel];

        NSTextField* nameField = [[NSTextField alloc] initWithFrame:NSMakeRect(52, y, self.bounds.size.width - 64, 22)];
        nameField.stringValue = [NSString stringWithUTF8String:it->name.c_str()];
        nameField.font = [NSFont systemFontOfSize:12];
        nameField.bezelStyle = NSTextFieldRoundedBezel;
        nameField.tag = it->id;
        nameField.target = self;
        nameField.action = @selector(gooseNameChanged:);
        [_contentView addSubview:nameField];

        y -= 28;
    }

    return y;
}

- (void)gooseNameChanged:(NSTextField*)sender {
    int gooseId = (int)sender.tag;
    std::string newName = std::string([sender.stringValue UTF8String]);
    for (auto it = g_geese.begin(); it != g_geese.end(); ++it) {
        if (it->id == gooseId) {
            it->name = newName;
            break;
        }
    }
}

- (void)sliderChanged:(NSSlider*)sender {
    float value = (float)sender.doubleValue;
    NSString* keyStr = sender.identifier;
    if (keyStr) {
        s_setFloatValue(std::string([keyStr UTF8String]), value);
        for (NSView* subview in _contentView.subviews) {
            if ([subview isKindOfClass:[NSTextField class]] && ![subview isEqualTo:sender] && [((NSTextField*)subview).identifier isEqualToString:keyStr]) {
                ((NSTextField*)subview).stringValue = [NSString stringWithFormat:@"%.2f", value];
                break;
            }
        }
    }
}

- (void)valueFieldChanged:(NSTextField*)sender {
    float value = (float)sender.doubleValue;
    NSString* keyStr = sender.identifier;
    if (keyStr) {
        s_setFloatValue(std::string([keyStr UTF8String]), value);
        for (NSView* subview in _contentView.subviews) {
            if ([subview isKindOfClass:[NSSlider class]] && [((NSSlider*)subview).identifier isEqualToString:keyStr]) {
                ((NSSlider*)subview).doubleValue = value;
                break;
            }
        }
    }
}

@end
