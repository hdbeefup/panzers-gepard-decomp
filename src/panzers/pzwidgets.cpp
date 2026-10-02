// src/panzers/pzwidgets.cpp
// Panzers option widgets (HD PANZERS.exe); see pzwidgets.h.
//
// HD board slots used here and their SWINE SIBoard equivalents (as in
// mainmenu.cpp): +0x08 CreateFrame (type 1 sprite, 2 text, 3 fixed-size
// text, 4 box), +0x0c DestroyFrame, +0x10 MoveFrame, +0x14 ResizeFrame,
// +0x18 ShowFrame, +0x20 GetFrameSize, +0x24 SetSpriteGlyph, +0x28
// SetTextColor, +0x34 SetText(frame, font, align, text, 0), +0x3c
// SetBoxColor, +0x70 LoadFixedFont, +0x7c LoadTexture (PzLoadTexture),
// +0x80 ReleaseFont/ReleaseTexture. HD font indices go through g_PzFont.

#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pzwidgets.h"
#include "pzboard.h"
#include "board.h"
#include "iconcert.h"
#include "logger.h"
#include "timer.h"

static int PzFont(int hdFont)
{
    return (hdFont >= 0 && hdFont < 6) ? g_PzFont[hdFont] : hdFont;
}

static void PlayMenuSound(const char* name, float db)
{
    if (Concert)
        Concert->PlaySound(name, db, 0, -1);                      // concert +0x50
}

static bool Inside(const SWidget* w, int x, int y)
{
    return x >= 0 && x < w->Width && y >= 0 && y < w->Height;
}

// PANZERS 0x6c4fb0
void PzBringFrameToFront(int frame)
{
    SBoard* b = static_cast<SBoard*>(Board);
    SHeap<SFrame>::Element* a = b->Frames.array;
    while (frame >= 0 && frame < b->Frames.size && a[frame].use == 0x7FFFFFFF) {
        int parent = a[frame].data.Parent;
        if (parent < 0)
            return;
        if (a[frame].data.Sibling >= 0) {
            // unlink
            if (a[parent].data.Child == frame) {
                a[parent].data.Child = a[frame].data.Sibling;
            } else {
                int prev = a[parent].data.Child;
                while (prev >= 0 && a[prev].data.Sibling != frame)
                    prev = a[prev].data.Sibling;
                if (prev < 0)
                    Logger.g->Panic("SHeap<SFrame>::operator[]: invalid index (%d)", prev);
                a[prev].data.Sibling = a[frame].data.Sibling;
            }
            // append as the last child
            int last = a[parent].data.Child;
            while (a[last].data.Sibling >= 0)
                last = a[last].data.Sibling;
            a[last].data.Sibling = frame;
            a[frame].data.Sibling = -1;
        }
        frame = parent;
    }
}

// PANZERS 0x6c5260
// HD returns SFontProp +0x20, the line height of a .font file. The SWINE
// board keeps that value in SFontProp::fontSize (see LoadFontFileFont).
int PzGetFontHeight(int font)
{
    SBoard* b = static_cast<SBoard*>(Board);
    if (font < 0 || font >= b->Fonts.size || b->Fonts.array[font].use != 0x7FFFFFFF)
        Logger.g->Panic("SBoard::GetFontHeight: Invalid font index");
    return b->Fonts.array[font].data.fontSize;
}

// PANZERS 0x543d90
void PzDrawFrameBox(SWidget* w)
{
    int x, y, width, height;
    w->GetPosition(&x, &y, &width, &height);
    int parent = w->GetFrame();
    struct Piece { int x, y, glyph, rw, rh; } pieces[9] = {
        { 0,          0,           0x2f, 0,          0 },
        { 0xe,        0,           0x30, width - 0x1c, 0xe },
        { width - 0xe, 0,          0x31, 0,          0 },
        { 0,          0xe,         0x32, 0xe,        height - 0x1c },
        { 0xe,        0xe,         0x33, width - 0x1c, height - 0x1c },
        { width - 0xe, 0xe,        0x34, 0xe,        height - 0x1c },
        { 0,          height - 0xe, 0x35, 0,          0 },
        { 0xe,        height - 0xe, 0x36, width - 0x1c, 0xe },
        { width - 0xe, height - 0xe, 0x37, 0,         0 },
    };
    for (const Piece& p : pieces) {
        int f = Board->CreateFrame(FT_SPRITE, parent, p.x, p.y, 0, 1);
        Board->SetSpriteGlyph(f, g_MenuControlsFont, p.glyph);
        if (p.rw)
            Board->ResizeFrame(f, p.rw, p.rh);
    }
}

