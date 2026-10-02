// common/keybinds.cpp
// Rebindable keyboard controls implementation.
// Non-original: gated behind HD_KEYBINDS.

#include "keybinds.h"

#ifdef HD_KEYBINDS

#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "properties.h"
#include "stream.h"

// Preset binding tables — [KA_COUNT] entries, indexed by EKeyAction.
// 0 means unbound. Only primary key per action; modifiers stay hardcoded.

static const unsigned char PRESET_CLASSIC[KA_COUNT] = {
    /* KA_NONE                 */ 0,
    /* KA_PAUSE                */ VK_SPACE,
    /* KA_STEP_FRAME           */ 'N',
    /* KA_FAST_FORWARD         */ VK_OEM_PLUS,   // `+/=` key
    /* KA_CMD_ATTACK           */ 'A',
    /* KA_CMD_MOVE             */ 'M',
    /* KA_CMD_SPECACTION       */ 'X',
    /* KA_CMD_STOP             */ 'S',
    /* KA_CMD_CHASE            */ 'C',
    /* KA_CMD_FIRE_AT_WILL     */ 'F',
    /* KA_CMD_HOLD_FIRE        */ 'H',
    /* KA_CMD_RETREAT          */ 'R',
    /* KA_CMD_RETURN_FIRE      */ 'T',
    /* KA_QUICKBUY             */ VK_F1,
    /* KA_CAM_UP               */ VK_UP,
    /* KA_CAM_DOWN             */ VK_DOWN,
    /* KA_CAM_LEFT             */ VK_LEFT,
    /* KA_CAM_RIGHT            */ VK_RIGHT,
    /* KA_CAM_ROTATE_L         */ VK_INSERT,
    /* KA_CAM_ROTATE_R         */ VK_DELETE,
    /* KA_CAM_ELEVATION_UP     */ VK_HOME,
    /* KA_CAM_ELEVATION_DOWN   */ VK_END,
    /* KA_CAM_ZOOM_IN          */ VK_PRIOR,
    /* KA_CAM_ZOOM_OUT         */ VK_NEXT,
};

// WASD: camera on W/A/S/D; only Attack and Stop migrate to Q/E.
// Other commands keep their original letters (none collide with WASD).
// Camera rotate stays on Insert/Delete (same as Classic) to avoid stealing Q/E.
static const unsigned char PRESET_WASD[KA_COUNT] = {
    /* KA_NONE                 */ 0,
    /* KA_PAUSE                */ VK_SPACE,
    /* KA_STEP_FRAME           */ 'N',
    /* KA_FAST_FORWARD         */ VK_OEM_PLUS,
    /* KA_CMD_ATTACK           */ 'Q',
    /* KA_CMD_MOVE             */ 'M',
    /* KA_CMD_SPECACTION       */ 'X',
    /* KA_CMD_STOP             */ 'E',
    /* KA_CMD_CHASE            */ 'C',
    /* KA_CMD_FIRE_AT_WILL     */ 'F',
    /* KA_CMD_HOLD_FIRE        */ 'H',
    /* KA_CMD_RETREAT          */ 'R',
    /* KA_CMD_RETURN_FIRE      */ 'T',
    /* KA_QUICKBUY             */ VK_F1,
    /* KA_CAM_UP               */ 'W',
    /* KA_CAM_DOWN             */ 'S',
    /* KA_CAM_LEFT             */ 'A',
    /* KA_CAM_RIGHT            */ 'D',
    /* KA_CAM_ROTATE_L         */ VK_INSERT,
    /* KA_CAM_ROTATE_R         */ VK_DELETE,
    /* KA_CAM_ELEVATION_UP     */ VK_HOME,
    /* KA_CAM_ELEVATION_DOWN   */ VK_END,
    /* KA_CAM_ZOOM_IN          */ VK_PRIOR,
    /* KA_CAM_ZOOM_OUT         */ VK_NEXT,
};

// Comfy: WASD base + N=FastForward, =/+ =StepFrame, B=QuickBuy.
// (Comfy swaps StepFrame/FastForward vs WASD and frees F1 by moving QuickBuy to B.)
static const unsigned char PRESET_COMFY[KA_COUNT] = {
    /* KA_NONE                 */ 0,
    /* KA_PAUSE                */ VK_SPACE,
    /* KA_STEP_FRAME           */ VK_OEM_PLUS,
    /* KA_FAST_FORWARD         */ 'N',
    /* KA_CMD_ATTACK           */ 'Q',
    /* KA_CMD_MOVE             */ 'M',
    /* KA_CMD_SPECACTION       */ 'X',
    /* KA_CMD_STOP             */ 'E',
    /* KA_CMD_CHASE            */ 'C',
    /* KA_CMD_FIRE_AT_WILL     */ 'F',
    /* KA_CMD_HOLD_FIRE        */ 'H',
    /* KA_CMD_RETREAT          */ 'R',
    /* KA_CMD_RETURN_FIRE      */ 'T',
    /* KA_QUICKBUY             */ 'B',
    /* KA_CAM_UP               */ 'W',
    /* KA_CAM_DOWN             */ 'S',
    /* KA_CAM_LEFT             */ 'A',
    /* KA_CAM_RIGHT            */ 'D',
    /* KA_CAM_ROTATE_L         */ VK_INSERT,
    /* KA_CAM_ROTATE_R         */ VK_DELETE,
    /* KA_CAM_ELEVATION_UP     */ VK_HOME,
    /* KA_CAM_ELEVATION_DOWN   */ VK_END,
    /* KA_CAM_ZOOM_IN          */ VK_PRIOR,
    /* KA_CAM_ZOOM_OUT         */ VK_NEXT,
};

