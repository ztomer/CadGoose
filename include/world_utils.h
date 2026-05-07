// ===========================
// world_utils.h
// World maintenance utilities
// ===========================
#ifndef WORLD_UTILS_H
#define WORLD_UTILS_H

#include "world.h"
#include "config.h"

#ifdef __APPLE__
#import <AppKit/AppKit.h>
#endif

void World_CleanupExpired(double currentTime);
void World_TickLeafPiles(double currentTime, float dt, Goose* nearestGoose);
void World_SpawnRandomLeafPile(float screenWidth, float screenHeight, double currentTime);
bool ItemHitTest(NSPoint p, float viewHeight, DroppedItem** hitItem, float closeButtonSize);
bool CheckCloseButton(float lx, float ly, float w, float h, float closeButtonSize);
void DeleteDroppedItem(DroppedItem* item);
void MoveItemToFront(DroppedItem* item);
bool ShouldAcceptMouseEvents();

#endif