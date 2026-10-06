// src/game/unitsave.cpp
// SUnit save / load (unit.h block "M3"): the per-unit part of the save game
// that every mission start writes (SPanzersCampaign::SaveGameStartMission
// 0x596e30 -> SaveGame 0x5966a0 -> SGameLogic 0x57e110 -> SUnit::Save), and
// the generic variable writer gSaveVariables (unitsave.h). OWNER: agent F
// (docs/M3_INTERFACES.md); agent C owns the rest of SUnit.

#include <string.h>
#include "unit.h"
#include "unitsave.h"
#include "m3common.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"
#include "doodad.h"

namespace pz {

// PANZERS 0x670af0
void SaveSingleVariable(SStream* s, const void* src, int type, const SVarDesc* members)
{
    const unsigned char* p = (const unsigned char*)src;
    switch (type) {
    case 1: s->WriteByte(*p != 0); return;                        // 0x65daf0
    case 2: s->WriteInt(*(const int*)p); return;                  // 0x65dc40
    case 3: s->WriteFloat(*(const float*)p); return;              // 0x65dc20
    case 4:
        s->WriteFloat(((const float*)p)[0]);
        s->WriteFloat(((const float*)p)[1]);
        s->WriteFloat(((const float*)p)[2]);
        return;
    case 5: s->WriteString(SStr(*(const SString*)p)); return;     // 0x5b2480
    case 6:
        s->WriteChunkStart(0x6c636d5f);                           // "_mcl"
        SaveVariables(s, p, members);
        s->WriteChunkEnd();
        return;
    case 8:
        s->WriteFloat(((const float*)p)[0]);
        s->WriteFloat(((const float*)p)[1]);
        return;
    case 12: s->WriteWord(*(const unsigned short*)p); return;     // 0x65dd00
    default:
        Logger.g->Panic("gSaveSingleVariable: Not a Single type");
    }
}

// PANZERS 0x670c50
void SaveVariables(SStream* s, const void* obj, const SVarDesc* desc)
{
    if (!obj)
        Logger.g->Panic("::gSaveVariables: NULL parameter");
    for (const SVarDesc* d = desc;; ++d) {
        s->WriteInt(d->Type);
        if (d->Type == 0)
            return;
        s->WriteString(d->Name ? d->Name : "");                   // 0x65dca0
        const int* a = (const int*)((const unsigned char*)obj + d->Offset);   // SDArray {array, size, max, ...}
        switch (d->Type) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 8: case 12:
            SaveSingleVariable(s, a, d->Type, d->Members);
            break;
        case 10:
            s->WriteInt(d->ElemType);
            s->WriteInt(a[1]);
            for (int i = 0; i < a[1]; ++i)
                SaveSingleVariable(s, (const unsigned char*)(size_t)a[0] + d->ElemSize * i, d->ElemType, d->Members);
            break;
        case 11: {
            // SDEQueue {array, size, max, base, last (+0x10), first (+0x14)}
            s->WriteInt(d->ElemType);
            s->WriteInt(a[1]);
            s->WriteInt(a[5]);
            s->WriteInt(a[4]);
            if (a[1] != 0 && a[1] != a[4] - a[5] + 1)
                Logger.g->Panic("gSaveVariables: DEQueue is damaged");
            for (int i = a[5]; i <= a[4]; ++i) {
                int k = i - a[3];
                k = k < a[2] ? k : k - a[2];
                SaveSingleVariable(s, (const unsigned char*)(size_t)a[0] + d->ElemSize * k, d->ElemType, d->Members);
            }
            break;
        }
        default:
            break;                                                // HD writes only the type and the name
        }
    }
}

void SUnit::Save(struct SStream* s)
{
    STUB_LOG("SUnit::Save (0x5be320)");
    PZ_M3_TRACE("SUnit::Save (0x5be320)");
    // HD: 'vars' {gSaveVariables(class descriptor of vtbl +0x1c)}, 'gunn'
    // {count, each gunner's variables (+0x30)}, 'driv' {count, each driver's
    // variables (+0x50)}, then 'targ' when targets are set (the STarget list
    // and their indices). The class descriptor tables are not lifted.
    (void)s;
}

void SUnit::Load(struct SStream* s)
{
    STUB_LOG("SUnit::Load (0x5bbd30)");
    PZ_M3_TRACE("SUnit::Load (0x5bbd30)");
    (void)s;
}

} // namespace pz