// Action ini key names (must be stable across versions).
static const char* kActionIniKey[KA_COUNT] = {
    "",
    "Pause",
    "StepFrame",
    "FastForward",
    "CmdAttack",
    "CmdMove",
    "CmdSpecAction",
    "CmdStop",
    "CmdChase",
    "CmdFireAtWill",
    "CmdHoldFire",
    "CmdRetreat",
    "CmdReturnFire",
    "QuickBuy",
    "CamUp",
    "CamDown",
    "CamLeft",
    "CamRight",
    "CamRotateLeft",
    "CamRotateRight",
    "CamElevationUp",
    "CamElevationDown",
    "CamZoomIn",
    "CamZoomOut",
};

// User-facing labels.
static const char* kActionLabel[KA_COUNT] = {
    "",
    "Pause / Resume",
    "Step Frame",
    "Fast Forward",
    "Attack Command",
    "Move Command",
    "Special Action",
    "Stop",
    "Chase",
    "Fire at Will",
    "Hold Fire",
    "Retreat",
    "Return Fire",
    "Quick Buy",
    "Camera Up",
    "Camera Down",
    "Camera Left",
    "Camera Right",
    "Rotate Left",
    "Rotate Right",
    "Elevation Up",
    "Elevation Down",
    "Zoom In",
    "Zoom Out",
};

void SKeyBindings::Clear()
{
    memset(Key, 0, sizeof(Key));
    memset(Reverse, KA_NONE, sizeof(Reverse));
}

void SKeyBindings::LoadPreset(int scheme)
{
    const unsigned char* src = PRESET_CLASSIC;
    if (scheme == CS_WASD)       src = PRESET_WASD;
    else if (scheme == CS_COMFY) src = PRESET_COMFY;
    memcpy(Key, src, sizeof(Key));
    RebuildReverse();
}

void SKeyBindings::RebuildReverse()
{
    memset(Reverse, KA_NONE, sizeof(Reverse));
    for (int a = 1; a < KA_COUNT; ++a) {
        unsigned char vk = Key[a];
        if (vk && Reverse[vk] == KA_NONE)
            Reverse[vk] = (unsigned char)a;
    }
}

EKeyAction SKeyBindings::LookupAction(int vk) const
{
    if (vk < 0 || vk > 0xFF) return KA_NONE;
    return (EKeyAction)Reverse[vk];
}

void SKeyBindings::Assign(EKeyAction action, int vk)
{
    if (action <= KA_NONE || action >= KA_COUNT) return;
    // If vk was bound to another action, unbind it.
    if (vk > 0 && vk <= 0xFF) {
        for (int a = 1; a < KA_COUNT; ++a) {
            if (Key[a] == vk) Key[a] = 0;
        }
    }
    Key[action] = (unsigned char)(vk & 0xFF);
    RebuildReverse();
}

bool SKeyBindings::IsSystemKey(int vk)
{
    if (vk >= '1' && vk <= '9') return true;          // unit group slots
    if (vk >= VK_F1 && vk <= VK_F12) return true;     // menu hotkeys
    switch (vk) {
        case VK_RETURN:  // chat open / cheat submit
        case VK_TAB:     // chat team toggle
        case VK_ESCAPE:  // in-game menu
        case VK_SHIFT:   // queued-command modifier
        case VK_CONTROL: // group-create modifier
        case VK_MENU:    // Alt — reserved by OS
        case VK_LWIN: case VK_RWIN:
            return true;
    }
    return false;
}

EAssignResult SKeyBindings::TryAssign(EKeyAction action, int vk, EKeyAction* conflict)
{
    if (conflict) *conflict = KA_NONE;
    if (action <= KA_NONE || action >= KA_COUNT) return ASSIGN_SYSTEM_KEY;

    // vk == 0 means "clear this binding" — bypass duplicate + system-key checks.
    if (vk == 0) {
        Key[action] = 0;
        RebuildReverse();
        return ASSIGN_OK;
    }

    if (vk < 0 || vk > 0xFF) return ASSIGN_SYSTEM_KEY;

    // F-keys and friends are reserved for everything except Quick Buy, whose
    // default IS F1. Letting the user park QuickBuy on F1-F12 keeps the
    // original muscle memory working; other actions still can't steal F-keys.
    if (action != KA_QUICKBUY && IsSystemKey(vk)) return ASSIGN_SYSTEM_KEY;

    // Duplicate check — allow re-binding the same action to the same key (no-op).
    for (int a = 1; a < KA_COUNT; ++a) {
        if (a != (int)action && Key[a] == vk) {
            if (conflict) *conflict = (EKeyAction)a;
            return ASSIGN_DUPLICATE;
        }
    }

    Key[action] = (unsigned char)(vk & 0xFF);
    RebuildReverse();
    return ASSIGN_OK;
}

