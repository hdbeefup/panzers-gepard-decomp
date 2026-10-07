// src/panzers/loadgame.h
// Load Game (SSuperWindow::OnAction 0x494c1, SGameView 0x61f840), the
// Load Game list and the in-game save, and the recompile's save / load test
// hooks. OWNER: agent S (M4).

#ifndef PZ_LOADGAME_H
#define PZ_LOADGAME_H

struct SSuperWindow;
struct SGameView;
struct SString;
namespace pz { struct SGameLogic; }

int  PzSaveGameType(const char* file, SString* map);       // 0x5955d0
bool PzLoadGameAction(SSuperWindow* sw, const char* file); // 0x659250 case 0x494c1
void M4OnMainMenu(SSuperWindow* sw);                       // PZ_M4_LOADGAME
// PZ_M4_SAVE_AT: pz::M4SaveTestHook (gamelogic_save.cpp, called by SGameLogic::BeginFrame)
void PzM4AfterLoad(SGameView* v, const char* file);        // PZ_M4_RT
bool PzQuickSave(SGameView* v);                            // 0x622f50 F6
bool PzQuickLoad(SGameView* v);                            // 0x622f50 F9

#endif // PZ_LOADGAME_H
