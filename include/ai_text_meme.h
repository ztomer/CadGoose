#pragma once
#include <string>
#include <vector>

void AI_TextMeme_Tick(double time);
bool AI_TextMeme_HasAvailable();
std::string AI_TextMeme_Dequeue();
int AI_TextMeme_QueueSize();
void AI_TextMeme_Reset();
void AI_TextMeme_Inject(const std::string& text);
void AI_TextMeme_LoadFileTexts();
