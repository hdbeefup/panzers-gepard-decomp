// src/panzers/loadgame_menu.cpp
// The Load Game screen (SLoadMenu) and its list box (SSaveLoadListBox),
// lifted from the HD exe; see loadgame_menu.h. The list box is HD's
// SGenericListBox template over SSaveLoadListBoxItem: the same code as
// pz::SListBox (pzwidgets.cpp) with three text columns. OWNER: agent S (M4).

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "loadgame_menu.h"
#include "loadgame.h"
#include "pzboard.h"
#include "board.h"
#include "iconcert.h"
#include "gettext.h"
#include "logger.h"
#include "timer.h"
#include "stub_log.h"
#include "m3common.h"
#include "world_save.h"

static int LbFont(int hdFont)
{
    return (hdFont >= 0 && hdFont < 6) ? g_PzFont[hdFont] : hdFont;
}

static void LbSound(const char* name, float db)
{
    if (Concert)
        Concert->PlaySound(name, db, 0, -1);                      // concert +0x50
}

static const char* Tx(const char* id) { return GetText("panzers/InGameMenu.cpp", id); }

static void SetStr(SString* s, const char* text)
{
    *s = text ? text : "";
}

// ===========================================================================
// SSaveLoadListBox
// ===========================================================================

namespace pz {

// PANZERS 0x53eef0 (over SGenericListBox ctor 0x53ee40)
SSaveLoadListBox::SSaveLoadListBox()
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
    CodeFrames = TitleFrames = DateFrames = BoxFrames = nullptr;
    TitleWidth = 0;
}

void SSaveLoadListBox::FreeItems()
{
    for (int i = 0; i < ItemCount; ++i) {
        FreeSString(&Items[i].Code);
        FreeSString(&Items[i].Title);
        FreeSString(&Items[i].Date);
        FreeSString(&Items[i].File);
    }
    free(Items);
    Items = nullptr;
    ItemCount = ItemMax = 0;
}

// PANZERS 0x53f110
SSaveLoadListBox::~SSaveLoadListBox()
{
    CurSel = -1;
    ResetContent();
    TopIndex = 0;
    Update();
    delete[] CodeFrames;
    delete[] TitleFrames;
    delete[] DateFrames;
    delete[] BoxFrames;
    CodeFrames = TitleFrames = DateFrames = BoxFrames = nullptr;
    FreeItems();
}

// PANZERS 0x53f4c0
// One line per row: a box (+0x130) with three fixed-size texts: the code at
// x 5 (codeW wide), the title after gap1, the date after gap2. The date
// column is as wide as a sample date (1. 1. 2006 0:00 in the user's format)
// plus 2; the title takes the rest of titleRight.
void SSaveLoadListBox::Create(int font, int lines, int codeW, int gap1, int titleRight, int gap2,
                              int dateExtra, bool selectable)
{
    Font = font;
    int lh = PzGetFontHeight(LbFont(font));                       // board +0x8c
    LineHeight = lh;
    Selectable = selectable;
    VisibleLines = lines;
    TopIndex = 0;
    Columns = 1;
    HasScrollbar = true;
    Resize(Width, lh * lines + 0xc);                              // vtbl +0x10
    if (BackColor != 0)
        Logger.g->Panic("SGenericListBox<T>::Create: BackColor != 0");
    SDXWidget::Create(0);                                         // 0x539a10
    PzDrawFrameBox(this);                                         // 0x543d90
    if (HasScrollbar) {
        InsertChild(&Scrollbar);                                  // vtbl +0x54
        Scrollbar.Create(Width - 0x1b, 4, Height - 8, VisibleLines);   // 0x540770
    }
    CodeFrames = new int[VisibleLines];
    TitleFrames = new int[VisibleLines];
    DateFrames = new int[VisibleLines];
    BoxFrames = new int[VisibleLines];
    FILETIME ft;
    ft.dwLowDateTime = 0x7b7eee00;
    ft.dwHighDateTime = 0x1c90966;
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    char date[260], time[260], sample[530];
    GetDateFormatA(0, DATE_SHORTDATE, &st, nullptr, date, sizeof(date));
    GetTimeFormatA(0, TIME_NOSECONDS | TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &st, nullptr, time, sizeof(time));
    _snprintf(sample, sizeof(sample) - 1, "%s %s", date, time);   // date + " " (0x7f33a0) + time
    sample[sizeof(sample) - 1] = 0;
    int tw = 0, th = 0;
    Board->GetTextExtent(LbFont(font), sample, (int)strlen(sample), &tw, &th, 1.0f);   // board +0x88(font, text, len, ...)
    int dateW = tw + 2;
    TitleWidth = (titleRight - 0x27) - (dateW - dateExtra);       // +0x134
    int sb = HasScrollbar ? 0x16 : 0;
    for (int i = 0; i < VisibleLines; ++i) {
        BoxFrames[i] = Board->CreateFrame(FT_BOX, BackFrame, 6, lh * i + 6, 0, 1);
        Board->ResizeFrame(BoxFrames[i], (Width - sb) - 0xc, lh);
        CodeFrames[i] = Board->CreateFrame(FT_FIXTEXT, BoxFrames[i], 5, 0, 0, 1);
        Board->ResizeFrame(CodeFrames[i], codeW, lh);
        TitleFrames[i] = Board->CreateFrame(FT_FIXTEXT, BoxFrames[i], gap1 + codeW + 5, 0, 0, 1);
        Board->ResizeFrame(TitleFrames[i], TitleWidth, lh);
        DateFrames[i] = Board->CreateFrame(FT_FIXTEXT, BoxFrames[i], gap2 + 5 + gap1 + codeW + TitleWidth, 0, 0, 1);
        Board->ResizeFrame(DateFrames[i], dateW, lh);
    }
    Update();                                                     // vtbl +0x78
}

