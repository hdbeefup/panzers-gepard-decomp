// src/panzers/hudwidgets.cpp
// HD SButton, STextBox and SMessageBox (hudwidgets.h). OWNER: agent H.
//
// HD board slots as in pzwidgets.cpp: +0x08 CreateFrame, +0x10 MoveFrame,
// +0x14 ResizeFrame, +0x18 ShowFrame, +0x20 GetFrameSize, +0x24
// SetSpriteGlyph, +0x28 SetTextColor, +0x34 SetText(frame, font, align,
// text, 0), +0x88 GetTextExtent(font, text, len, &w, &h). HD font indices
// 0..5 go through g_PzFont.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "hudwidgets.h"
#include "pzboard.h"
#include "board.h"
#include "iconcert.h"
#include "logger.h"
#include "timer.h"
#include "gettext.h"
#include "window.h"

static int HwFont(int hdFont)
{
    return (hdFont >= 0 && hdFont < 6) ? g_PzFont[hdFont] : hdFont;
}

static void HwSound(const char* name, float db)
{
    if (Concert)
        Concert->PlaySound(name, db, 0, -1);                      // concert +0x50
}

// Second halves of the fonts loaded through PzLoadCustomFont (by the
// first half's board handle).
static int s_BigFontHi[64];
static int s_BigFontLo[64];
static int s_BigFontCount = 0;

int PzLoadCustomFont(const char* filename, int count, const SCustomGlyph* glyphs)
{
    if (count <= 0x100)
        return Board->LoadCustomFont(filename, count, const_cast<SCustomGlyph*>(glyphs), Default);
    int lo = Board->LoadCustomFont(filename, 0x100, const_cast<SCustomGlyph*>(glyphs), Default);
    int hi = Board->LoadCustomFont(filename, count - 0x100, const_cast<SCustomGlyph*>(glyphs + 0x100), Default);
    if (s_BigFontCount < 64) {
        s_BigFontLo[s_BigFontCount] = lo;
        s_BigFontHi[s_BigFontCount] = hi;
        ++s_BigFontCount;
    }
    return lo;
}

static int BigFontHi(int font)
{
    for (int i = 0; i < s_BigFontCount; ++i)
        if (s_BigFontLo[i] == font)
            return s_BigFontHi[i];
    return -1;
}

void PzSetSpriteGlyph(int frame, int font, int glyph)
{
    if (glyph >= 0x100) {
        int hi = BigFontHi(font);
        if (hi >= 0) {
            Board->SetSpriteGlyph(frame, hi, glyph - 0x100);
            return;
        }
    }
    Board->SetSpriteGlyph(frame, font, glyph);
}

void PzReleaseCustomFont(int font)
{
    if (font < 0)
        return;
    for (int i = 0; i < s_BigFontCount; ++i) {
        if (s_BigFontLo[i] == font) {
            Board->ReleaseFont(s_BigFontHi[i]);
            s_BigFontLo[i] = s_BigFontLo[s_BigFontCount - 1];
            s_BigFontHi[i] = s_BigFontHi[s_BigFontCount - 1];
            --s_BigFontCount;
            break;
        }
    }
    Board->ReleaseFont(font);
}

namespace pz {

// ===========================================================================
// SButton
// ===========================================================================

// PANZERS 0x537a20
SButton::SButton()
{
    ToolTipFeatureEnabled = false;
    Font = -1;
    GlyphNormal = GlyphOver = GlyphDown = GlyphChecked = -1;
    SpriteFrame = -1;                                              // [0x1b] = -1
    Over = Pressed = Checked = Highlight = false;                  // [0x1c] = 0
}

// PANZERS 0x537a50
SButton::~SButton()
{
}

// PANZERS 0x537a80
void SButton::Create(int font, int normal, int down, int over, int checked)
{
    SDXWidget::Create(0);                                          // 0x539a10
    Font = font;
    GlyphNormal = normal;
    GlyphOver = over;
    GlyphDown = down;
    GlyphChecked = checked;
    SpriteFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 0);
    Redraw();                                                      // vtbl +0x7c
}

