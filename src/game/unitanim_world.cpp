// src/game/unitanim_world.cpp
// g_UnitAnimEnv bound to the game world (g_World, g_GameLogic). The animview
// test tool links its own binding instead. OWNER: agent A.

#include "unitanim.h"
#include "blockmap.h"
#include "buildingunit.h"
#include "gamelogic.h"
#include "worldapi.h"
#include "world.h"
#include "logger.h"

namespace pz {

static unsigned* WorldSeed()
{
    return &g_World->RandomSeed;                                  // World+0x7518
}

static float WorldTerrainHeight(float x, float z)
{
    return g_World->GetTerrainHeight(x, z);                       // 0x5e7730
}

static int WorldLocalPlayer()
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(g_World) + 0x16c);
}

static bool WorldNoFogOfWar()
{
    return *(reinterpret_cast<unsigned char*>(g_World) + 0x4d0) != 0;
}

static int WorldPlayerTeam(int player)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(g_World) + 0x17c + player * 0x48);
}

static bool WorldHasGameLogic()
{
    return g_GameLogic != nullptr;                                // DAT_008f2078
}

static bool WorldCanSeeGroundUnit(int player, SIUnit* unit)
{
    return g_GameLogic->CanSeeGroundUnit(player, unit);          // 0x562760
}

static int WorldFrame()
{
    return g_GameLogic->GetFrame();                               // 0x56d1a0
}

static SIUnit* WorldGetUnit(int index)
{
    if (!g_World->Units.IsLive(index))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", index);
    // The heap element type is the M1 stand-in until agent U replaces it
    // with SIUnit*.
    return reinterpret_cast<SIUnit*>(reinterpret_cast<void*>(g_World->Units.Array[index].Unit));
}

static int WorldGameLogicInt(unsigned offset)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(g_GameLogic) + offset);
}

static SIPixie* WorldPixie()
{
    return g_Pixie;
}

static bool WorldBuildingOccupiedByTeam(SIUnit* b, int player)
{
    return static_cast<SBuildingUnit*>(b)->IsOccupiedByTeam(player);   // 0x5468e0
}

static SIScene* WorldScene()
{
    return g_Scene;
}

static float WorldWaterHeight(float x, float z)
{
    return g_World->GetWaterHeight(x, z);                         // 0x5ec490
}
static bool WorldStaticBlocked(float x, float z, int size, unsigned mask)
{
    return BlockMap_CheckStatic(g_World, x, z, size, mask);       // 0x5d9e00
}
SUnitAnimEnv g_UnitAnimEnv = {
    WorldSeed, WorldTerrainHeight, WorldLocalPlayer, WorldNoFogOfWar, WorldPlayerTeam,
    WorldHasGameLogic, WorldCanSeeGroundUnit, WorldFrame, WorldGetUnit, WorldGameLogicInt,
    WorldPixie, WorldBuildingOccupiedByTeam, WorldScene, WorldWaterHeight, WorldStaticBlocked,
};

} // namespace pz
