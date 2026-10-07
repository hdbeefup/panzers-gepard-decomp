// panzers/chatline.cpp
// The game view's chat / cheat line (chatline.h). HD keeps the edit box and
// the check box inside SGameView (+0x5dc, +0x75c); the recompile's
// SGameView has no room for them (agent H's HUD owns that range), so they
// live in a small object next to the view, made and freed with the HUD.

#include <windows.h>
#include "chatline.h"
#include "cheats.h"
#include "gameview.h"
#include "savemenu.h"
#include "pzwidgets.h"
#include "pzboard.h"
#include "board.h"
#include "gettext.h"
#include "logger.h"

namespace {

struct SChatLine {
    pz::SEditBox  Edit;      // HD view +0x5dc
    pz::SCheckBox Allies;    // HD view +0x75c "Send message to allies only"
    SGameView*    View = nullptr;
};

SChatLine* s_Chat = nullptr;

SChatLine* Chat(SGameView* v) { return (s_Chat && s_Chat->View == v) ? s_Chat : nullptr; }

const char* Gv(const char* id) { return GetText("panzers/GameView.cpp", id); }   // 0x660c50

// HD 0x5439f0: walk up the parents making each widget the focused child.
void FocusWidget(SWidget* widget)
{
    for (SWidget* w = widget; w && w->Enabled && w->Visible && w->Parent; w = w->Parent) {
        if (w->Parent->Focus != w) {
            for (SWidget* c = w->Parent->Child; c; c = c->Sibling)
                if (c->FocusSibling == w)
                    c->FocusSibling = nullptr;
            w->FocusSibling = w->Parent->Focus;
            w->Parent->Focus = w;
        }
    }
}

// The open dialogs +0x3e48..+0x3e68 (OnKeyDown Enter opens the line only
// when none is open).
bool AnyDialog(SGameView* v)
{
    if (v->InGameMenu)                                            // +0x3e48 (recompile member)
        return true;
    const int* d = (const int*)((unsigned char*)static_cast<SGameViewData*>(v) + (0x3e48 - 0x5c));
    for (int i = 0; i < 9; ++i)
        if (d[i] != 0)
            return true;
    return false;
}

} // namespace

// PANZERS 0x619c90 (0x61bd6b..0x61be1c: the chat line)
void PzChatLineCreate(SGameView* v)
{
    PzChatLineDestroy(v);
    SChatLine* c = new SChatLine();
    s_Chat = c;
    c->View = v;
    // board +0x08(2, view frame, 0x1b8, 0x210, 0, 1)
    v->_5d8 = Board->CreateFrame(FT_TEXT, v->GetFrame(), 0x1b8, 0x210, 0, true);
    v->InsertChild(&c->Edit);                                     // vtbl +0x54
    c->Edit.SetPosition(0x1bd, 0x210, 0xf8, 0x12);                // vtbl +0x08
    c->Edit.Create(0, true);                                      // 0x53a750(0, 1)
    c->Edit.SetVisible(false);                                    // vtbl +0x6c(0)
    v->InsertChild(&c->Allies);
    c->Allies.SetPosition(0x190, 0x226, 0, 0);
    c->Allies.SetText(Gv("Send message to allies only"));         // 0x538110
    c->Allies.Create();                                           // 0x537f20
    c->Allies.SetCheck(true);                                     // 0x5380f0(1)
    c->Allies.SetVisible(false);
}

void PzChatLineDestroy(SGameView* v)
{
    SChatLine* c = Chat(v);
    if (!c)
        return;
    delete c;                                                     // the members unlink themselves from the view
    s_Chat = nullptr;
}

bool PzChatLineIsOpen(SGameView* v)
{
    SChatLine* c = Chat(v);
    return c && c->Edit.Visible;                                  // view +0x615
}

void PzChatLineHide(SGameView* v)
{
    if (v->_5d8 >= 0)
        Board->ShowFrame(v->_5d8, false);                         // board +0x18(+0x5d8, 0)
    if (SChatLine* c = Chat(v)) {
        c->Allies.SetVisible(false);                              // +0x75c vtbl +0x6c(0)
        c->Edit.SetVisible(false);                                // +0x5dc vtbl +0x6c(0)
    }
}

// PANZERS 0x622f50 (the Enter case with no end box: 0x623728..0x62417e)
// Single player only: the multiplayer chat (0x623750..0x623854: "To
// allies:" / "To everybody:", SMulti 0x521c00) needs SMulti.
void PzChatLineEnter(SGameView* v)
{
    SChatLine* c = Chat(v);
    if (!c)
        return;
    if (c->Edit.Visible) {                                        // +0x615: send the line
        PzCheatRun(c->Edit.GetText());                            // 0x623859..0x623fcb
        PzChatLineHide(v);                                        // 0x623fd1
        return;
    }
    if (AnyDialog(v))
        return;
    c->Allies.SetVisible(false);                                  // 0x6240f2
    c->Allies.SetCheck(false);                                    // 0x5380f0(0)
    Board->SetText(v->_5d8, g_PzFont[PZF_SANS21_SHADOW], 1, Gv("Cheat:"));   // board +0x34(+0x5d8, 3, 1, s)
    Board->ShowFrame(v->_5d8, true);                              // board +0x18(+0x5d8, 1)
    c->Edit.SetText("");                                          // 0x53ae10
    c->Edit.SetVisible(true);                                     // vtbl +0x6c(1)
    FocusWidget(&c->Edit);                                        // 0x5439f0
}