// PANZERS 0x53f1b0
int SSaveLoadListBox::AddItem(const char* code, const char* title, const char* date, const char* file,
                              const SYSTEMTIME& t, unsigned int color)
{
    if (ItemCount == ItemMax) {                                   // 0x53f140 (SDArray::AddEmpty)
        int n = ItemMax < 0x10 ? 0x10 : ItemMax * 6 / 5;
        Items = (SSaveLoadListBoxItem*)realloc(Items, n * sizeof(SSaveLoadListBoxItem));
        memset(Items + ItemMax, 0, (n - ItemMax) * sizeof(SSaveLoadListBoxItem));
        ItemMax = n;
    }
    int i = ItemCount++;
    SetStr(&Items[i].Code, code);
    SetStr(&Items[i].Title, title);
    SetStr(&Items[i].Date, date);
    SetStr(&Items[i].File, file);
    Items[i].Color = color;
    Items[i].Time = t;
    if (!Selectable)
        EnsureVisible(i);                                         // 0x53f8f0
    Update();
    return i;
}

// PANZERS 0x53f390 (qsort comparator: newest first; the day of the week is skipped)
static int CompareSaveTime(const void* pa, const void* pb)
{
    const SYSTEMTIME& a = ((const SSaveLoadListBoxItem*)pa)->Time;
    const SYSTEMTIME& b = ((const SSaveLoadListBoxItem*)pb)->Time;
    const WORD ka[7] = { a.wYear, a.wMonth, a.wDay, a.wHour, a.wMinute, a.wSecond, a.wMilliseconds };
    const WORD kb[7] = { b.wYear, b.wMonth, b.wDay, b.wHour, b.wMinute, b.wSecond, b.wMilliseconds };
    for (int k = 0; k < 7; ++k) {
        if (ka[k] < kb[k])
            return 1;
        if (ka[k] != kb[k])
            return -1;
    }
    return 0;
}

// PANZERS 0x53ff80
void SSaveLoadListBox::Sort(int from)
{
    if (ItemCount > from)
        qsort(Items + from, ItemCount - from, sizeof(SSaveLoadListBoxItem), CompareSaveTime);
    Update();
}

// PANZERS 0x53ffc0
void SSaveLoadListBox::ResetContent()
{
    CurSel = -1;
    for (int i = 0; i < ItemCount; ++i) {                         // 0x53f290(0)
        FreeSString(&Items[i].Code);
        FreeSString(&Items[i].Title);
        FreeSString(&Items[i].Date);
        FreeSString(&Items[i].File);
    }
    ItemCount = 0;
    if (Items)
        memset(Items, 0, ItemMax * sizeof(SSaveLoadListBoxItem));
    TopIndex = 0;
    Update();
}

// PANZERS 0x53f9c0
const char* SSaveLoadListBox::GetFileName(int index) const
{
    if (index < 0 || index >= ItemCount)
        Logger.g->Panic("SSaveLoadListBox::GetFileName: Invalid index");
    return Items[index].File.buf ? Items[index].File.buf : "";
}

// PANZERS 0x53fa70
void SSaveLoadListBox::GetText(int index, SString* code, SString* title, SString* date) const
{
    if (index < 0 || index >= ItemCount)
        Logger.g->Panic("SSaveLoadListBox::GetText: Invalid index");
    SetStr(code, SStr(Items[index].Code));                        // 0x52c2c0
    SetStr(title, SStr(Items[index].Title));
    SetStr(date, SStr(Items[index].Date));
}

int SSaveLoadListBox::SetCurSel(int index)
{
    if (index < ItemCount && index >= -1) {
        int old = CurSel;
        CurSel = index;
        Update();
        return old;
    }
    return -2;
}