namespace pz {

// ===========================================================================
// SSliderH
// ===========================================================================

// PANZERS 0x540ef0
SSliderH::SSliderH()
{
    ToolTipFeatureEnabled = false;
    SliderFont = -1;
    ButtonFont = -1;
    SliderFrame = SliderBox = ButtonLeftFrame = ButtonRightFrame = -1;
    FirstClickTimer = -1;
    TickTimer = -1;
    NumberOfFixPos = 0;
    SliderPos = 0;
    Ticks = nullptr;
    PressedLeft = PressedRight = ActiveLeft = ActiveRight = false;
    KnobDown = false;
    KnobX = 0;
    KnobDelta = 0;
    KnobDownX = 0;
}

// PANZERS 0x540f70
SSliderH::~SSliderH()
{
    if (BackFrame >= 0) {
        PzReleaseTexture(SliderFont);                              // board +0x80
        Board->ReleaseFont(ButtonFont);
    }
    KillTimer(&FirstClickTimer);
    KillTimer(&TickTimer);
    if (Ticks) {
        free(Ticks);
        Ticks = nullptr;
    }
}

// PANZERS 0x541040
void SSliderH::Create(int x, int y, int w, int numberOfFixPos, int pos)
{
    SetPosition(x, y, w, 0x16);                                    // vtbl +0x08
    SDXWidget::Create(0);                                          // 0x539a10
    NumberOfFixPos = numberOfFixPos;
    SliderPos = pos;
    ButtonLeftFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 2, 0, 0, 1);
    ButtonRightFrame = Board->CreateFrame(FT_SPRITE, BackFrame, Width - 0x16, 0, 0, 1);
    ButtonFont = Board->LoadFixedFont("menu/widgets/arrows_medium_hq.tga", 0x15, 0x16, 6, 0xc, nullptr, Default);
    SliderBox = Board->CreateFrame(FT_BOX, BackFrame, 0x12, 10, 0, 0);
    Board->SetBoxColor(SliderBox, 0xff848484);
    Board->ResizeFrame(SliderBox, Width - 0x25, 2);
    Ticks = (int*)malloc(sizeof(int) * (NumberOfFixPos + 1));
    for (int i = 0; i < NumberOfFixPos + 1; ++i) {
        // _DAT_007f35dc = 22.0
        Ticks[i] = Board->CreateFrame(FT_BOX, BackFrame,
            (int)(((float)(Width - 0x2e) / (float)NumberOfFixPos) * (float)i + 22.0f), 9, 0, 0);
        Board->SetBoxColor(Ticks[i], 0xff848484);
        Board->ResizeFrame(Ticks[i], 2, 4);
    }
    SliderFont = PzLoadTexture("menu/widgets/slider_button_hq.tga"); // board +0x7c
    SliderFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0x32, Height / 2 - 6, 0, 1);
    Board->SetSpriteGlyph(SliderFrame, SliderFont, 0);
    Update();
}

// PANZERS 0x5416b0
void SSliderH::SetValue(int pos)
{
    SliderPos = pos;
    Update();
}

// Position under x (0x5412a0/0x5414f0): DAT_007ea760 = 0.5 rounding.
static int SliderPosFromX(const SSliderH* s, int x)
{
    int pos = (int)((double)((float)(x - 0x18) / ((float)(s->Width - 0x2e) / (float)s->NumberOfFixPos)) + 0.5);
    if (pos < 0)
        pos = 0;
    if (pos > s->NumberOfFixPos)
        pos = s->NumberOfFixPos;
    return pos;
}

// PANZERS 0x5412a0
void SSliderH::OnMouseDown(int button, int x, int y, int shift)
{
    (void)shift;
    if (button != 1)
        return;
    PressedLeft = (unsigned)x < 0x14 && y >= 0 && y < Height;
    PressedRight = x >= Width - 0x14 && x < Width && y >= 0 && y < Height;
    if (!PressedLeft && !PressedRight) {
        SliderPos = SliderPosFromX(this, x);
        Update();
        SendAction(PZA_SLIDER_CHANGED, SliderPos);
    }
    if (!PressedLeft && !PressedRight)
        KnobDown = y >= Height / 2 - 5 && y < Height / 2 + 5 && x >= KnobX && x < KnobX + 0xc;
    else
        KnobDown = false;
    if (KnobDown) {
        KnobDownX = x;
        KnobDelta = 0;
    }
    CaptureMouse();                                                // 0x543300
    Update();
    if (PressedLeft || PressedRight) {
        PlayMenuSound("menu/button_down.wav", -12.0f);
        FirstClickTimer = SetTimer(200);                           // 0x543b10
    }
    if (PressedLeft)
        SendAction(PZA_SLIDER_LEFT, 0);
    if (PressedRight)
        SendAction(PZA_SLIDER_RIGHT, 0);
}

// PANZERS 0x5414f0
void SSliderH::OnMouseUp(int button, int x, int y, int shift)
{
    (void)shift;
    if (button == 1) {
        KnobDelta = 0;
        ReleaseMouse();                                            // 0x5437c0
    }
    if (!PressedLeft) {
        if (!PressedRight && button == 1 && KnobDown) {
            KnobDown = false;
            SliderPos = SliderPosFromX(this, x);
            Update();
            SendAction(PZA_SLIDER_CHANGED, SliderPos);
        }
        if (!PressedLeft && !PressedRight)
            return;
    }
    if (button == 1) {
        KillTimer(&TickTimer);                                     // 0x543690
        ActiveLeft = (unsigned)x < 0x14 && y >= 0 && y < Height;
        ActiveRight = x >= Width - 0x14 && x < Width && y >= 0 && y < Height;
        PressedLeft = PressedRight = false;
        Update();
    }
}

// PANZERS 0x541460
void SSliderH::OnMouseMove(int x, int y, int shift)
{
    (void)shift;
    ActiveLeft = (unsigned)x < 0x14 && y >= 0 && y < Height;
    ActiveRight = x >= Width - 0x14 && x < Width && y >= 0 && y < Height;
    if (KnobDown)
        KnobDelta = x - KnobDownX;
    Update();
}

// PANZERS 0x5414e0
void SSliderH::OnMouseOver()
{
}

// PANZERS 0x5414d0
void SSliderH::OnMouseOut()
{
    ActiveLeft = ActiveRight = false;
    Update();
}

// PANZERS 0x541240
bool SSliderH::OnAction(SWidget* source, int action, int param)
{
    (void)source; (void)param;
    if (action == PZA_SLIDER_LEFT)
        --SliderPos;
    else if (action == PZA_SLIDER_RIGHT)
        ++SliderPos;
    else
        return false;
    if (SliderPos < 0)
        SliderPos = 0;
    if (SliderPos > NumberOfFixPos)
        SliderPos = NumberOfFixPos;
    Update();
    SendAction(PZA_SLIDER_CHANGED, SliderPos);
    return true;
}

