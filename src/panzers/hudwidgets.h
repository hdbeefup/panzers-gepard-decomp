// src/panzers/hudwidgets.h
// The HD generic widgets the in-game screens and the market are built from:
//   pz::SButton      HD SButton, vftable 0x7f2578, 0x74 bytes (ctor 0x537a20):
//                    a sprite button with normal / over / pressed / checked
//                    glyphs; base of SUnitButton, SCommandButton,
//                    SMarketCategoryButton (vtbl +0x7c = Redraw).
//   pz::STextBox     HD STextBox (SGenericListBox<STextBoxLine>), vftable
//                    0x7f3664, 0x140 bytes (ctor 0x541910): word-wrapped
//                    coloured lines (help, objectives, message boxes, the
//                    market description).
//   pz::SMessageBox  HD SMessageBox, vftable 0x7f312c, 0x3ec bytes (ctor
//                    0x53e0d0): the "Are you sure?" boxes (OK, Yes / No,
//                    Retry / Ignore); sends 0x4d581..0x4d585.
// The SWINE classes of the same names (src/window) draw SWINE skins and use
// other action codes, so these lifts live in namespace pz (as pzwidgets.h
// does). They derive from the SWINE SDXWidget for behaviour; HD offsets are
// given per member.
//
// OWNER: agent H (M3). These rows sat in agent E's generic-widget list; H
// needed them first for the HUD, the in-game menus and the market.

#ifndef PANZERS_HUDWIDGETS_H
#define PANZERS_HUDWIDGETS_H

#include "dxwidget.h"
#include "string2.h"
#include "pzwidgets.h"
#include "mainmenu.h"

// SMessageBox results (sent to the target / the parents, 0x53e890).
enum PzMessageBoxAction {
    PZA_MSGBOX_OK     = 0x4d581,
    PZA_MSGBOX_YES    = 0x4d582,
    PZA_MSGBOX_NO     = 0x4d583,
    PZA_MSGBOX_RETRY  = 0x4d584,
    PZA_MSGBOX_IGNORE = 0x4d585,
};

// SMessageBox::Create types (low word) and flags.
enum PzMessageBoxType {
    PZ_MB_OK          = 0,
    PZ_MB_YESNO       = 4,
    PZ_MB_NOBUTTONS   = 0x7b3,   // message_2_hq.tga background, no buttons
    PZ_MB_RETRYIGNORE = 0x7b4,
};

// HD board +0x74 LoadCustomFont with more than 256 glyphs. The HD board
// takes any count (menu/panzers_interface_hq.tga has 0x18a / 0x18b); the
// recompile's SFontProp holds 256, so the glyphs from 256 on go into a
// second board font, and PzSetSpriteGlyph picks the font by glyph. Use the
// returned handle with these helpers only. (A board change for E.)
int  PzLoadCustomFont(const char* filename, int count, const SCustomGlyph* glyphs);
void PzSetSpriteGlyph(int frame, int font, int glyph);              // board +0x24
void PzReleaseCustomFont(int font);                                 // board +0x80

namespace pz {

// HD SButton, 0x74 bytes.
struct SButton : SDXWidget {
    int  Font;          // 0x58
    int  GlyphNormal;   // 0x5c
    int  GlyphOver;     // 0x60
    int  GlyphDown;     // 0x64
    int  GlyphChecked;  // 0x68
    int  SpriteFrame;   // 0x6c
    bool Over;          // 0x70
    bool Pressed;       // 0x71
    bool Checked;       // 0x72
    bool Highlight;     // 0x73

    SButton();                                                       // 0x537a20
    ~SButton() override;                                             // 0x537a50

    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x537b30
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x537c40
    void OnMouseOver() override;                                     // +0x34 0x537bf0
    void OnMouseOut() override;                                      // +0x38 0x537bc0
    virtual void Redraw();                                           // +0x7c 0x537d60

