#ifndef AUDIO_H
#define AUDIO_H

void Audio_Init();
void Audio_PlayHonk();
void Audio_PlayGulag();
void Audio_PlayBite();
void Audio_PlayMudSquish();
void Audio_PlayPat();
void Audio_SetMuted(bool muted);
void Audio_SetEnabled(bool enabled);
void Audio_Cleanup();

#endif