// PANZERS 0x541620
void SSliderH::OnTimer(int id, unsigned int elapsed)
{
    (void)elapsed;
    if (id == FirstClickTimer) {
        if (PressedLeft || PressedRight)
            TickTimer = SetTimer(0x46);
        KillTimer(&FirstClickTimer);
        return;
    }
    if (id == TickTimer) {
        if (PressedLeft) {
            SendAction(PZA_SLIDER_LEFT, 0);
            return;
        }
        if (PressedRight) {
            SendAction(PZA_SLIDER_RIGHT, 0);
            return;
        }
        KillTimer(&TickTimer);
    }
}

// PANZERS 0x5416d0
void SSliderH::Update()
{
    if (BackFrame < 0 || !Enabled)
        return;
    // DAT_007f35d8 = 20.0
    int x = (int)(((float)(Width - 0x2e) / (float)NumberOfFixPos) * (float)SliderPos + 20.0f);
    KnobX = x;
    if (KnobDown)
        KnobX = KnobDelta + x;
    if (KnobX < 0x14)
        KnobX = 0x14;
    if (KnobX > Width - 0x1a)
        KnobX = Width - 0x1a;
    Board->MoveFrame(SliderFrame, KnobX - 2, Height / 2 - 6);
    Board->ShowFrame(ButtonLeftFrame, true);
    Board->ShowFrame(ButtonRightFrame, true);
    Board->ShowFrame(SliderBox, true);
    Board->ShowFrame(SliderFrame, true);
    Board->SetSpriteGlyph(ButtonLeftFrame, ButtonFont, 0);
    Board->SetSpriteGlyph(ButtonRightFrame, ButtonFont, 3);
    if (PressedLeft)
        Board->SetSpriteGlyph(ButtonLeftFrame, ButtonFont, 2);
    else if (PressedRight)
        Board->SetSpriteGlyph(ButtonRightFrame, ButtonFont, 5);
    else if (ActiveLeft)
        Board->SetSpriteGlyph(ButtonLeftFrame, ButtonFont, 1);
    else if (ActiveRight)
        Board->SetSpriteGlyph(ButtonRightFrame, ButtonFont, 4);
}

// ===========================================================================
// SCheckBox
// ===========================================================================

// PANZERS 0x537e20
SCheckBox::SCheckBox()
{
    ToolTipFeatureEnabled = false;
    TextFrame = -1;
    BoxFrame = -1;
    _68 = -1;
    // HD also names the widget "SCheckBox" (SString at SWidget level).
    Over = false;
    Pressed = false;
    Checked = false;
}

// PANZERS 0x537ea0
SCheckBox::~SCheckBox()
{
    if (Text.buf) {
        delete[] Text.buf;
        Text.buf = nullptr;
    }
}

// PANZERS 0x537f20
void SCheckBox::Create()
{
    SDXWidget::Create(0);
    BoxFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 0);
    TextFrame = Board->CreateFrame(FT_TEXT, BackFrame, 0x19, 1, 0, 1);
    Update();
}

// PANZERS 0x538110
void SCheckBox::SetText(const char* text)
{
    Text = text ? text : "";
    Update();
}

// PANZERS 0x5380f0
void SCheckBox::SetCheck(bool checked)
{
    Checked = checked;
    Update();
}

// PANZERS 0x537f90
void SCheckBox::OnMouseDown(int button, int x, int y, int shift)
{
    if (button == 1) {
        Pressed = true;
        CaptureMouse();
        Update();
        SendAction(PZA_CHECKBOX_DOWN, 0);
    }
    SDXWidget::OnMouseDown(button, x, y, shift);                   // 0x539b10
}

// PANZERS 0x538050
void SCheckBox::OnMouseUp(int button, int x, int y, int shift)
{
    (void)shift;
    if (!Pressed || button != 1)
        return;
    Over = Inside(this, x, y);
    Pressed = false;
    ReleaseMouse();
    Update();
    if (Over) {
        PlayMenuSound("menu/radiobutton.wav", -12.0f);
        Checked = !Checked;
        Update();
        SendAction(PZA_CHECKBOX_CLICK, 0);
    }
}

// PANZERS 0x538000
void SCheckBox::OnMouseOver()
{
    PlayMenuSound("menu/button_over.wav", -30.0f);
    Over = true;
    Update();
    SendAction(PZA_CHECKBOX_OVER, 0);
}

// PANZERS 0x537fe0
void SCheckBox::OnMouseOut()
{
    Over = false;
    Update();
    SDXWidget::OnMouseOut();                                       // 0x539b30
}

// PANZERS 0x538130
void SCheckBox::Update()
{
    if (BackFrame < 0)
        return;
    const char* t = Text.buf ? Text.buf : "";
    if (!Enabled) {
        Board->SetTextColor(TextFrame, 0x666666);
        Board->SetText(TextFrame, PzFont(3), 0, t);
        Board->SetSpriteGlyph(BoxFrame, g_MenuControlsFont, Checked ? 0x18 : 0x17);
    } else {
        Board->SetSpriteGlyph(BoxFrame, g_MenuControlsFont, Checked ? 0x16 : 0x15);
        Board->SetTextColor(TextFrame, (Pressed || Over) ? 0xffffff : 0xd0d0d0);
        Board->SetText(TextFrame, PzFont(3), 0, t);
    }
    int w = 0, h = 0;
    Board->GetFrameSize(TextFrame, &w, &h);                        // board +0x20
    Resize(w + 0x14, h);                                           // vtbl +0x10
}

