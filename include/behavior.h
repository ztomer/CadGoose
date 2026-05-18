// ===========================
// behavior.h
// Multi-Entity Behavior System with per-instance state management
// ===========================
#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <shared_mutex>

#include "goose_math.h"
#include "event_bus.h"

struct Goose;
class IRenderer;

#include "behavior_state.h"
#include "behavior_manager.h"
#include "behavior_registry.h"
#include "behavior_api.h"

#endif // BEHAVIOR_H
