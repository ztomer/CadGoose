#ifndef BEHAVIOR_API_H
#define BEHAVIOR_API_H

// API functions for behaviors
struct Goose;
float Rainbow_GetHue(int gooseId);
void Rainbow_SetHue(int gooseId, float hue);
void Health_Damage(Goose* goose, float amount, double time);
void Health_Heal(Goose* goose, float amount);
void Honcker_Honk(Goose* goose, double time);


#endif