// ===========================================================================
// SDropList
// ===========================================================================

// PANZERS 0x538bb0
SDropList::SDropList()
{
    ToolTipFeatureEnabled = false;
    Items = nullptr;
    ItemCount = 0;
    ItemMax = 0;
    Font = 0;
    ItemHeight = 0;
    CurSel = -1;
    HoverSel = -1;
    Over = false;
    Dropped = false;
    Dropable = true;
    MainFrame = TextFrame = -1;
    ItemFrames = nullptr;
    ItemTextFrames = nullptr;
    BottomFrame = -1;
}

// PANZERS 0x538c20
void SDropList::FreeItems()
{
    for (int i = 0; i < ItemCount; ++i)
        if (Items[i].Text.buf) {
            delete[] Items[i].Text.buf;
            Items[i].Text.buf = nullptr;
        }
    free(Items);
    Items = nullptr;
    ItemMax = 0;
    ItemCount = 0;
}

// PANZERS 0x538c90
SDropList::~SDropList()
{
    if (Dropped)
        ToggleDrop();
    ResetContent();
    FreeItems();
}

// PANZERS 0x538f20
void SDropList::Create()
{
    Font = 2;
    ItemHeight = 0x18;
    Resize(0xaf, 0x22);                                            // vtbl +0x10
    SDXWidget::Create(0);
    MainFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);
    TextFrame = Board->CreateFrame(FT_FIXTEXT, MainFrame, 8, 5, 0, 1);
    Board->ResizeFrame(TextFrame, 0x8c, Height);
    Update();
}

// PANZERS 0x538dd0
int SDropList::AddItem(const char* text, unsigned int data)
{
    if (Dropped)
        ToggleDrop();
    // 0x538d60: SDArray grow (16, then x6/5)
    if (ItemCount == ItemMax) {
        int n = ItemMax < 0x10 ? 0x10 : ItemMax * 6 / 5;
        Items = (SDropListItem*)realloc(Items, n * sizeof(SDropListItem));
        memset(Items + ItemMax, 0, (n - ItemMax) * sizeof(SDropListItem));
        ItemMax = n;
    }
    int i = ItemCount++;
    Items[i].Text = text ? text : "";
    Items[i].Data = data;
    Update();
    return i;
}

// PANZERS 0x5396a0
void SDropList::ResetContent()
{
    if (Dropped)
        ToggleDrop();
    if (ItemCount >= 0 && CurSel != -1) {
        if (Dropped)
            ToggleDrop();
        CurSel = -1;
        Update();
    }
    // 0x538e70(0): free the texts and empty the array
    for (int i = 0; i < ItemCount; ++i)
        if (Items[i].Text.buf) {
            delete[] Items[i].Text.buf;
            Items[i].Text.buf = nullptr;
        }
    ItemCount = 0;
    if (Items)
        memset(Items, 0, ItemMax * sizeof(SDropListItem));
    Update();
}

// PANZERS 0x5392a0
unsigned int SDropList::GetItemData(int index) const
{
    if (index < 0 || index >= ItemCount)
        Logger.g->Panic("SDropList::GetItemData: Invalid index");
    return Items[index].Data;
}

// PANZERS 0x5396f0
int SDropList::SetCurSel(int index)
{
    if (index < ItemCount && index >= -1 && CurSel != index) {
        if (Dropped)
            ToggleDrop();
        int old = CurSel;
        CurSel = index;
        Update();
        return old;
    }
    return -1;
}

// PANZERS 0x539020
void SDropList::ToggleDrop()
{
    if (Dropped) {
        for (int i = 0; i < ItemCount; ++i)
            Board->DestroyFrame(ItemFrames[i]);                    // board +0x0c
        Board->DestroyFrame(BottomFrame);
        delete[] ItemFrames;
        ItemFrames = nullptr;
        delete[] ItemTextFrames;
        ItemTextFrames = nullptr;
        ReleaseMouse();
        Dropped = false;
        return;
    }
    if (ItemCount == 0)
        return;
    HoverSel = CurSel;
    PzBringFrameToFront(BackFrame);                                // board +0x5c
    ItemFrames = new int[ItemCount];
    ItemTextFrames = new int[ItemCount];
    int i = 0;
    for (; i < ItemCount; ++i) {
        ItemFrames[i] = Board->CreateFrame(FT_SPRITE, BackFrame, 0, ItemHeight * i + Height, 0, 1);
        Board->SetSpriteGlyph(ItemFrames[i], g_MenuControlsFont, 0xe);
        ItemTextFrames[i] = Board->CreateFrame(FT_FIXTEXT, ItemFrames[i], 8, 0, 0, 1);
        Board->ResizeFrame(ItemTextFrames[i], Width - 0x10, ItemHeight);
        Board->SetTextColor(ItemTextFrames[i], 0xd0d0d0);
        Board->SetText(ItemTextFrames[i], PzFont(Font), 0, Items[i].Text.buf ? Items[i].Text.buf : "");
    }
    BottomFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, ItemHeight * i + Height, 0, 1);
    Board->SetSpriteGlyph(BottomFrame, g_MenuControlsFont, 0x10);
    CaptureMouse();
    Dropped = true;
}

// PANZERS 0x539370
void SDropList::OnMouseDown(int button, int x, int y, int shift)
{
    if (!Dropable)
        return;
    if (button == 1) {
        if (!Dropped) {
            PlayMenuSound("menu/button_down.wav", -12.0f);
            ToggleDrop();
        } else {
            if (x >= 0 && x <= Width && y >= Height && y < ItemCount * ItemHeight + Height) {
                SetCurSel((y - Height) / ItemHeight);
                PlayMenuSound("menu/button_down.wav", -12.0f);
                SendAction(PZA_DROPLIST_SELECT, CurSel);
            }
            if (Dropped)
                ToggleDrop();
        }
    }
    Update();
    SDXWidget::OnMouseDown(button, x, y, shift);
}

