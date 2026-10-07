// panzers/cheats.h
// The single-player cheat codes of SGameView::OnKeyDown 0x622f50 (the
// Enter case, 0x623859..0x624009). See docs/m5/ch.md for the list.
#pragma once

namespace pz { struct SPanzersCampaign; }

void PzCheatRun(const char* text);                         // 0x623859..0x623fcb
void PzCampaignSetCheated(pz::SPanzersCampaign* c, int v); // 0x597180 (+0xb84 unless it is 2)
