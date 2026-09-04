#import "ai_think_block_stripper.h"

NSString* stripThinkBlocks(NSString* content) {
    if (!content || content.length == 0) return content;
    NSError* regexErr;
    NSRegularExpression* regex = [NSRegularExpression regularExpressionWithPattern:@"<think>.*?</think>"
                                                                           options:NSRegularExpressionDotMatchesLineSeparators
                                                                             error:&regexErr];
    if (regexErr) return content;
    NSString* stripped = [regex stringByReplacingMatchesInString:content options:0 range:NSMakeRange(0, content.length) withTemplate:@""];
    stripped = [stripped stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (stripped.length > 0) return stripped;
    return content;
}