int SSaveLoadListBox::SetTopIndex(int index)
{
    if (index >= 0 && index < (ItemCount - 1 + Columns) / Columns) {
        int old = TopIndex;
        TopIndex = index;
        Update();
        return old;
    }
    return -1;
}

// PANZERS 0x53f8f0
void SSaveLoadListBox::EnsureVisible(int index)
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

static int RowAt(const SSaveLoadListBox* l, int y)
{
    return l->TopIndex + (y - 6) / l->LineHeight;
}

// PANZERS 0x53fc10 (SGenericListBox::OnKeyDown, as pz::SListBox 0x53c6d0)
bool SSaveLoadListBox::OnKeyDown(int key, bool repeat)
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

// PANZERS 0x53fce0 (as pz::SListBox 0x53c7a0)
void SSaveLoadListBox::OnMouseDown(int button, int x, int y, int shift)
{
    if (!Selectable)
        return;
    if (button == 1) {
        float now = (float)((double)Timer.GetTickValue() / 1000.0);   // 0x661800
        if (LastClickTime != 0.0f && now - LastClickTime <= 0.75f && CurSel >= 0) {   // DAT_007f2fcc
            int row = CurSel - TopIndex;
            if (LineHeight * row + 6 <= y && y < (row + 1) * LineHeight + 6) {
                LastClickTime = 0.0f;
                SendAction(PZA_LISTBOX_DBLCLICK, CurSel);
                return;
            }
        }
        LastClickTime = (float)(int)now;
        int idx = RowAt(this, y);
        if (idx < ItemCount && idx >= -1) {
            int old = CurSel;
            CurSel = idx;
            Update();
            if (old != -2) {
                LbSound("menu/button_down.wav", -12.0f);
                SendAction(PZA_LISTBOX_SELECT, CurSel);
                return;
            }
        }
    }
    SDXWidget::OnMouseDown(button, x, y, shift);
}

// PANZERS 0x53fe60
void SSaveLoadListBox::OnMouseMove(int x, int y, int shift)
{
    if (!Selectable)
        return;
    Hover = RowAt(this, y);
    Update();
    SDXWidget::OnMouseMove(x, y, shift);
}

// PANZERS 0x53fef0
void SSaveLoadListBox::OnMouseWheel(int delta, int x, int y, int keys)
{
    (void)x; (void)y; (void)keys;
    int rows = (ItemCount - 1 + Columns) / Columns;
    if (delta < 1) {
        if (VisibleLines + TopIndex < rows) {
            SetTopIndex(TopIndex + 1);
            SendAction(PZA_LISTBOX_SCROLLED, 0);
        }
    } else if (TopIndex > 0) {
        SetTopIndex(TopIndex - 1);
        SendAction(PZA_LISTBOX_SCROLLED, 0);
    }
}

// PANZERS 0x53fed0
void SSaveLoadListBox::OnMouseOut()
{
    if (!Selectable)
        return;
    Hover = -1;
    Update();
    SDXWidget::OnMouseOut();
}

// PANZERS 0x53fb60 (the scroll bar's actions)
bool SSaveLoadListBox::OnAction(SWidget* source, int action, int param)
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

// PANZERS 0x540200
// Per visible row: the box grey (0x80808080) when selected; code, title and
// date; white under the mouse, else the item's colour; empty rows blank.
void SSaveLoadListBox::Update()
{
    if (BackFrame < 0)
        return;
    for (int i = 0; i < VisibleLines; ++i) {
        int idx = TopIndex + i;
        Board->SetBoxColor(BoxFrames[i], idx == CurSel ? 0x80808080 : 0);   // board +0x3c
        if (idx < ItemCount) {
            const SSaveLoadListBoxItem& it = Items[idx];
            Board->SetText(CodeFrames[i], LbFont(Font), 0, it.Code.buf ? it.Code.buf : "");   // board +0x34
            Board->SetText(TitleFrames[i], LbFont(Font), 0, it.Title.buf ? it.Title.buf : "");
            Board->SetText(DateFrames[i], LbFont(Font), 0, it.Date.buf ? it.Date.buf : "");
            unsigned c = idx == Hover ? 0xffffff : it.Color;
            Board->SetTextColor(CodeFrames[i], c);                // board +0x28
            Board->SetTextColor(TitleFrames[i], c);
            Board->SetTextColor(DateFrames[i], c);
        } else {
            Board->SetText(CodeFrames[i], LbFont(Font), 0, "");
            Board->SetText(TitleFrames[i], LbFont(Font), 0, "");
            Board->SetText(DateFrames[i], LbFont(Font), 0, "");
        }
    }
    if (HasScrollbar)
        Scrollbar.SetRange((ItemCount - 1 + Columns) / Columns, TopIndex);   // 0x540c80
}

} // namespace pz

