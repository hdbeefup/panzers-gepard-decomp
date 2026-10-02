// common/keybinds.h
// Rebindable keyboard controls — action enum, binding table, presets.
// Non-original: gated behind HD_KEYBINDS.

#ifndef COMMON_KEYBINDS_H
#define COMMON_KEYBINDS_H

#include "hdbeefup.h"

#ifdef HD_KEYBINDS

struct SProperties;
struct SStream;

enum EKeyAction {
    KA_NONE = 0,

    KA_PAUSE,
    KA_STEP_FRAME,
    KA_FAST_FORWARD,

    KA_CMD_ATTACK,
    KA_CMD_MOVE,
    KA_CMD_SPECACTION,
    KA_CMD_STOP,
    KA_CMD_CHASE,
    KA_CMD_FIRE_AT_WILL,
    KA_CMD_HOLD_FIRE,
    KA_CMD_RETREAT,
    KA_CMD_RETURN_FIRE,

    KA_QUICKBUY,

    KA_CAM_UP,
    KA_CAM_DOWN,
    KA_CAM_LEFT,
    KA_CAM_RIGHT,
    KA_CAM_ROTATE_L,
    KA_CAM_ROTATE_R,
    KA_CAM_ELEVATION_UP,
    KA_CAM_ELEVATION_DOWN,
    KA_CAM_ZOOM_IN,
    KA_CAM_ZOOM_OUT,

    KA_COUNT
};

enum EControlScheme {
    CS_CLASSIC = 0,
    CS_WASD    = 1,
    CS_CUSTOM  = 2,
    CS_COMFY   = 3
};

enum EAssignResult {
    ASSIGN_OK,
    ASSIGN_SYSTEM_KEY,      // reserved (1-9 groups, F-keys, Tab/Enter/Esc, modifiers)
    ASSIGN_DUPLICATE        // already bound to another action (conflict reported via out-param)
};

struct SKeyBindings {
    unsigned char Key[KA_COUNT];       // action -> VK code (0 = unbound)
    unsigned char Reverse[256];        // VK code -> action (rebuilt from Key)

    void Clear();
    void LoadPreset(int scheme);
    void RebuildReverse();
    EKeyAction LookupAction(int vk) const;
    // Assign `vk` to `action`, clearing any other action that was using `vk`.
    void Assign(EKeyAction action, int vk);

    // User-input assign. Rejects system keys and duplicates. On ASSIGN_DUPLICATE,
    // *conflict (if non-null) is set to the action already using `vk`.
    EAssignResult TryAssign(EKeyAction action, int vk, EKeyAction* conflict);

    void LoadFromIni(SProperties* ini);
    void WriteToStream(SStream* out) const;

    const char* ActionLabel(int action) const;
    const char* VkLabel(int vk) const;           // caller-supplied buffer not needed; returns static

    static bool IsSystemKey(int vk);             // reserved keys that cannot be rebound
};

#endif // HD_KEYBINDS
#endif // COMMON_KEYBINDS_H