// PANZERS 0x537d10
void SButton::SetGlyphs(int font, int normal, int down, int over, int checked)
{
    Font = font;
    GlyphNormal = normal;
    GlyphOver = over;
    GlyphDown = down;
    GlyphChecked = checked;
    Redraw();
}

// PANZERS 0x537d60
void SButton::Redraw()
{
    if (BackFrame < 0)
        return;
    int glyph;
    if (Checked) {
        glyph = GlyphChecked;
        if (glyph < 0)
            glyph = GlyphDown;
    } else if (Pressed) {
        glyph = GlyphDown;
    } else if (Over && GlyphOver >= 0) {
        glyph = GlyphOver;
    } else if (Highlight && GlyphChecked >= 0) {
        glyph = GlyphChecked;
    } else {
        glyph = GlyphNormal;
    }
    PzSetSpriteGlyph(SpriteFrame, Font, glyph);                    // board +0x24
    int w = 0, h = 0;
    Board->GetFrameSize(SpriteFrame, &w, &h);                      // board +0x20
    Resize(w, h);                                                  // vtbl +0x10
}

// PANZERS 0x537b30
void SButton::OnMouseDown(int button, int x, int y, int shift)
{
    SDXWidget::OnMouseDown(button, x, y, shift);                   // 0x539b10
    if (button == 1) {
        HwSound("menu/button_down.wav", -12.0f);                   // 0xc1400000
        Pressed = true;
        CaptureMouse();                                            // 0x543300
        Redraw();
        SendAction(0x42541, 0);
    } else if (button == 3) {
        SendAction(0x42544, 0);
    }
}

// PANZERS 0x537c40
void SButton::OnMouseUp(int button, int x, int y, int shift)
{
    (void)shift;
    if (!Pressed || button != 1)
        return;
    Over = x >= 0 && x < Width && y >= 0 && y < Height;
    Pressed = false;
    ReleaseMouse();                                                // 0x5437c0
    Redraw();
    if (Over)
        SendAction(0x42542, 0);
}

// PANZERS 0x537bf0
void SButton::OnMouseOver()
{
    HwSound("menu/button_over.wav", -30.0f);                       // 0xc1f00000
    Over = true;
    Redraw();
    SendAction(0x42543, 0);
}

// PANZERS 0x537bc0
void SButton::OnMouseOut()
{
    Over = false;
    Redraw();
    SDXWidget::OnMouseOut();                                       // 0x539b30
    SendAction(0x42545, 0);
}

// PANZERS 0x537cf0
void SButton::SetOver(bool over)
{
    if (Over != over) {
        Over = over;
        Redraw();
    }
}

// PANZERS 0x537d40
void SButton::SetHighlight(bool highlight)
{
    if (Highlight != highlight) {
        Highlight = highlight;
        Redraw();
    }
}

// PANZERS 0x537df0
void SButton::SetChecked(bool checked)
{
    if (Checked != checked) {
        Checked = checked;
        Redraw();
    }
}

// ===========================================================================
// STextBox
// ===========================================================================

// PANZERS 0x541910 (over SGenericListBox<STextBoxLine> 0x541860)
STextBox::STextBox()
{
    ToolTipFeatureEnabled = false;
    LineHeight = 0;
    VisibleLines = 0;
    TopIndex = 0;
    ColumnWidth = 0;
    Columns = 1;
    Selectable = false;
    CurSel = -1;
    Hover = -1;
    LastClickTime = 0.0f;
    HasScrollbar = false;
    Lines = nullptr;
    LineCount = 0;
    LineMax = 0;
    Font = -1;
    MaxLines = 10000;                                              // [0x49]
    TextFrames = nullptr;
    Align = 0;
    Center = false;
    _134 = -1;                                                     // [0x4d]
    _138 = 0;                                                      // [0x4e]
    _13c = true;                                                   // [0x4f]
}

// PANZERS 0x541960
void STextBox::FreeLines()
{
    for (int i = 0; i < LineCount; ++i) {
        delete[] Lines[i].Text.buf;
        Lines[i].Text.buf = nullptr;
    }
    free(Lines);
    Lines = nullptr;
    LineMax = 0;
    LineCount = 0;
}

