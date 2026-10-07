// src/game/cutscene.h
// In-game cut-scenes (M4 agent T). HD has two kinds, both started by
// SGameLogic 0x56ea20 (trigger action 0x38 "Play cut-scene", and the
// -cutscene switch of SGameView::LoadMap):
//  - "cutscenes/<n>/<n>.ingame" (SInGameAnimLogic, global 0x929100): a
//    script of ExecuteScriptStatement lines run by the game logic at fixed
//    frames while the world keeps ticking (0x56f530 start, 0x568af0 per
//    tick, 0x565390 end). The tutorial uses these.
//  - "cutscenes/<n>/<n>.4d" (0x56f0d0): a separate scene with its own camera
//    node, played on the view clock (SGameLogic +0x288).
// The recompile lifts the logic side of the .ingame kind (statements, start
// and end state). The camera splines (CAM*/CMS*), the camera colour fades,
// the letterbox, the subtitles and the cut-scene mp3 are not drawn / played
// (logged once).
#ifndef PZ_CUTSCENE_H
#define PZ_CUTSCENE_H

namespace pz {

struct SGameLogic;

// 0x56ea20(name, len): picks .ingame (0x56f530) or .4d (0x56f0d0).
void PzCutscenePlay(SGameLogic* gl, const char* name);
// 0x568af0: statements due this frame; ends the cut-scene after its length.
void PzCutsceneTick(SGameLogic* gl);
// 0x565390: end of the .ingame cut-scene (restores the camera and the HUD state).
void PzCutsceneEnd(SGameLogic* gl);
// 0x58adc0 from UpdateUnitVisuals 0x5638f0: the cut-scene camera between ticks.
void PzCutsceneCamera(SGameLogic* gl, double interpolation);
// True while an .ingame cut-scene runs (SGameLogic +0x2b8).
bool PzCutsceneRunning();
// 0x589860: a camera track of the loaded cut-scene has more than one key.
bool PzCutsceneHasCamera();

// SGameLogic::ExecuteScriptStatement 0x568bc0 (triggers_script.cpp).
// preprocess = the second argument (1 when 0x56f530 walks the statements
// before the cut-scene starts: only "create" / "create2" / "create_model"
// do work then).
void PzExecuteScriptStatement(SGameLogic* gl, const char* text, bool preprocess);

// The message lines of the trigger actions (src/panzers/tutorial_msg.cpp).
void PzMessagesTick(SGameLogic* gl);                               // 0x578b00 (fading lines)
void PzMessagesClearFading(SGameLogic* gl);                        // 0x563860
void PzMessagesClearStatic(SGameLogic* gl);                        // 0x5638b0
void PzMessageStatic(SGameLogic* gl, const char* text, int color); // 0x5815e0
void PzMessageFading(SGameLogic* gl, const char* text, int color); // 0x56a480 SGameLogic::StaticMessage
void PzTriggerTextBlock(SGameLogic* gl, const char* key, bool fading);   // actions 0x40 / 0x41
void PzCutsceneOverlay(unsigned argb);                             // 0x58c5d0 (the cut-scene colour)

} // namespace pz

#endif // PZ_CUTSCENE_H
