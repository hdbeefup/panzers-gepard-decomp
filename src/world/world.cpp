// src/world/world.cpp
// pz::SWorld skeleton. OWNER: agent D. Lift from the HD exe only (SWINE
// world/ and game/ are banned). See src/3dengine/pz/pzscene.cpp for the
// stub rules.

#include <string.h>
#include "world.h"
#include "worldapi.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "stub_log.h"

namespace pz {

SWorld::SWorld(int p1)
{
    STUB_LOG("SWorld::SWorld (0x5d2f90)");
    PZ_TRACE("SWorld::SWorld (0x5d2f90)");
    (void)p1;
    memset(_04, 0, sizeof(_04));
    // HD 0x5d2f90 (the parts the 3D path needs; the rest is agent D's):
    //   if (g_World) delete g_World; g_World = this;
    //   ... g_Scene = Gepard +0x0c CreateScene(); scene +0x08 SetAtmosphere
    //   ("" until ATMS is read), Gepard +0x44 LoadTexture x5,
    //   Gepard +0x20 LoadModelPrototype("units/flag/*-hero.4D", 0.005) x3, ...
    if (g_World)
        delete g_World;
    g_World = this;
    g_Scene = PzGepard()->CreateScene();
    g_Scene->SetAtmosphere("");
}

SWorld::~SWorld()
{
    STUB_LOG("SWorld::~SWorld (0x5d68b0)");
    PZ_TRACE("SWorld::~SWorld (0x5d68b0)");
    // HD: the world dtor releases the scene ("Scene destroyed").
    if (g_Scene) {
        g_Scene->Release();
        g_Scene = nullptr;
    }
    if (g_World == this)
        g_World = nullptr;
}

void SWorld::Slot_04()
{
    STUB_LOG("SWorld::Slot_04 (0x5ec2a0)");
    PZ_TRACE("SWorld::Slot_04 (0x5ec2a0)");
}

void SWorld::Slot_08()
{
    STUB_LOG("SWorld::Slot_08 (0x5fec80)");
    PZ_TRACE("SWorld::Slot_08 (0x5fec80)");
}

void SWorld::Slot_0C()
{
    STUB_LOG("SWorld::Slot_0C (0x5ec0c0)");
    PZ_TRACE("SWorld::Slot_0C (0x5ec0c0)");
}

void SWorld::Slot_10()
{
    STUB_LOG("SWorld::Slot_10 (0x5ebee0)");
    PZ_TRACE("SWorld::Slot_10 (0x5ebee0)");
}

void SWorld::ShowLoadingIcon(int parentFrame)
{
    STUB_LOG("SWorld::ShowLoadingIcon (0x5edca0)");
    PZ_TRACE("SWorld::ShowLoadingIcon (0x5edca0)");
    (void)parentFrame;
}

void SWorld::HideLoadingIcon()
{
    STUB_LOG("SWorld::HideLoadingIcon (0x5dc7d0)");
    PZ_TRACE("SWorld::HideLoadingIcon (0x5dc7d0)");
}

bool SWorld::LoadMap(SStream* stream, bool p2, int p3, int p4)
{
    STUB_LOG("SWorld::LoadMap (0x5f1990)");
    PZ_TRACE("SWorld::LoadMap (0x5f1990)");
    (void)stream; (void)p2; (void)p3; (void)p4;
    // HD 0x5f1990 starts by drawing a loading frame: Gepard +0x3c
    // GetViewport(0) -> +0x50 Render(0, 0) (no scene).
    if (SIViewport* vp = PzGepard()->GetViewport(0))
        vp->Render(nullptr, 0);
    return true;
}

void SWorld::Initialize()
{
    STUB_LOG("SWorld::Initialize (0x5eec90)");
    PZ_TRACE("SWorld::Initialize (0x5eec90)");
}

void SWorld::ComputeCamera(SIViewport* vp)
{
    STUB_LOG("SWorld::ComputeCamera (0x5ddc30)");
    PZ_TRACE("SWorld::ComputeCamera (0x5ddc30)");
    // HD mode 0 (menu.map): vp +0x20 SetCamera(eye x, y, z, yaw +0x44,
    // -pitch +0x50); Concert +0x08 listener; scene +0x24 SetFocusHeight;
    // if dirty (+0x34): vp +0x28 SetProjection(60 deg, +0x2c, +0x30).
    // Placeholder values until the CAM chunk (0x5fd7c0) is lifted.
    static bool projectionSet = false;
    if (!vp)
        return;
    vp->SetCamera(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    if (g_Scene)
        g_Scene->SetFocusHeight(0.0f);
    if (!projectionSet) {
        vp->SetProjection(1.0471976f, 1.0f, 1000.0f);   // 0x3f860a92; near/far are placeholders
        projectionSet = true;
    }
}

} // namespace pz