// PANZERS 0x539550
void SDropList::OnMouseUp(int button, int x, int y, int shift)
{
    (void)shift;
    if (Dropable && Dropped && button == 1 && x >= 0 && x <= Width
        && y >= Height && y < ItemCount * ItemHeight + Height) {
        SetCurSel((y - Height) / ItemHeight);
        PlayMenuSound("menu/button_down.wav", -12.0f);
        SendAction(PZA_DROPLIST_SELECT, CurSel);
        if (Dropped)
            ToggleDrop();
    }
}

// PANZERS 0x539460
void SDropList::OnMouseMove(int x, int y, int shift)
{
    Over = Inside(this, x, y);
    if (Dropable && Dropped && x >= 0 && x <= Width && y >= Height
        && y < ItemCount * ItemHeight + Height) {
        HoverSel = (y - Height) / ItemHeight;
        Update();
    }
    SDXWidget::OnMouseMove(x, y, shift);                           // 0x539b20
}

// PANZERS 0x539500
void SDropList::OnMouseOver()
{
    if (!Dropable)
        return;
    PlayMenuSound("menu/button_over.wav", -30.0f);
    Over = true;
    Update();
}

// PANZERS 0x5394e0
void SDropList::OnMouseOut()
{
    if (Dropable) {
        Over = false;
        Update();
    }
    SDXWidget::OnMouseOut();
}

// PANZERS 0x5397f0
void SDropList::Update()
{
    if (BackFrame < 0)
        return;
    if (Dropped) {
        for (int i = 0; i < ItemCount; ++i)
            Board->SetSpriteGlyph(ItemFrames[i], g_MenuControlsFont, i == HoverSel ? 0xf : 0xe);
        Board->SetSpriteGlyph(MainFrame, g_MenuControlsFont, 0xc);
    } else {
        unsigned int color;
        if (!Dropable) {
            Board->SetSpriteGlyph(MainFrame, g_MenuControlsFont, 0xd);
            color = 0xd0d0d0;
        } else if (!Over) {
            Board->SetSpriteGlyph(MainFrame, g_MenuControlsFont, 10);
            color = 0xd0d0d0;
        } else {
            Board->SetSpriteGlyph(MainFrame, g_MenuControlsFont, 0xb);
            color = 0xffffff;
        }
        Board->SetTextColor(TextFrame, color);
    }
    const char* t = "";
    if (CurSel >= 0 && CurSel < ItemCount && Items[CurSel].Text.buf)
        t = Items[CurSel].Text.buf;
    Board->SetText(TextFrame, PzFont(Font), 0, t);
}

// ===========================================================================
// SScrollbar
// ===========================================================================

// PANZERS 0x540630
SScrollbar::SScrollbar()
{
    ToolTipFeatureEnabled = false;
    KnobTopFrame = KnobMidFrame = KnobBottomFrame = UpFrame = DownFrame = -1;
    FirstClickTimer = -1;
    TickTimer = -1;
    PressedUp = PressedDown = ActiveUp = ActiveDown = false;
    Dragging = false;
    PageSize = 0;
    Pos = 0;
    Range = 0;
    KnobY = 0;
    KnobH = 0;
    DragOffset = 0;
}

// PANZERS 0x540690
SScrollbar::~SScrollbar()
{
    KillTimer(&FirstClickTimer);
    KillTimer(&TickTimer);
}

// PANZERS 0x540770
void SScrollbar::Create(int x, int y, int h, int pageSize)
{
    SetPosition(x, y, 0x1c, h);
    SDXWidget::Create(0);
    PageSize = pageSize;
    UpFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);
    DownFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, Height - 0x16, 0, 1);
    int f = Board->CreateFrame(FT_SPRITE, BackFrame, 1, 0x16, 0, 1);
    Board->SetSpriteGlyph(f, g_MenuControlsFont, 0x29);
    f = Board->CreateFrame(FT_SPRITE, BackFrame, 1, 0x17, 0, 1);
    Board->SetSpriteGlyph(f, g_MenuControlsFont, 0x2a);
    Board->ResizeFrame(f, 0x12, Height - 0x2e);
    f = Board->CreateFrame(FT_SPRITE, BackFrame, 1, Height - 0x17, 0, 1);
    Board->SetSpriteGlyph(f, g_MenuControlsFont, 0x2b);
    KnobTopFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 3, 0x1e, 0, 1);
    Board->SetSpriteGlyph(KnobTopFrame, g_MenuControlsFont, 0x2c);
    KnobMidFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 3, 0x31, 0, 1);
    Board->SetSpriteGlyph(KnobMidFrame, g_MenuControlsFont, 0x2d);
    KnobBottomFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 3, 0x25, 0, 1);
    Board->SetSpriteGlyph(KnobBottomFrame, g_MenuControlsFont, 0x2e);
    Update();
}

// PANZERS 0x540c80
void SScrollbar::SetRange(int range, int pos)
{
    Range = range;
    Pos = pos;
    Update();
}

