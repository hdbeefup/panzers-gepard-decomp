// src/panzers/hud.cpp
// HUD widgets (hud.h). OWNER: agent H (docs/M3_INTERFACES.md). Skeleton.

#include "hud.h"
#include "m3common.h"
#include "stub_log.h"

SSpecInfoWidget::SSpecInfoWidget() { ToolTipFeatureEnabled = false; }
SSpecInfoWidget::~SSpecInfoWidget() {}

SUnitButton::SUnitButton() { ToolTipFeatureEnabled = false; }
SUnitButton::~SUnitButton() {}

void SUnitButton::OnMouseOver()
{
    STUB_LOG("SUnitButton::OnMouseOver (0x6251d0)");
    PZ_M3_TRACE("SUnitButton::OnMouseOver (0x6251d0)");
}

void SUnitButton::Redraw()
{
    STUB_LOG("SUnitButton::Redraw (0x626280)");
    PZ_M3_TRACE("SUnitButton::Redraw (0x626280)");
}

SHeroUnitButton::SHeroUnitButton() {}
SHeroUnitButton::~SHeroUnitButton() {}

void SHeroUnitButton::OnMouseOver()
{
    STUB_LOG("SHeroUnitButton::OnMouseOver (0x6251b0)");
    PZ_M3_TRACE("SHeroUnitButton::OnMouseOver (0x6251b0)");
}

void SHeroUnitButton::Redraw()
{
    STUB_LOG("SHeroUnitButton::Redraw (0x537d60)");
    PZ_M3_TRACE("SHeroUnitButton::Redraw (0x537d60)");
}

SCommandButton::SCommandButton() { ToolTipFeatureEnabled = false; }
SCommandButton::~SCommandButton() {}

void SCommandButton::OnMouseOver()
{
    STUB_LOG("SCommandButton::OnMouseOver (0x625180)");
    PZ_M3_TRACE("SCommandButton::OnMouseOver (0x625180)");
}

void SCommandButton::SetVisible(bool visible)
{
    STUB_LOG("SCommandButton::SetVisible (0x627da0)");
    PZ_M3_TRACE("SCommandButton::SetVisible (0x627da0)");
    SDXWidget::SetVisible(visible);
}

void SCommandButton::Redraw()
{
    STUB_LOG("SCommandButton::Redraw (0x626150)");
    PZ_M3_TRACE("SCommandButton::Redraw (0x626150)");
}

SMinimap::SMinimap() { ToolTipFeatureEnabled = false; }
SMinimap::~SMinimap() {}

void SMinimap::OnMouseDown(int button, int x, int y, int shift)
{
    STUB_LOG("SMinimap::OnMouseDown (0x64c3b0)");
    PZ_M3_TRACE("SMinimap::OnMouseDown (0x64c3b0)");
    SDXWidget::OnMouseDown(button, x, y, shift);
}

void SMinimap::OnMouseUp(int button, int x, int y, int shift)
{
    STUB_LOG("SMinimap::OnMouseUp (0x64c510)");
    PZ_M3_TRACE("SMinimap::OnMouseUp (0x64c510)");
    SDXWidget::OnMouseUp(button, x, y, shift);
}

void SMinimap::OnMouseMove(int x, int y, int shift)
{
    STUB_LOG("SMinimap::OnMouseMove (0x64c4a0)");
    PZ_M3_TRACE("SMinimap::OnMouseMove (0x64c4a0)");
    SDXWidget::OnMouseMove(x, y, shift);
}

void SMinimap::OnMouseOut()
{
    STUB_LOG("SMinimap::OnMouseOut (0x64c500)");
    PZ_M3_TRACE("SMinimap::OnMouseOut (0x64c500)");
    SDXWidget::OnMouseOut();
}

void SMinimap::SetVisible(bool visible)
{
    STUB_LOG("SMinimap::SetVisible (0x64c530)");
    PZ_M3_TRACE("SMinimap::SetVisible (0x64c530)");
    SDXWidget::SetVisible(visible);
}

void SMinimap::Create(int p1, int p2, int p3, int p4)
{
    STUB_LOG("SMinimap::Create (0x64c2f0)");
    PZ_M3_TRACE("SMinimap::Create (0x64c2f0)");
    (void)p1; (void)p2; (void)p3; (void)p4;
}
