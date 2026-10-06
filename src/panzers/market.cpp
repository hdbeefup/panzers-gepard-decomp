// src/panzers/market.cpp
// SMarket (market.h). OWNER: agent H (docs/M3_INTERFACES.md). Skeleton
// stand-in: a centred SRightMenu "Headquarters" with Start Mission / Cancel.
// HD asks "Are you sure?" before Start Mission; the stand-in sends 0x4d542
// at once.

#include "market.h"
#include "m3common.h"
#include "stub_log.h"
#include "gettext.h"

SMarket::SMarket()
{
    STUB_LOG("SMarket::SMarket (0x63f2f0)");
    PZ_M3_TRACE("SMarket::SMarket (0x63f2f0)");
}

SMarket::~SMarket()
{
    STUB_LOG("SMarket::~SMarket (0x63fa10)");
    PZ_M3_TRACE("SMarket::~SMarket (0x63fa10)");
}

void SMarket::Create()
{
    STUB_LOG("SMarket::Create (0x6407d0)");
    PZ_M3_TRACE("SMarket::Create (0x6407d0)");
    SRightMenu::Create(GetText("panzers/Market.cpp", "Headquarters"), false);
    InsertChild(&StartMission);
    StartMission.SetPosition((Width - 0x100) / 2, 0x32, 0, 0);
    StartMission.Create(2, GetText("panzers/Market.cpp", "Start Mission"));
    InsertChild(&CancelButton);
    CancelButton.SetPosition((Width - 0x100) / 2, 0x189, 0, 0);
    CancelButton.Create(2, GetText("panzers/Market.cpp", "Cancel"));
    Cursor = 0;
}

bool SMarket::OnAction(SWidget* source, int action, int param)
{
    STUB_LOG("SMarket::OnAction (0x647d00)");
    PZ_M3_TRACE("SMarket::OnAction (0x647d00)");
    (void)param;
    if (action != PZA_BUTTON_CLICK)
        return false;
    if (source == &StartMission)
        SendAction(PZA_MARKET_START, 0);
    else if (source == &CancelButton)
        SendAction(PZA_MARKET_CANCEL, 0);
    return true;
}

void SMarket::Update()
{
    STUB_LOG("SMarket::Update (0x64b380)");
    PZ_M3_TRACE("SMarket::Update (0x64b380)");
    SRightMenu::Update();
}

void SMarket::LoadUnitInfo()
{
    STUB_LOG("SMarket::LoadUnitInfo (0x6450c0)");
    PZ_M3_TRACE("SMarket::LoadUnitInfo (0x6450c0)");
}

void SMarket::SaveArmy(int p1)
{
    STUB_LOG("SMarket::SaveArmy (0x64a180)");
    PZ_M3_TRACE("SMarket::SaveArmy (0x64a180)");
    (void)p1;
}