void SKeyBindings::LoadFromIni(SProperties* ini)
{
    // Start from classic defaults, then overlay INI values.
    LoadPreset(CS_CLASSIC);
    if (!ini) return;
    for (int a = 1; a < KA_COUNT; ++a) {
        int v = ini->GetInt("Keyboard bindings", kActionIniKey[a], Key[a]);
        if (v < 0) v = 0;
        if (v > 0xFF) v = 0xFF;
        Key[a] = (unsigned char)v;
    }
    RebuildReverse();
}

void SKeyBindings::WriteToStream(SStream* out) const
{
    if (!out) return;
    char buf[128];
    strcpy(buf, "[Keyboard bindings]\r\n\r\n");
    out->Write(buf, (int)strlen(buf));
    strcpy(buf, "; VK code for each action (only used when Control scheme = 2 / Custom)\r\n\r\n");
    out->Write(buf, (int)strlen(buf));
    for (int a = 1; a < KA_COUNT; ++a) {
        sprintf(buf, "%s = %d\r\n", kActionIniKey[a], Key[a]);
        out->Write(buf, (int)strlen(buf));
    }
    strcpy(buf, "\r\n\r\n");
    out->Write(buf, (int)strlen(buf));
}

const char* SKeyBindings::ActionLabel(int action) const
{
    if (action <= KA_NONE || action >= KA_COUNT) return "";
    return kActionLabel[action];
}

const char* SKeyBindings::VkLabel(int vk) const
{
    // Static per-call buffer is unsafe, but the UI calls this one row at a time
    // and copies the string into the listbox immediately, so a rotating pool is fine.
    static char pool[8][16];
    static int cursor = 0;
    char* buf = pool[cursor];
    cursor = (cursor + 1) & 7;

    if (vk == 0) { strcpy(buf, "(unbound)"); return buf; }

    // Named keys first
    switch (vk) {
        case VK_SPACE:   strcpy(buf, "Space");     return buf;
        case VK_TAB:     strcpy(buf, "Tab");       return buf;
        case VK_RETURN:  strcpy(buf, "Enter");     return buf;
        case VK_ESCAPE:  strcpy(buf, "Esc");       return buf;
        case VK_LEFT:    strcpy(buf, "Left");      return buf;
        case VK_RIGHT:   strcpy(buf, "Right");     return buf;
        case VK_UP:      strcpy(buf, "Up");        return buf;
        case VK_DOWN:    strcpy(buf, "Down");      return buf;
        case VK_PRIOR:   strcpy(buf, "PgUp");      return buf;
        case VK_NEXT:    strcpy(buf, "PgDn");      return buf;
        case VK_HOME:    strcpy(buf, "Home");      return buf;
        case VK_END:     strcpy(buf, "End");       return buf;
        case VK_INSERT:  strcpy(buf, "Ins");       return buf;
        case VK_DELETE:  strcpy(buf, "Del");       return buf;
        case VK_BACK:    strcpy(buf, "Backspace"); return buf;
    }
    switch (vk) {
        case VK_OEM_PLUS:    strcpy(buf, "+ / =");  return buf;
        case VK_OEM_MINUS:   strcpy(buf, "- / _");  return buf;
        case VK_OEM_COMMA:   strcpy(buf, ",");      return buf;
        case VK_OEM_PERIOD:  strcpy(buf, ".");      return buf;
        case VK_OEM_1:       strcpy(buf, ";");      return buf;
        case VK_OEM_2:       strcpy(buf, "/");      return buf;
        case VK_OEM_3:       strcpy(buf, "`");      return buf;
        case VK_OEM_4:       strcpy(buf, "[");      return buf;
        case VK_OEM_5:       strcpy(buf, "\\");     return buf;
        case VK_OEM_6:       strcpy(buf, "]");      return buf;
        case VK_OEM_7:       strcpy(buf, "'");      return buf;
    }
    if (vk >= VK_F1 && vk <= VK_F12) { sprintf(buf, "F%d", vk - VK_F1 + 1); return buf; }
    if (vk >= '0' && vk <= '9')      { sprintf(buf, "%c", vk); return buf; }
    if (vk >= 'A' && vk <= 'Z')      { sprintf(buf, "%c", vk); return buf; }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) { sprintf(buf, "Num%d", vk - VK_NUMPAD0); return buf; }

    sprintf(buf, "VK 0x%02X", vk);
    return buf;
}

#endif // HD_KEYBINDS
