import sys

with open('include/behavior.h', 'r') as f:
    lines = f.readlines()

# Find sections
state_start = -1
manager_start = -1
behavior_start = -1
registry_start = -1
api_start = -1

for i, line in enumerate(lines):
    if line.startswith('struct BehaviorContext {'):
        state_start = i
    elif line.startswith('class BehaviorStateManager {'):
        manager_start = i
    elif line.startswith('struct Behavior {'):
        behavior_start = i
    elif line.startswith('class BehaviorRegistry {'):
        registry_start = i
    elif line.startswith('// API functions for behaviors'):
        api_start = i

state_lines = lines[state_start:manager_start]
manager_lines = lines[manager_start:behavior_start]
registry_lines = lines[behavior_start:api_start]
api_lines = lines[api_start:-1]

# Extract misplaced types
puddle_start = -1
flower_start = -1
toy_start = -1

for i, line in enumerate(state_lines):
    if line.startswith('struct InteractivePuddle {'):
        puddle_start = i
    elif line.startswith('struct InteractiveFlower {'):
        flower_start = i
    elif line.startswith('struct Toy {'):
        toy_start = i

def get_struct_end(start_idx, lines_list):
    for i in range(start_idx, len(lines_list)):
        if lines_list[i].startswith('};'):
            return i + 1
    return -1

puddle_end = get_struct_end(puddle_start, state_lines)
flower_end = get_struct_end(flower_start, state_lines)
toy_end = get_struct_end(toy_start, state_lines)

puddle_code = "".join(state_lines[puddle_start:puddle_end])
flower_code = "".join(state_lines[flower_start:flower_end])
toy_code = "".join(state_lines[toy_start:toy_end])

# Remove from state_lines
# sort descending to remove
ranges = sorted([(puddle_start, puddle_end), (flower_start, flower_end), (toy_start, toy_end)], reverse=True)
for start, end in ranges:
    del state_lines[start:end]

# Append to world.h
with open('include/world.h', 'a') as f:
    f.write("\n" + puddle_code + "\n" + flower_code + "\n" + toy_code)

# Fix MakeKey in manager_lines
for i in range(len(manager_lines)):
    if manager_lines[i].find('std::unordered_map<int, std::unique_ptr<BehaviorState>> states;') != -1:
        manager_lines[i] = manager_lines[i].replace('int', 'uint64_t', 1)
    elif manager_lines[i].find('int MakeKey(int gooseId, const char* behaviorId)') != -1:
        manager_lines[i] = manager_lines[i].replace('int MakeKey', 'uint64_t MakeKey')
    elif manager_lines[i].find('return (gooseId << 16) | static_cast<int>(hash & 0xFFFF);') != -1:
        manager_lines[i] = '        return (static_cast<uint64_t>(gooseId) << 32) | static_cast<uint32_t>(hash);\n'
    elif manager_lines[i].find('int key = MakeKey(') != -1:
        manager_lines[i] = manager_lines[i].replace('int key', 'uint64_t key')
    elif manager_lines[i].find('int storedId = (it->first >> 16);') != -1:
        manager_lines[i] = manager_lines[i].replace('(it->first >> 16)', 'static_cast<int>(it->first >> 32)')


# Consolidate macros in registry_lines
new_macros = """// Centralized behavior constructor — enabledPtr and configPtr always
// point to the same config bool, preventing the toggle-desync bug.
#define BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, cleanupFn, starterFlag, groundFlag) \\
    Behavior { \\
        .id = bid, \\
        .name = bname, \\
        .description = bdesc, \\
        .enabledPtr = &configBool, \\
        .configPtr = &configBool, \\
        .init = initFn, \\
        .tick = tickFn, \\
        .render = renderFn, \\
        .cleanup = cleanupFn, \\
        .renderOnGround = groundFlag, \\
        .conflicts = nullptr, \\
        .priority = 0, \\
        .config = { .requiresAccessibility = false, .isStarter = starterFlag } \\
    }

#define BEHAVIOR_DEF(bid, bname, bdesc, configBool, initFn, tickFn, renderFn) \\
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, nullptr, false, false)

#define BEHAVIOR_DEF_STARTER(bid, bname, bdesc, configBool, initFn, tickFn, renderFn) \\
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, nullptr, true, false)

#define BEHAVIOR_DEF_GROUND(bid, bname, bdesc, configBool, initFn, tickFn, renderFn) \\
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, nullptr, false, true)

#define BEHAVIOR_DEF_CUSTOM(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, cleanupFn, starterFlag, groundFlag) \\
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, cleanupFn, starterFlag, groundFlag)

#define BEHAVIOR_ENABLED(goose, name) \\
    ([]() { \\
        auto* _b = BehaviorRegistry::Instance().Get(name); \\
        return _b && *_b->enabledPtr && (goose) && (goose)->behaviorsEnabled; \\
    }())
"""

# Find where macros start
macro_start = -1
for i, line in enumerate(registry_lines):
    if line.startswith('// Centralized behavior constructor'):
        macro_start = i
        break

# Only replace if we found the macros
if macro_start != -1:
    macro_end = -1
    for i in range(macro_start, len(registry_lines)):
        if registry_lines[i].startswith('// API functions'):
            macro_end = i
            break
    if macro_end == -1:
        macro_end = len(registry_lines)
    
    del registry_lines[macro_start:macro_end]
    registry_lines.append(new_macros)


header = lines[0:state_start]

# Write files
with open('include/behavior_state.h', 'w') as f:
    f.write("#ifndef BEHAVIOR_STATE_H\n#define BEHAVIOR_STATE_H\n\n#include <vector>\n#include \"goose_math.h\"\n#include \"event_bus.h\"\n#include \"world.h\"\n\n")
    f.write("".join(state_lines))
    f.write("\n#endif\n")

with open('include/behavior_manager.h', 'w') as f:
    f.write("#ifndef BEHAVIOR_MANAGER_H\n#define BEHAVIOR_MANAGER_H\n\n#include <unordered_map>\n#include <memory>\n#include <shared_mutex>\n#include <string>\n#include \"behavior_state.h\"\n\n")
    f.write("".join(manager_lines))
    f.write("\n#endif\n")

with open('include/behavior_registry.h', 'w') as f:
    f.write("#ifndef BEHAVIOR_REGISTRY_H\n#define BEHAVIOR_REGISTRY_H\n\n#include <vector>\n#include <functional>\n#include \"behavior_state.h\"\n\n")
    f.write("".join(registry_lines))
    f.write("\n#endif\n")

with open('include/behavior_api.h', 'w') as f:
    f.write("#ifndef BEHAVIOR_API_H\n#define BEHAVIOR_API_H\n\n")
    f.write("".join(api_lines))
    f.write("\n#endif\n")

with open('include/behavior.h', 'w') as f:
    f.write("".join(header))
    f.write("#include \"behavior_state.h\"\n")
    f.write("#include \"behavior_manager.h\"\n")
    f.write("#include \"behavior_registry.h\"\n")
    f.write("#include \"behavior_api.h\"\n")
    f.write("\n#endif // BEHAVIOR_H\n")