// PANZERS 0x540910
void SScrollbar::OnMouseDown(int button, int x, int y, int shift)
{
    (void)x; (void)shift;
    if (button != 1)
        return;
    if (y < 0x16) {
        PressedUp = true;
        PressedDown = false;
        Dragging = false;
    } else if (y < Height - 0x16) {
        if (y < KnobY) {
            SendAction(PZA_SCROLL_UP, PageSize);
            return;
        }
        if (y >= KnobH + KnobY) {
            SendAction(PZA_SCROLL_DOWN, PageSize);
            return;
        }
        PressedUp = PressedDown = false;
        Dragging = true;
    } else {
        PressedUp = false;
        PressedDown = true;
        Dragging = false;
    }
    if (Dragging)
        DragOffset = KnobY - y;
    CaptureMouse();
    Update();
    if (PressedUp || PressedDown) {
        PlayMenuSound("menu/button_down.wav", -12.0f);
        FirstClickTimer = SetTimer(200);
    }
    if (PressedUp)
        SendAction(PZA_SCROLL_UP, 1);
    if (PressedDown)
        SendAction(PZA_SCROLL_DOWN, 1);
}

// PANZERS 0x540b60
void SScrollbar::OnMouseUp(int button, int x, int y, int shift)
{
    (void)shift;
    if (button == 1) {
        Dragging = false;
        ReleaseMouse();
        Update();
    }
    if ((PressedUp || PressedDown) && button == 1) {
        KillTimer(&TickTimer);
        ActiveUp = (unsigned)x < 0x16 && (unsigned)y < 0x16;
        ActiveDown = (unsigned)x < 0x16 && y >= Height - 0x16 && y < Height;
        PressedUp = PressedDown = false;
        Update();
    }
}

// PANZERS 0x540a30
void SScrollbar::OnMouseMove(int x, int y, int shift)
{
    (void)shift;
    ActiveUp = (unsigned)x < 0x16 && (unsigned)y < 0x16;
    ActiveDown = (unsigned)x < 0x16 && y >= Height - 0x16 && y < Height;
    if (Dragging) {
        int ny = DragOffset + y;
        if (ny < 0x16)
            ny = 0x16;
        int track = (Height - 0x2c) - KnobH;
        KnobY = ny;
        if (KnobY > track + 0x16)
            KnobY = track + 0x16;
        int pos = (int)lrintf(((float)(Range - PageSize) * (float)(KnobY - 0x16)) / (float)track);
        if (pos < 0)
            pos = 0;
        if (pos > Range - PageSize)
            pos = Range - PageSize;
        if (pos != Pos) {
            SendAction(PZA_SCROLL_TO, pos);
            return;
        }
    }
    Update();
}

// PANZERS 0x540b50
void SScrollbar::OnMouseOver()
{
}

// PANZERS 0x540b40
void SScrollbar::OnMouseOut()
{
    ActiveUp = ActiveDown = false;
    Update();
}

// PANZERS 0x540bf0
// The repeat ticks send a step of 0 rows, so holding an arrow only scrolls
// once (SListBox::OnAction subtracts/adds the param). Kept as in HD.
void SScrollbar::OnTimer(int id, unsigned int elapsed)
{
    (void)elapsed;
    if (id == FirstClickTimer) {
        if (PressedUp || PressedDown)
            TickTimer = SetTimer(0x46);
        KillTimer(&FirstClickTimer);
        return;
    }
    if (id == TickTimer) {
        if (PressedUp) {
            SendAction(PZA_SCROLL_UP, 0);
            return;
        }
        if (PressedDown) {
            SendAction(PZA_SCROLL_DOWN, 0);
            return;
        }
        KillTimer(&TickTimer);
    }
}

// PANZERS 0x540ca0
void SScrollbar::Update()
{
    if (BackFrame < 0 || !Enabled)
        return;
    if (PageSize < Range) {
        int track = Height - 0x2c;
        KnobH = (int)(((double)PageSize * (double)track) / (double)Range);
        if (KnobH < 6)
            KnobH = 5;
        if (!Dragging)   // _PTR_007f3508 = 22.0
            KnobY = (int)(((double)(track - KnobH) * (double)Pos) / (double)(Range - PageSize) + 22.0);
        Board->MoveFrame(KnobTopFrame, 3, KnobY);
        Board->MoveFrame(KnobMidFrame, 3, KnobY + 1);
        Board->ResizeFrame(KnobMidFrame, 0x10, KnobH - 2);
        Board->MoveFrame(KnobBottomFrame, 3, KnobY - 2 + KnobH);
        Board->ShowFrame(KnobTopFrame, true);
        Board->ShowFrame(KnobMidFrame, true);
        Board->ShowFrame(KnobBottomFrame, true);
        Board->SetSpriteGlyph(UpFrame, g_MenuControlsFont, 0x21);
        Board->SetSpriteGlyph(DownFrame, g_MenuControlsFont, 0x25);
        if (PressedUp)
            Board->SetSpriteGlyph(UpFrame, g_MenuControlsFont, 0x23);
        else if (PressedDown)
            Board->SetSpriteGlyph(DownFrame, g_MenuControlsFont, 0x27);
        else if (ActiveUp)
            Board->SetSpriteGlyph(UpFrame, g_MenuControlsFont, 0x22);
        else if (ActiveDown)
            Board->SetSpriteGlyph(DownFrame, g_MenuControlsFont, 0x26);
    } else {
        Board->SetSpriteGlyph(UpFrame, g_MenuControlsFont, 0x24);
        Board->SetSpriteGlyph(DownFrame, g_MenuControlsFont, 0x28);
        Board->ShowFrame(KnobTopFrame, false);
        Board->ShowFrame(KnobMidFrame, false);
        Board->ShowFrame(KnobBottomFrame, false);
    }
}

// ===========================================================================
// SListBox
// ===========================================================================