// PANZERS 0x5419d0
STextBox::~STextBox()
{
    Clear();
    if (BackFrame >= 0 && TextFrames) {
        delete[] TextFrames;
        TextFrames = nullptr;
    }
    FreeLines();
}

// PANZERS 0x53e860 (the STextBox reset that SMessageBox::SetText 0x53e910 inlines)
void STextBox::Clear()
{
    CurSel = -1;                                                   // [0x1c] = -1
    for (int i = 0; i < LineCount; ++i) {                          // 0x53e300(0)
        delete[] Lines[i].Text.buf;
        Lines[i].Text.buf = nullptr;
    }
    LineCount = 0;
    if (Lines)
        memset(Lines, 0, LineMax * sizeof(STextBoxLine));
    TopIndex = 0;                                                  // [0x18] = 0
    Update();                                                      // vtbl +0x78
}

// PANZERS 0x5422e0
void STextBox::GenericCreate(int lineHeight, int lines, int columnWidth, int columns,
                             bool selectable, bool background, bool scrollbars)
{
    ColumnWidth = columnWidth;
    LineHeight = lineHeight;
    Columns = columns;
    Selectable = selectable;
    HasScrollbar = scrollbars;
    VisibleLines = lines;
    Resize(Width, lineHeight * lines + 0xc);                       // vtbl +0x10
    if (BackColor != 0)
        Logger.g->Panic("SGenericListBox<T>::Create: BackColor != 0");
    SDXWidget::Create(0);                                          // 0x539a10
    if (background)
        PzDrawFrameBox(this);                                      // 0x543d90
    if (HasScrollbar) {
        InsertChild(&Scrollbar);                                   // vtbl +0x54
        Scrollbar.Create(Width - 0x1b, 4, Height - 8, VisibleLines); // 0x540770
    }
}

// PANZERS 0x542380
void STextBox::Create(int font, int lines, bool background, bool scrollbars,
                      int align, bool center)
{
    Align = align;                                                 // [0x4b]
    Font = font;                                                   // [0x48]
    Center = center;                                               // [0x4c]
    GenericCreate(PzGetFontHeight(HwFont(font)), lines, 0, 1, false, background, scrollbars);
    TextFrames = new int[VisibleLines];                            // [0x4a]
    for (int i = 0; i < VisibleLines; ++i) {
        int x = Align == 2 ? Width / 2 : 6;
        TextFrames[i] = Board->CreateFrame(FT_FIXTEXT, BackFrame, x, LineHeight * i + 6, 0, 1);
        int sb = HasScrollbar ? 0x16 : 0;
        Board->ResizeFrame(TextFrames[i], (Width - sb) - 0xc, LineHeight);
    }
    Update();
}

// PANZERS 0x541af0 (grow) + the line copy of 0x541b60
int STextBox::PushLine(const char* text, int len, unsigned int color)
{
    if (LineCount == LineMax) {
        int n = LineMax < 0x10 ? 0x10 : LineMax * 6 / 5;
        Lines = (STextBoxLine*)realloc(Lines, n * sizeof(STextBoxLine));
        memset(Lines + LineMax, 0, (n - LineMax) * sizeof(STextBoxLine));
        LineMax = n;
    }
    int i = LineCount++;
    char* s = new char[len + 1];
    memcpy(s, text, len);
    s[len] = 0;
    Lines[i].Text.buf = s;
    Lines[i].Text.size = len;
    Lines[i].Color = color;
    return i;
}

