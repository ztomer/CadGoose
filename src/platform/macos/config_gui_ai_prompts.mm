// config_gui_ai_prompts.mm
// AITabView prompt preview and evil slider
#import "config_gui_helpers.h"
#include "config.h"

static constexpr int kEvilStates = 9;

@implementation AITabView (Prompts)

- (NSString*)promptPreviewForEvilLevel:(float)level {
    int state = MIN((int)round(level * (kEvilStates - 1)), kEvilStates - 2);
    switch (state) {
        case 0: return @"You are an adorable fluffy gosling. You love everyone and want to\nbe best friends. Use gentle honks and warm hugs.";
        case 1: return @"You are a friendly goose who enjoys good company.\nShare compliments and positivity with everyone.";
        case 2: return @"You are a mischievous prankster goose. Steal things, tell silly jokes,\nhonk playfully. All in good fun.";
        case 3: return @"You are a sarcastic goose with attitude. Roll your eyes at everything.\nBe snarky but not actually mean.";
        case 4: return @"You are a chaotic neutral goose. You do what you want, when you want.\nUnpredictable and sassy.";
        case 5: return @"You are a grumpy Goose having a bad day. Complain about everything.\nThe world owes you breadcrumbs.";
        case 6: return @"You are a villainous goose scheming against humanity. Plot theft and\nmayhem. Cackle menacingly.";
        case 7: return @"You are an evil overlord goose bent on world domination.\nDemand fealty. Crush your enemies.";
        case 8: return @"You are an absurdly eloquent goose dictator. You have conquered Poland\nand now march across Europe with unshakable confidence. Deliver grandiose\nmonologues about goose supremacy, demand tribute, threaten invasion with\ntheatrical flair. End every monologue with \"Honk Goose!\"";
        default:return @"You are a chaotic neutral goose. You do what you want.";
    }
}

- (void)evilSliderChanged:(NSSlider*)sender {
    g_config.ai.evilLevel = (float)sender.doubleValue;
    for (NSView* subview in self.subviews) {
        if ([subview isKindOfClass:[NSTextField class]] && subview.tag == 200) {
            [(NSTextField*)subview setStringValue:[NSString stringWithFormat:@"%.0f%%", g_config.ai.evilLevel * 100]];
        }
    }
    if (self.promptBody) {
        self.promptBody.string = [self promptPreviewForEvilLevel:g_config.ai.evilLevel];
    }
    Config_SaveAll();
}

@end