// ===========================================================================
// SLoadMenu
// ===========================================================================

// PANZERS 0x62cbc0
SLoadMenu::SLoadMenu()
{
    Picture = -1;                                                 // +0x278
    PictureFrame = -1;
}

// PANZERS 0x62d310
SLoadMenu::~SLoadMenu()
{
}

// PANZERS 0x62f950
// "Load Game" centre menu: in game Load + Back, from the main menu Load
// only (hint "Load selected game", off until a game is selected); the
// "Games" caption and the list of LoadSavedGameNames 0x595fa0, newest
// first.
void SLoadMenu::Create(bool fromMainMenu)
{
    PZ_M3_TRACE("SLoadMenu::Create (0x62f950)");
    FromMainMenu = fromMainMenu;
    SCenterMenu::Create(Tx("Load Game"), fromMainMenu);           // 0x64bc80
    if (!fromMainMenu) {
        CreateOkCancel(&LoadButton, Tx("Load"), &BackButton, Tx("Back"));   // 0x64bf60
    } else {
        InsertChild(&LoadButton);                                 // 0x64bfd0
        LoadButton.SetPosition(0x18b, 400, 0, 0);
        LoadButton.Create(1, Tx("Load"));
    }
    // HD 0x543bf0: the hint "Load selected game" (tool tips are not mapped).
    LoadButton.SetEnable(false);                                  // vtbl +0x70(0)
    // HD: a sprite (board +0x08(1, ..., W / 2 - 200, 0xd2)) with the font
    // +0x278, which is -1 here (no picture).
    int caption = Board->CreateFrame(FT_TEXT, BackFrame, 0x32, 0x32, 0, 1);   // board +0x08(2, ...)
    Board->SetText(caption, LbFont(2), 0, Tx("Games"));           // board +0x34(f, 2, 0, "Games")
    InsertChild(&List);                                           // vtbl +0x54
    List.SetPosition(0x28, 0x46, 0x208, 0);                       // vtbl +0x08
    List.Create(0, 0x14, 0x1e, 8, 0x172, 8, 0x5f, true);          // 0x53f4c0
    pz::SHdArray<pz::SLoadSaveName> names = { nullptr, 0, 0 };
    pz::LoadSavedGameNames(&names);                               // 0x595fa0
    for (int i = 0; i < names.Size; ++i) {
        const pz::SLoadSaveName& n = names.Array[i];
        List.AddItem(pz::SStr(n.Code), pz::SStr(n.Title), pz::SStr(n.Date), pz::SStr(n.File), n.Time, 0xd0d0d0);
    }
    List.Sort(0);                                                 // 0x53ff80(0)
    if (List.CurSel < 0)
        LoadButton.SetEnable(false);
    // HD 0x5439f0: focus; 0x62cde0 frees the names.
    for (int i = 0; i < names.Size; ++i) {
        pz::FreeSString(&names.Array[i].Code);
        pz::FreeSString(&names.Array[i].Title);
        pz::FreeSString(&names.Array[i].Date);
        pz::FreeSString(&names.Array[i].File);
    }
    free(names.Array);
}

// PANZERS 0x630f80
void SLoadMenu::LoadSelected()
{
    if (List.CurSel < 0)
        return;
    static char s_File[260];                                      // HD passes the SString's buffer
    _snprintf(s_File, sizeof(s_File) - 1, "%s", List.GetFileName(List.CurSel));   // 0x53f9c0
    s_File[sizeof(s_File) - 1] = 0;
    SendAction(0x494c1, (int)(size_t)s_File);                     // 0x543930
}

// PANZERS 0x632500
bool SLoadMenu::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_RETURN && LoadButton.Enabled) {                 // +0x90
        LoadSelected();
        return true;
    }
    return false;
}

// PANZERS 0x631f40
bool SLoadMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_BUTTON_CLICK) {
        if (source == &LoadButton) {
            LoadSelected();
            return true;
        }
        if (source != &BackButton)
            return false;
        SendAction(0x494c3, 0);                                   // the menu closes itself
        return true;
    }
    if (action == PZA_LISTBOX_SELECT) {
        LoadButton.SetEnable(true);                               // vtbl +0x70(1)
        return true;
    }
    if (action != PZA_LISTBOX_DBLCLICK)
        return false;
    LoadSelected();
    return true;
}

// SMainMenu::OnAction 0x63b6f0, Load Game button: new 0x280 (0x62cbc0),
// the super window's child, Create(1).
SWidget* PzCreateLoadMenu(SWidget* parent)
{
    SLoadMenu* m = new SLoadMenu();
    if (parent)
        parent->InsertChild(m);
    m->Create(true);
    return m;
}