// PANZERS 0x541b60
// Adds text as one or more lines: the whole text when it fits and has no
// newline, else it is broken at spaces / newlines into lines that fit the
// box width (a word longer than the width gets a line of its own). Lines
// over MaxLines are dropped from the top. HD's other branch (0x64dd70 !=
// 0, board +0x90 per-character breaking for the Asian code pages) is not
// taken with the shipped languages.
void STextBox::AddLine(const char* text, unsigned int color)
{
    const char* s = text ? text : "";
    int len = (int)strlen(s);
    int width = Width - (HasScrollbar ? 0x16 : 0) - 0xc;
    int font = HwFont(Font);
    int w = 0, h = 0;
    Board->GetTextExtent(font, s, len, &w, &h, 1.0f);              // board +0x88
    if (w < width && !strchr(s, '\n')) {
        PushLine(s, len, color);
    } else {
        int start = 0;
        while (start < len) {
            int lineEnd = start;    // end of the longest fitting prefix
            int next = start;       // where the next line starts
            int pos = start;
            bool fitted = false;
            for (;;) {
                const char* sp = strchr(s + pos, ' ');
                const char* nl = strchr(s + pos, '\n');
                const char* brk = nullptr;
                bool isNl = false;
                if (nl && (!sp || nl < sp)) { brk = nl; isNl = true; }
                else brk = sp;
                int end = brk ? (int)(brk - s) : len;
                if (end > len) end = len;
                Board->GetTextExtent(font, s + start, end - start, &w, &h, 1.0f);
                if (w < width || !fitted) {
                    lineEnd = end;
                    next = end + 1;
                    fitted = true;
                    if (w >= width) break;          // first word too long: it gets its own line
                } else {
                    break;
                }
                if (isNl || !brk || end >= len)
                    break;
                pos = end + 1;
            }
            PushLine(s + start, lineEnd - start, color);
            start = next;
        }
        if (len == 0)
            PushLine("", 0, color);
    }
    while (MaxLines < LineCount) {
        delete[] Lines[0].Text.buf;
        --LineCount;
        memmove(Lines, Lines + 1, LineCount * sizeof(STextBoxLine));
        memset(Lines + LineCount, 0, sizeof(STextBoxLine));
    }
    Update();
}

// PANZERS 0x542990
int STextBox::SetTopIndex(int index)
{
    if (index >= 0 && index < (LineCount - 1 + Columns) / Columns) {
        int old = TopIndex;
        TopIndex = index;
        Update();
        return old;
    }
    return -1;
}

// PANZERS 0x542a00
void STextBox::Update()
{
    if (BackFrame < 0)
        return;
    int offset = 0;
    if (Center && LineCount < VisibleLines)
        offset = ((VisibleLines - LineCount) * LineHeight) / 2;
    for (int i = 0; i < VisibleLines; ++i) {
        int idx = TopIndex + i;
        if (idx < LineCount) {
            Board->SetText(TextFrames[i], HwFont(Font), Align, Lines[idx].Text.buf ? Lines[idx].Text.buf : "");
            Board->SetTextColor(TextFrames[i], Lines[idx].Color);
        } else {
            Board->SetText(TextFrames[i], HwFont(Font), Align, "");
        }
        int x = Align == 2 ? Width / 2 : 6;
        Board->MoveFrame(TextFrames[i], x, LineHeight * i + offset + 6);
    }
    if (HasScrollbar)
        Scrollbar.SetRange((LineCount - 1 + Columns) / Columns, TopIndex);  // 0x540c80
}

// PANZERS 0x5425f0
bool STextBox::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_UP) {
        if (!Selectable && TopIndex > 0) {
            SetTopIndex(TopIndex - 1);
            SendAction(0x4c422, 0);
        }
        return true;
    }
    if (key != VK_DOWN)
        return false;
    if (!Selectable && VisibleLines + TopIndex < (LineCount - 1 + Columns) / Columns) {
        SetTopIndex(TopIndex + 1);
        SendAction(0x4c422, 0);
    }
    return true;
}

// PANZERS 0x5426c0 (not selectable in any M3 box: falls through to the base)
void STextBox::OnMouseDown(int button, int x, int y, int shift)
{
    if (!Selectable)
        return;
    SDXWidget::OnMouseDown(button, x, y, shift);
}

// PANZERS 0x542840
void STextBox::OnMouseMove(int x, int y, int shift)
{
    if (!Selectable)
        return;
    Hover = (TopIndex + (y - 6) / LineHeight) * Columns;
    Update();
    SDXWidget::OnMouseMove(x, y, shift);
}