// PANZERS 0x53bdc0 (over SGenericListBox ctor 0x53bd10)
SListBox::SListBox()
{
    ToolTipFeatureEnabled = false;
    Items = nullptr;
    ItemCount = 0;
    ItemMax = 0;
    LineHeight = 0;
    VisibleLines = 0;
    TopIndex = 0;
    ColumnWidth = 0;
    Columns = 1;
    Selectable = false;
    CurSel = -1;
    Hover = -1;
    HasScrollbar = false;
    LastClickTime = 0.0f;
    Font = -1;
    TextFrames = nullptr;
    BoxFrames = nullptr;
    Align = 0;
    SelColor = 0x80808080;
}

// PANZERS 0x53bdf0
void SListBox::FreeItems()
{
    for (int i = 0; i < ItemCount; ++i) {
        if (Items[i].Text2.buf) { delete[] Items[i].Text2.buf; Items[i].Text2.buf = nullptr; }
        if (Items[i].Text.buf) { delete[] Items[i].Text.buf; Items[i].Text.buf = nullptr; }
    }
    free(Items);
    Items = nullptr;
    ItemMax = 0;
    ItemCount = 0;
}

// PANZERS 0x53be80
SListBox::~SListBox()
{
    CurSel = -1;
    ResetContent();
    TopIndex = 0;
    Update();
    if (BackFrame >= 0) {
        delete[] TextFrames;
        TextFrames = nullptr;
        delete[] BoxFrames;
        BoxFrames = nullptr;
    }
    FreeItems();
    if (Hint.buf) {
        delete[] Hint.buf;
        Hint.buf = nullptr;
    }
    // ~SScrollbar (0x540690) runs as the member is destroyed; the SWINE
    // SWidget dtor unlinks it from this widget's child list first.
}

// PANZERS 0x543bf0
void SListBox::SetHint(const char* text)
{
    Hint = text ? text : "";
}

// PANZERS 0x53c1c0
void SListBox::GenericCreate(int lineHeight, int lines, int columnWidth, int columns,
                             bool selectable, bool background, bool scrollbars)
{
    ColumnWidth = columnWidth;
    LineHeight = lineHeight;
    Columns = columns;
    Selectable = selectable;
    HasScrollbar = scrollbars;
    VisibleLines = lines;
    Resize(Width, lineHeight * lines + 0xc);
    if (BackColor != 0)
        Logger.g->Panic("SGenericListBox<T>::Create: BackColor != 0");
    SDXWidget::Create(0);
    if (background)
        PzDrawFrameBox(this);                                      // 0x543d90
    if (HasScrollbar) {
        InsertChild(&Scrollbar);                                   // vtbl +0x54
        Scrollbar.Create(Width - 0x1b, 4, Height - 8, VisibleLines);
    }
}

// PANZERS 0x53c260
void SListBox::Create(int font, int lines, bool background, bool scrollbars,
                      int align, bool selectable)
{
    Font = font;
    Align = align;
    GenericCreate(PzGetFontHeight(PzFont(font)), lines, 0, 1, selectable, background, scrollbars);
    TextFrames = new int[VisibleLines];
    BoxFrames = new int[VisibleLines];
    int sb = HasScrollbar ? 0x16 : 0;
    for (int i = 0; i < VisibleLines; ++i) {
        BoxFrames[i] = Board->CreateFrame(FT_BOX, BackFrame, 6, LineHeight * i + 6, 0, 1);
        Board->ResizeFrame(BoxFrames[i], (Width - sb) - 0xc, LineHeight);
        TextFrames[i] = Board->CreateFrame(FT_FIXTEXT, BoxFrames[i], 5, 0, 0, 1);
        Board->ResizeFrame(TextFrames[i], (Width - sb) - 0x16, LineHeight);
    }
    Update();
}

// PANZERS 0x53c020
int SListBox::AddItem(const char* text, const char* text2, unsigned int color, unsigned int data)
{
    // 0x53bfb0: SDArray grow (16, then x6/5)
    if (ItemCount == ItemMax) {
        int n = ItemMax < 0x10 ? 0x10 : ItemMax * 6 / 5;
        Items = (SListBoxItem*)realloc(Items, n * sizeof(SListBoxItem));
        memset(Items + ItemMax, 0, (n - ItemMax) * sizeof(SListBoxItem));
        ItemMax = n;
    }
    int i = ItemCount++;
    Items[i].Text = text ? text : "";
    Items[i].Text2 = text2 ? text2 : "";
    Items[i].Color = color;
    Items[i].Data = data;
    Update();
    return i;
}

// PANZERS 0x53c0c0 (with size 0)
void SListBox::ResetContent()
{
    for (int i = 0; i < ItemCount; ++i) {
        if (Items[i].Text2.buf) { delete[] Items[i].Text2.buf; Items[i].Text2.buf = nullptr; }
        if (Items[i].Text.buf) { delete[] Items[i].Text.buf; Items[i].Text.buf = nullptr; }
    }
    ItemCount = 0;
    if (Items)
        memset(Items, 0, ItemMax * sizeof(SListBoxItem));
}

// PANZERS 0x53c580
const char* SListBox::GetItemText(int index) const
{
    if (index < 0 || index >= ItemCount)
        Logger.g->Panic("SListBox::GetText: Invalid index");
    return Items[index].Text.buf ? Items[index].Text.buf : "";
}

// PANZERS 0x53caf0
int SListBox::SetCurSel(int index)
{
    if (index < ItemCount && index >= -1) {
        int old = CurSel;
        CurSel = index;
        Update();
        return old;
    }
    return -2;
}

// PANZERS 0x53cc00
int SListBox::SetTopIndex(int index)
{
    if (index >= 0 && index < (ItemCount - 1 + Columns) / Columns) {
        int old = TopIndex;
        TopIndex = index;
        Update();
        return old;
    }
    return -1;
}

