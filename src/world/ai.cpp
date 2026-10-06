// src/world/ai.cpp
// SWorld AI and the mission start / map load extras (world.h block "M3").
// OWNER: agent C (RefreshAI), agent F (the rest; docs/M3_INTERFACES.md).

#include <string.h>
#include "world.h"
#include "worldapi.h"
#include "doodad.h"
#include "blockmaprefresh.h"
#include "unit.h"
#include "pz/iterrain.h"
#include "pz/imodel.h"
#include "m3common.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// PANZERS 0x5f5b50
// Mission start: every ambient sound (+0x658 heap, 0x28 each, 0x5edeb0:
// Concert +0x38 3D loop at the position, +0x40 / +0x4c) and every water
// effect (+0x73e8 heap, 0xa0 each, 0x6015d0 with +0x73e4) starts; then
// +0x64c = 1. The map loader keeps AMBS raw and does not fill either heap,
// so only the flags are set here.
void SWorld::StartEffects()
{
    PZ_M3_TRACE("SWorld::StartEffects (0x5f5b50)");
    unsigned char* w = (unsigned char*)this;
    w[0x73e4] = 1;
    w[0x64c] = 1;
}

// SGameWorld::InitCameraSpline 0x609760: worldcamera.cpp (agent V).

// PANZERS 0x5e2d70
void SWorld::LoadMapExtra_5e2d70()
{
    PZ_M3_TRACE("SWorld::LoadMapExtra_5e2d70 (0x5e2d70)");
    if (Terrain)
        Terrain->SetCompactMode(!Terrain->GetCompactMode());      // terrain +0x30, +0x2c
}

// PANZERS 0x5debb0 (empty in HD)
void SWorld::LoadMapExtra_5debb0()
{
}

// PANZERS 0x607ad0
// Every live +0x73b0 record (0x38 bytes) with its +0x34 "dirty" flag gets
// its scene object rebuilt (scene +0xac bounds, +0xa0 remove, +0x9c create)
// and the block map refreshed over the old and new bounds. No loader fills
// the heap in the recompile, so it is checked and reported only.
void SWorld::LoadMapExtra_607ad0()
{
    PZ_M3_TRACE("SWorld::LoadMapExtra_607ad0 (0x607ad0)");
    const SHeap<unsigned char[0x34]>* h = (const SHeap<unsigned char[0x34]>*)((const unsigned char*)this + 0x73b0);
    for (int i = 0; i < h->Size; ++i)
        if (h->IsLive(i) && h->Array[i].Data[0x30] != 0) {
            Logger.g->Log(1, "STUB: SWorld 0x607ad0 record %d needs a scene object (scene +0x9c) not lifted", i);
            return;
        }
}

// The height patch of one model (SIModel +0xb0 0x6d6700): the higher of each
// vertex and the patch goes into the second height map, the rect is marked
// dirty for the water / height bits (0x600, 0x5ef380), the patch is freed
// (0x661b10, delete 0x14). Inline twice in 0x5e65f0 (doodads, buildings).
static void ApplyHeightPatch(SWorld* w, SIModel* m)
{
    SHeightPatch* p = m->GetHeightPatch();
    if (!p)
        return;
    for (int r = 0; r < p->H; ++r)
        for (int c = 0; c < p->W; ++c) {
            float v = p->Data[p->W * r + c];
            float* d = &w->AltHeights[(p->Z + r) * (w->TerrainW + 1) + p->X + c];
            if (*d <= v && v != *d)
                *d = v;
        }
    BlockMap_MarkDirty(w, p->X, p->Z, p->W + p->X, p->H + p->Z, 0x600);
    operator delete(p->Data);
    p->Data = nullptr;
    delete p;
}

// PANZERS 0x5e65f0
// Bridges: the second height map (+0xec) starts as a copy of the first and
// takes the higher of its value and the height patch (model +0xb0) of every
// doodad and building model; the "Platform" node is hidden. Then the block
// map is refreshed.
void SWorld::FixBridges()
{
    PZ_M3_TRACE("SWorld::FixBridges (0x5e65f0)");
    if (UseAltHeights)
        Logger.g->Panic("SWorld::FixBridges: HeightMapLayer2 is already enabled.");
    UseAltHeights = true;
    memcpy(AltHeights, Heights, (size_t)(TerrainH + 1) * (TerrainW + 1) * 4);   // 0x76b3a0
    for (int i = 0; i < Doodads.Size; ++i) {
        if (!Doodads.IsLive(i) || !Doodads.Array[i].Data.Model)   // +0x140, 200-byte records, model +0x24
            continue;
        SIModel* m = Doodads.Array[i].Data.Model;
        ApplyHeightPatch(this, m);
        m->SetNodeVisible(m->FindNode("Platform"), false);        // +0x40("Platform", 0), +0x60
    }
    for (int i = 0; i < Units.Size; ++i) {
        if (!Units.IsLive(i))
            continue;
        SUnit* u = Units.Array[i].Unit;
        if (u->Proto->ClassType != 9 || !u->Model)                // +0x04 +0x40, +0x08
            continue;
        ApplyHeightPatch(this, u->Model);
        u->Model->SetNodeVisible(u->Model->FindNode("Platform"), false);
    }
    RefreshBlockMapDirtyRect();                                   // 0x604620
}

} // namespace pz