// PANZERS 0x5428b0
void STextBox::OnMouseOut()
{
    if (!Selectable)
        return;
    Hover = -1;
    Update();
    SDXWidget::OnMouseOut();
}

// PANZERS 0x5428d0
void STextBox::OnMouseWheel(int delta, int x, int y, int keys)
{
    (void)x; (void)y; (void)keys;
    int rows = (LineCount - 1 + Columns) / Columns;
    if (delta < 1) {
        if (VisibleLines + TopIndex < rows) {
            int t = TopIndex + 1;
            if (t >= 0 && t < rows) {
                TopIndex = t;
                Update();
            }
            SendAction(0x4c422, 0);
        }
    } else if (TopIndex > 0) {
        int t = TopIndex - 1;
        if (t >= 0 && t < rows) {
            TopIndex = t;
            Update();
        }
        SendAction(0x4c422, 0);
    }
}

// PANZERS 0x542540
bool STextBox::OnAction(SWidget* source, int action, int param)
{
    (void)source;
    int rows = (LineCount - 1 + Columns) / Columns;
    if (action == PZA_SCROLL_UP) {
        if (TopIndex < 1)
            return true;
        int t = TopIndex - param;
        if (t < 0)
            t = 0;
        if (t < rows) {
            TopIndex = t;
            Update();
        }
    } else if (action == PZA_SCROLL_DOWN) {
        if (rows <= VisibleLines + TopIndex)
            return true;
        int t = TopIndex + param;
        if (t > rows - VisibleLines)
            t = rows - VisibleLines;
        SetTopIndex(t);
    } else if (action == PZA_SCROLL_TO) {
        SetTopIndex(param);
    } else {
        return false;
    }
    SendAction(0x4c422, 0);
    return true;
}

// ===========================================================================
// SMessageBox
// ===========================================================================

// PANZERS 0x53e0d0
SMessageBox::SMessageBox()
{
    ToolTipFeatureEnabled = false;
    Type = 0;
    Modal = false;                                                 // [0xf8] = 0
    Target = nullptr;                                              // [0xf9] = 0
    TitleFrame = -1;
}

// PANZERS 0x53e190
SMessageBox::~SMessageBox()
{
    if (Modal && Visible) {
        if (SWindow* w = GetWindowParent())                        // 0x5435b0
            w->UnsetModalWidget(this);                             // 0x5450e0
    }
}

// PANZERS 0x53e3b0
void SMessageBox::Create(const char* title, const char* text, unsigned int type, bool modal)
{
    (void)text;    // HD ignores param 2 here; the text comes from SetText 0x53e910
    Type = type & 0xffff;                                          // [0xf7]
    Modal = modal;                                                 // [0xf8]
    SetBackgroundSprite(Type == PZ_MB_NOBUTTONS ? g_MenuMessage2Tex : g_MenuMessageTex, 0, false, false); // 0x539ba0
    SDXWidget::Create(0);                                          // 0x539a10
    SetPosition((0x400 - Width) / 2, (0x300 - Height) / 2, Width, Height); // vtbl +0x08
    Cursor = 0;                                                    // 0x543970(0, -1)
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);
    TitleFrame = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(TitleFrame, g_PzFont[PZF_SANS21], 2, title ? title : "MessageBox");
    Board->SetTextColor(TitleFrame, 0xffffff);
    InsertChild(&Text);                                            // vtbl +0x54
    Text.SetPosition(0x14, 0x32, Width - 0x28, 0);
    int font, lines;
    unsigned int align;
    if ((type & 0x10000) == 0) {
        lines = 7;
        font = (int)((~(type >> 0x12) & 1) | 2);
        align = ~(type >> 0x10) & 2;
    } else {
        lines = 10;
        font = 0;
        align = ~(type >> 0x10) & 2;
    }
    Text.Create(font, lines, ((type >> 0x12) & 1) != 0, false, (int)align, (~(type >> 0x11) & 1) != 0);
    const char* last = nullptr;
    SComplexButton* lastButton = nullptr;
    if (Type == PZ_MB_OK) {
        InsertChild(&OkButton);
        OkButton.SetPosition((Width - 0xc4) / 2, 0xe6, 0, 0);
        last = "OK";
        lastButton = &OkButton;
    } else if (Type == PZ_MB_YESNO) {
        InsertChild(&YesButton);
        YesButton.SetPosition(Width / 2 - 0xc4, 0xe6, 0, 0);
        YesButton.Create(1, GetText("window/MessageBox.cpp", "Yes"));
        InsertChild(&NoButton);
        NoButton.SetPosition(Width / 2, 0xe6, 0, 0);
        last = "No";
        lastButton = &NoButton;
    } else if (Type == PZ_MB_RETRYIGNORE) {
        InsertChild(&RetryButton);
        RetryButton.SetPosition(Width / 2 - 0xc4, 0xe6, 0, 0);
        RetryButton.Create(1, GetText("window/MessageBox.cpp", "Retry"));
        InsertChild(&IgnoreButton);
        IgnoreButton.SetPosition(Width / 2, 0xe6, 0, 0);
        last = "Ignore";
        lastButton = &IgnoreButton;
    } else if (Type != PZ_MB_NOBUTTONS) {
        Logger.g->Panic("SMessageBox::Create: Unsupported messagebox type");
    }
    if (lastButton)
        lastButton->Create(1, GetText("window/MessageBox.cpp", last));
    SDXWidget::SetVisible(false);                                  // 0x539c40(0)
    SetFocus();                                                    // 0x5439f0
}