// PANZERS 0x53c3c0
void SListBox::EnsureVisible(int index)
{
    if (index < 0 || index >= ItemCount)
        return;
    int top = TopIndex * Columns;
    if ((VisibleLines + top) * Columns <= index)
        top = (index / Columns - VisibleLines) + 1;
    if (index < Columns * top)
        top = index / Columns;
    if (top < 0)
        top = 0;
    if (top < (ItemCount - 1 + Columns) / Columns) {
        TopIndex = top;
        Update();
    }
    SendAction(PZA_LISTBOX_SCROLLED, 0);
}

static int ListIndexAt(const SListBox* l, int x, int y)
{
    int col;
    if (l->ColumnWidth == 0) {
        col = 0;
    } else {
        col = l->Columns - 1;
        int c = (x - 6) / l->ColumnWidth;
        if (c < l->Columns - 1)
            col = c;
    }
    return (l->TopIndex + (y - 6) / l->LineHeight) * l->Columns + col;
}

// PANZERS 0x53c6d0
bool SListBox::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_UP) {
        if (!Selectable) {
            if (TopIndex > 0) {
                SetTopIndex(TopIndex - 1);
                SendAction(PZA_LISTBOX_SCROLLED, 0);
            }
        } else if (CurSel > 0) {
            SetCurSel(CurSel - 1);
            EnsureVisible(CurSel);
            return true;
        }
    } else {
        if (key != VK_DOWN)
            return false;
        int last = ItemCount - 1;
        if (!Selectable) {
            if (VisibleLines + TopIndex < (last + Columns) / Columns) {
                SetTopIndex(TopIndex + 1);
                SendAction(PZA_LISTBOX_SCROLLED, 0);
            }
            return true;
        }
        if (CurSel < last) {
            SetCurSel(CurSel + 1);
            EnsureVisible(CurSel);
            return true;
        }
    }
    return true;
}

// PANZERS 0x53c7a0
void SListBox::OnMouseDown(int button, int x, int y, int shift)
{
    if (!Selectable)
        return;
    if (button == 1) {
        float now = (float)((double)Timer.GetTickValue() / 1000.0);  // 0x661800 (seconds)
        // Double click: within DAT_007f2fcc = 0.75 s on the selected row.
        if (LastClickTime != 0.0f && now - LastClickTime <= 0.75f && CurSel >= 0) {
            int col = CurSel % Columns;
            int row = CurSel / Columns - TopIndex;
            if (LineHeight * row + 6 <= y && y < (row + 1) * LineHeight + 6
                && (ColumnWidth == 0 || (col * ColumnWidth + 6 <= x && x < (col + 1) * ColumnWidth + 6))) {
                LastClickTime = 0.0f;
                SendAction(PZA_LISTBOX_DBLCLICK, CurSel);
                return;
            }
        }
        LastClickTime = (float)(int)now;   // HD stores (int)(float)t into the float field
        int idx = ListIndexAt(this, x, y);
        if (idx < ItemCount && idx >= -1) {
            int old = CurSel;
            CurSel = idx;
            Update();
            if (old != -2) {
                PlayMenuSound("menu/button_down.wav", -12.0f);
                SendAction(PZA_LISTBOX_SELECT, CurSel);
                return;
            }
        }
    }
    SDXWidget::OnMouseDown(button, x, y, shift);
}

// PANZERS 0x53c920
void SListBox::OnMouseMove(int x, int y, int shift)
{
    if (!Selectable)
        return;
    Hover = ListIndexAt(this, x, y);
    Update();
    SDXWidget::OnMouseMove(x, y, shift);
}

// PANZERS 0x53c9b0
void SListBox::OnMouseWheel(int delta, int x, int y, int keys)
{
    (void)x; (void)y; (void)keys;
    int rows = (ItemCount - 1 + Columns) / Columns;
    if (delta < 1) {
        if (VisibleLines + TopIndex < rows) {
            int t = TopIndex + 1;
            if (t >= 0 && t < rows) {
                TopIndex = t;
                Update();
            }
            SendAction(PZA_LISTBOX_SCROLLED, 0);
        }
    } else if (TopIndex > 0) {
        int t = TopIndex - 1;
        if (t >= 0 && t < rows) {
            TopIndex = t;
            Update();
        }
        SendAction(PZA_LISTBOX_SCROLLED, 0);
    }
}

// PANZERS 0x53c990
void SListBox::OnMouseOut()
{
    if (!Selectable)
        return;
    Hover = -1;
    Update();
    SDXWidget::OnMouseOut();
}

// PANZERS 0x53c620
bool SListBox::OnAction(SWidget* source, int action, int param)
{
    (void)source;
    int rows = (ItemCount - 1 + Columns) / Columns;
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
    SendAction(PZA_LISTBOX_SCROLLED, 0);
    return true;
}

// PANZERS 0x53cc70
void SListBox::Update()
{
    if (BackFrame < 0)
        return;
    for (int i = 0; i < VisibleLines; ++i) {
        int idx = TopIndex + i;
        Board->SetBoxColor(BoxFrames[i], idx == CurSel ? SelColor : 0);
        if (idx < ItemCount) {
            Board->SetText(TextFrames[i], PzFont(Font), Align, Items[idx].Text.buf ? Items[idx].Text.buf : "");
            if (!Selectable || idx != Hover)
                Board->SetTextColor(TextFrames[i], Items[idx].Color);
            else
                Board->SetTextColor(TextFrames[i], 0xffffff);
        } else {
            Board->SetText(TextFrames[i], PzFont(Font), Align, "");
        }
    }
    if (HasScrollbar)
        Scrollbar.SetRange((ItemCount - 1 + Columns) / Columns, TopIndex);
}

} // namespace pz