    void Create(int font, int normal, int down, int over, int checked);  // 0x537a80
    void SetGlyphs(int font, int normal, int down, int over, int checked); // 0x537d10
    void SetOver(bool over);                                         // 0x537cf0
    void SetHighlight(bool highlight);                               // 0x537d40
    void SetChecked(bool checked);                                   // 0x537df0
};

// HD STextBoxLine (0xc bytes).
struct STextBoxLine {
    SString      Text;   // +0x00
    unsigned int Color;  // +0x08
};

// HD STextBox, 0x140 bytes (SGenericListBox fields 0x58..0x11f).
struct STextBox : SDXWidget {
    int  LineHeight;        // 0x58
    int  VisibleLines;      // 0x5c
    int  TopIndex;          // 0x60
    int  ColumnWidth;       // 0x64
    int  Columns;           // 0x68
    bool Selectable;        // 0x6c
    int  CurSel;            // 0x70
    int  Hover;             // 0x74
    float LastClickTime;    // 0x78
    bool HasScrollbar;      // 0x7c
    SScrollbar Scrollbar;   // 0x80
    STextBoxLine* Lines;    // 0x114 SDArray {array, size, max}
    int  LineCount;         // 0x118
    int  LineMax;           // 0x11c
    int  Font;              // 0x120
    int  MaxLines;          // 0x124 (10000)
    int* TextFrames;        // 0x128
    int  Align;             // 0x12c
    bool Center;            // 0x130 centre the lines vertically when they do not fill the box
    int  _134;              // 0x134 (-1)
    int  _138;              // 0x138 (0)
    bool _13c;              // 0x13c (1)

    STextBox();                                                      // 0x541910
    ~STextBox() override;                                            // 0x5419d0

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x5425f0
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x5426c0
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x542840
    void OnMouseWheel(int delta, int x, int y, int keys) override;   // +0x30 0x5428d0
    void OnMouseOut() override;                                      // +0x38 0x5428b0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x542540
    void Update() override;                                          // +0x78 0x542a00

    void Create(int font, int lines, bool background, bool scrollbars,
                int align, bool center);                             // 0x542380
    void AddLine(const char* text, unsigned int color);              // 0x541b60 (word wrap)
    void Clear();                                                    // 0x53e860 / 0x53e910 head
    int  SetTopIndex(int index);                                     // 0x542990

private:
    void GenericCreate(int lineHeight, int lines, int columnWidth, int columns,
                       bool selectable, bool background, bool scrollbars); // 0x5422e0
    int  PushLine(const char* text, int len, unsigned int color);   // 0x541af0 + copy
    void FreeLines();                                                // 0x541960
};

// HD SMessageBox, 0x3ec bytes.
struct SMessageBox : SDXWidget {
    STextBox       Text;          // 0x58
    SComplexButton OkButton;      // 0x198
    SComplexButton YesButton;     // 0x20c
    SComplexButton NoButton;      // 0x280
    SComplexButton RetryButton;   // 0x2f4
    SComplexButton IgnoreButton;  // 0x368
    int      Type;                // 0x3dc
    bool     Modal;               // 0x3e0
    SWidget* Target;              // 0x3e4 receives the result first (0x53e8d0)
    int      TitleFrame;          // 0x3e8

    SMessageBox();                                                   // 0x53e0d0
    ~SMessageBox() override;                                         // 0x53e190 (deleting 0x53e2d0)

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x53e7d0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x53e6e0
    void SetVisible(bool visible) override;                          // +0x6c 0x53e9a0

    void Create(const char* title, const char* text, unsigned int type, bool modal); // 0x53e3b0
    void SetText(const char* text, unsigned int color);              // 0x53e910
    void SetTitle(const char* title);                                // 0x53e8e0
    void SetTarget(SWidget* target) { Target = target; }             // 0x53e8d0

private:
    void SendResult(int action);                                     // 0x53e890
};

} // namespace pz

#endif // PANZERS_HUDWIDGETS_H