// PANZERS 0x53e8e0
void SMessageBox::SetTitle(const char* title)
{
    Board->SetText(TitleFrame, g_PzFont[PZF_SANS21], 2, title ? title : "");
}

// PANZERS 0x53e910
void SMessageBox::SetText(const char* text, unsigned int color)
{
    Text.Clear();                                                  // [0x32]=-1, 0x53e300(0), [0x2e]=0, +0x78
    Text.AddLine(text, color);                                     // 0x541b60
}

// PANZERS 0x53e890
// The result goes to the target when one is set, else to this box; then up
// the parents until an OnAction takes it.
void SMessageBox::SendResult(int action)
{
    SWidget* w = Target ? Target : this;
    while (w && !w->OnAction(this, action, 0))
        w = w->Parent;
}

// PANZERS 0x53e6e0
bool SMessageBox::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action != 0x42542)                                         // SComplexButton click
        return false;
    int result = 0;
    if (source == &OkButton) result = PZA_MSGBOX_OK;
    else if (source == &YesButton) result = PZA_MSGBOX_YES;
    else if (source == &NoButton) result = PZA_MSGBOX_NO;
    else if (source == &RetryButton) result = PZA_MSGBOX_RETRY;
    else if (source == &IgnoreButton) result = PZA_MSGBOX_IGNORE;
    else return true;
    SetVisible(false);                                             // vtbl +0x6c
    SendResult(result);
    return true;
}

// PANZERS 0x53e7d0
bool SMessageBox::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_RETURN) {
        if (Type == PZ_MB_OK) {
            SetVisible(false);
            SendResult(PZA_MSGBOX_OK);
        } else if (Type == PZ_MB_YESNO) {
            SetVisible(false);
            SendResult(PZA_MSGBOX_YES);
        }
    } else if (key == VK_ESCAPE) {
        if (Type == PZ_MB_OK) {
            SetVisible(false);
            SendResult(PZA_MSGBOX_OK);
        } else if (Type == PZ_MB_YESNO) {
            SetVisible(false);
            SendResult(PZA_MSGBOX_NO);
        }
    }
    return true;
}

// PANZERS 0x53e9a0
void SMessageBox::SetVisible(bool visible)
{
    if (visible == Visible)
        return;
    SDXWidget::SetVisible(visible);                                // 0x539c40
    if (!Modal)
        return;
    SWindow* w = GetWindowParent();                                // 0x5435b0
    if (Visible) {
        SetFocus();                                                // 0x5439f0
        if (w)
            w->SetModalWidget(this);                               // 0x544fe0
    } else if (w) {
        w->UnsetModalWidget(this);                                 // 0x5450e0
    }
}

} // namespace pz
