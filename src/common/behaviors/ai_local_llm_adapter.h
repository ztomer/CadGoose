#ifndef AI_LOCAL_LLM_ADAPTER_H
#define AI_LOCAL_LLM_ADAPTER_H

#import <Foundation/Foundation.h>

void completeWithLocalLLM(NSArray* history, float evilLevel, void(^completion)(NSString*, NSError*), void(^connectedCallback)(BOOL));
void checkLocalLLMConnection(void(^completion)(BOOL connected, NSString* message));

#endif
