// panzers/chatline.h
// The game view's one-line text entry ("chat line"): HD SGameView keeps a
// board text frame at +0x5d8 (the prompt), a pz::SEditBox at +0x5dc and a
// "Send message to allies only" SCheckBox at +0x75c. Create 0x619c90 makes
// them (0x61bd6b..0x61be1c, hidden), OnKeyDown 0x622f50 Enter opens the
// line ("Cheat:" in single player), Enter again sends it (cheats.cpp; in
// multiplayer the chat, not in the recompile), Esc closes it.
#pragma once

struct SGameView;

void PzChatLineCreate(SGameView* v);    // Create 0x619c90 part (0x61bd6b..0x61be1c)
void PzChatLineDestroy(SGameView* v);   // (recompile) the widgets live outside the view object
bool PzChatLineIsOpen(SGameView* v);    // HD view +0x615 (= SEditBox +0x5dc visible)
void PzChatLineHide(SGameView* v);      // frame +0x5d8, +0x75c and +0x5dc hidden
void PzChatLineEnter(SGameView* v);     // OnKeyDown 0x622f50 Enter, 0x623728..0x62417e
