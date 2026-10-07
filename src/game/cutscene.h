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
struct SIScene;
struct SIViewport;

// 0x56ea20(name, len): picks .ingame (0x56f530) or .4d (0x56f0d0).
void PzCutscenePlay(SGameLogic* gl, const char* name);
// 0x568af0: statements due this frame; ends the cut-scene after its length.
void PzCutsceneTick(SGameLogic* gl);
// 0x565390: end of the .ingame cut-scene (restores the camera and the HUD state).
void PzCutsceneEnd(SGameLogic* gl);
// 0x58adc0 from UpdateUnitVisuals 0x5638f0: the cut-scene camera between ticks.
void PzCutsceneCamera(SGameLogic* gl, double interpolation);
// 0x582380 SGameLogic::UpdateAnimation: true while a .4d cut-scene plays
// (SGameView::Update then does nothing else).
bool PzCutsceneUpdateAnimation(SGameLogic* gl, SIViewport* vp);
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

// The subtitles (src/panzers/cutscene_sub.cpp, M5 agent CS).
void PzSubtitlesLoad(const char* file);                            // 0x56fd70 SSubtitler::ParseSubFile + the frame
void PzSubtitlesShow(int time);                                    // 0x5826c0 (time in 1/25 s)
void PzSubtitlesEnd();                                             // 0x565390 / 0x5652d0 tail
void PzCutsceneSoundPause();                                       // Concert +0x60
void PzCutsceneSoundResume();                                      // Concert +0x64
int  PzCutsceneSoundStart(const char* file);                       // Concert +0x2c(file, 1, 0, 0, 1)
void PzCutsceneSoundStop(int id);                                  // Concert +0x3c

// SGameLogic +0x00 (the view's SIGameViewCallback) calls
// (src/panzers/gameview_mapcut.cpp).
void PzViewLoadMapInPlace(int callback, const char* map);          // vtbl +0x10 0x624770
void PzViewResetClock(int callback);                               // vtbl +0x0c 0x624730
void PzViewSetScene(int callback, SIScene* scene);                // vtbl +0x08 0x625560

// RunTriggers case 0x38 (triggers.cpp): true when the map cut-scene
// replaced the map (the running triggers are gone).
bool PzMapCutscene(SGameLogic* gl, const char* name);
// Recompile-only test hook PZ_M5_CS_FORCE=<frame>:<name> (inert when unset):
// at that logic frame the trigger action 0x38 with <name> runs.
void PzCutsceneTestHook(SGameLogic* gl);

} // namespace pz

#endif // PZ_CUTSCENE_H
