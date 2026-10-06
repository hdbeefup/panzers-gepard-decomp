// src/panzers/market.cpp
// SMarket, the Headquarters screen (market.h). OWNER: agent H.
//
// Lifted for the single-player path (Training Camp, campaign); the
// multiplayer deck / army-file branches (DAT_008f1a74 != 0, campaign
// +0x11c "deck making") are logged and skipped. Not lifted: the 3D preview
// (SMarketView, its own SWorld "vasarlomenu" and sub-viewport) and with it
// the unit-state button of the info panel (SUnitButton::SetUnit on the
// preview unit); the army-file save (SaveArmy 0x64a180, multiplayer only).
//
// HD board slots as in pzwidgets.cpp. Glyph tables of the custom fonts come
// from the stack tables of Create 0x6407d0 (extracted from the decompile).

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <new>
#include "market.h"
#include "hud.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"
#include "pzboard.h"
#include "board.h"
#include "iconcert.h"
#include "timer.h"
#include "campaign.h"
#include "punit.h"
#include "idriver.h"
#include "properties.h"
#include "worldapi.h"
#include "world.h"
#include "unit.h"
#include "blockmaprefresh.h"
#include "window.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iterrain.h"
#include "pz/iviewport.h"

static const char* Tx(const char* id) { return GetText("panzers/Market.cpp", id); }
static const char* Str(const SString& s) { return s.buf ? s.buf : ""; }

static void SetStr(SString* s, const char* text)
{
    // SString::operator= (0x52c320)
    *s = text ? text : "";
}

static void FreeStr(SString* s)
{
    delete[] s->buf;
    s->buf = nullptr;
    s->size = 0;
}

static int HwFont(int hdFont)
{
    return (hdFont >= 0 && hdFont < 6) ? g_PzFont[hdFont] : hdFont;
}

static pz::SPUnit* PUnit(const char* name)
{
    // SUnitRegistry::GetPUnit(name, 0) (0x5d0e70). The recompile's GetPUnit
    // opens a modal warning box for an unknown name; the market looks up
    // names it built itself, so it logs instead.
    pz::SUnitRegistry* r = pz::g_UnitRegistry;
    if (!r || !name)
        return nullptr;
    for (int i = 0; i < r->Count; ++i)
        if (_stricmp(Str(r->Entries[i].Name), name) == 0)
            return r->Entries[i].Type;
    Logger.g->Log(1, "SUnitRegistry::GetPUnit - PUnit = NULL, UnitName = %s (market)", name);
    return nullptr;
}

// The display names of the unit prototypes, from the tail of the registry
// ctor 0x5cfe30 (units.ini [<unit>] "short name" -> +0x68, short name + " - "
// + "Type name" -> +0x70, "Unit desc" -> +0x78 (">>kitoltendo<<" = none);
// buildings get GetText("Building")). The recompile's registry ctor skips
// them (world/unitregistry.cpp: "not used by the menu"), so the market fills
// them once here. TODO(owner of unitregistry.cpp): move into 0x5cfe30.
static void LoadUnitDisplayNames()
{
    static bool done = false;
    pz::SUnitRegistry* r = pz::g_UnitRegistry;
    if (done || !r)
        return;
    done = true;
    SProperties ini("units.ini", true);                            // 0x65fe80("units.ini", 1)
    for (int i = 0; i < r->Count; ++i) {
        pz::SPUnit* p = r->Entries[i].Type;
        if (!p)
            continue;
        if (p->ClassType == 9) {
            const char* b = GetText("world/UnitRegistry.cpp", "Building");
            SetStr(&p->IniName68, b);
            SetStr(&p->IniName70, b);
            continue;
        }
        const char* unit = Str(r->Entries[i].Name);
        const char* shortName = ini.GetString(unit, "short name", "???");
        SetStr(&p->IniName68, shortName);
        char full[256];
        _snprintf(full, sizeof full, "%s - %s", shortName ? shortName : "", ini.GetString(unit, "Type name", "???"));
        full[sizeof full - 1] = 0;
        SetStr(&p->IniName70, full);
        const char* desc = ini.GetString(unit, "Unit desc", "");
        SString* d = (SString*)((unsigned char*)p + 0x78);
        if (desc && _stricmp(desc, ">>kitoltendo<<") == 0)
            desc = "";
        SetStr(d, desc);
    }
}

// SUnitRegistry equipment prices by slot number (0x5d0db0).
static int SlotPrice(int slot)
{
    pz::SUnitRegistry* r = pz::g_UnitRegistry;
    if (!r)
        return 0;
    switch (slot) {
    case 1: return r->PriceGrenade;        // +0xa4
    case 2: return r->PriceMolotov;        // +0xa8
    case 3: return r->PriceMagneticMine;   // +0xbc
    case 4: return r->PriceExplosives;     // +0xb8
    case 5: return r->PriceTankMine;       // +0xb0
    case 6: return r->PriceBinoculars;     // +0xc0
    case 7: return r->PriceBoat;           // +0xac
    case 8: return r->PriceMineDetector;   // +0xb4
    default: return 0;
    }
}

// SComplexButton::SetText (0x538a10): the HD SComplexButton row is shared
// (mainmenu.h); the recompile's class has no SetText yet.
static void SetButtonText(SComplexButton* b, const char* text)
{
    SetStr(&b->Text, text);
    Board->SetText(b->TextFrame, g_PzFont[PZF_SANS21_SHADOW], 2, Str(b->Text));
    b->Update();
}

// SUnitDef copy 0x560290 (operator=, agent F's campaign.h).
static void CopyUnitDef(pz::SUnitDef* dst, const pz::SUnitDef* src)
{
    pz::UnitDefCopy(dst, src);
}


// ---------------------------------------------------------------------------
// Glyph tables (stack tables of Create 0x6407d0, x, y, w, h)
// ---------------------------------------------------------------------------

#include "market_glyphs.inl"
#include "mods.h"
#if PANZERS_MOD_WIDESCREEN
#include "mod_widescreen.h"
#include "superwindow.h"
#endif

// ===========================================================================
// SFullScreenMenu
// ===========================================================================

// PANZERS 0x64bad0
SFullScreenMenu::SFullScreenMenu()
{
    ToolTipFeatureEnabled = false;
    Texture = PzLoadTexture("menu/fullscreen_menu_hq.tga");        // board +0x7c
}

// PANZERS 0x64bb50
SFullScreenMenu::~SFullScreenMenu()
{
    PzReleaseTexture(Texture);                                     // board +0x80
}

// PANZERS 0x64bd50
void SFullScreenMenu::Create(const char* title)
{
    SetBackgroundSprite(Texture, 0, false, false);                 // 0x539ba0
    SDXWidget::Create(0);                                          // 0x539a10
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);
    int text = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(text, g_PzFont[PZF_SANS21], 2, title ? title : "");
    Board->SetTextColor(text, 0xffffff);
}

// ===========================================================================
// SMarketListBox
// ===========================================================================

// PANZERS 0x63f240
SMarketListBox::SMarketListBox()
{
    ToolTipFeatureEnabled = false;
    LineHeight = VisibleLines = TopIndex = ColumnWidth = 0;
    Columns = 0;
    Selectable = false;
    CurSel = -1;
    Hover = -1;
    LastClickTime = 0.0f;
    HasScrollbar = false;
    Items = nullptr;
    ItemCount = ItemMax = 0;
    Font = -1;
    for (int i = 0; i < 4; ++i)
        IconFrames[i] = NameFrames[i] = PriceFrames[i] = BoxFrames[i] = nullptr;
    HighlightFrame = -1;
    Align = 0;
    SelColor = 0x80808080;
}

void SMarketListBox::FreeItems()
{
    for (int i = 0; i < ItemCount; ++i) {
        FreeStr(&Items[i].FileName);
        FreeStr(&Items[i].Name);
    }
    free(Items);
    Items = nullptr;
    ItemCount = ItemMax = 0;
}

// PANZERS 0x640110 (over 0x63fe50)
SMarketListBox::~SMarketListBox()
{
    for (int i = 0; i < 4; ++i) {
        delete[] IconFrames[i];
        delete[] NameFrames[i];
        delete[] PriceFrames[i];
        delete[] BoxFrames[i];
    }
    FreeItems();
}

// PANZERS 0x640720
void SMarketListBox::GenericCreate(int lineHeight, int lines, int columnWidth, int columns,
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
    SDXWidget::Create(0);
    if (background)
        PzDrawFrameBox(this);                                      // 0x543d90
    if (HasScrollbar) {
        InsertChild(&Scrollbar);
        Scrollbar.Create(Width - 0x1b, 4, Height - 8, VisibleLines); // 0x540770
    }
}

// PANZERS 0x644130
void SMarketListBox::Create(int font, int lines, int columns, bool background, bool scrollbars)
{
    Font = font;                                                   // [0x48]
    Align = 0;                                                     // [0x5a]
    int fh = PzGetFontHeight(HwFont(font));                        // board +0x8c
    GenericCreate(fh + 0x33, lines, 0x80, columns, true, background, scrollbars);
    for (int c = 0; c < Columns; ++c) {
        IconFrames[c] = new int[VisibleLines];
        NameFrames[c] = new int[VisibleLines];
        PriceFrames[c] = new int[VisibleLines];
        BoxFrames[c] = new int[VisibleLines];
    }
    int sb = HasScrollbar ? 0x16 : 0;
    for (int r = 0; r < VisibleLines; ++r) {
        int x = 6;
        for (int c = 0; c < Columns; ++c, x += 0x80) {
            int box = Board->CreateFrame(FT_BOX, BackFrame, x, LineHeight * r + 6, 0, 1);
            BoxFrames[c][r] = box;
            Board->ResizeFrame(box, ((Width - sb) - 0xc) / 2, LineHeight);
            int name = Board->CreateFrame(FT_FIXTEXT, box, 4, 0x32, 0, 1);
            NameFrames[c][r] = name;
            Board->ResizeFrame(name, ((Width - sb) - 0xc) / 2 - 8, LineHeight);
            int icon = Board->CreateFrame(FT_SPRITE, box, 0, 0, 0, 1);
            IconFrames[c][r] = icon;
            Board->ResizeFrame(icon, (Width - sb) - 0x16, LineHeight);
            PriceFrames[c][r] = Board->CreateFrame(FT_TEXT, icon, 0x7b, 0x32 - fh, 0, 1);
        }
    }
    HighlightFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);
    Board->SetSpriteGlyph(HighlightFrame, g_MenuControlsFont, 0x42);
    Board->ShowFrame(HighlightFrame, false);
    Update();
}

// PANZERS 0x640620
void SMarketListBox::ResetContent(int size)
{
    for (int i = 0; i < ItemCount; ++i) {
        FreeStr(&Items[i].FileName);
        FreeStr(&Items[i].Name);
    }
    ItemCount = size;
    if (ItemMax < size) {
        ItemMax = size;
        Items = (SMarketListBoxItem*)realloc(Items, size * sizeof(SMarketListBoxItem));
    }
    if (Items)
        memset(Items, 0, ItemMax * sizeof(SMarketListBoxItem));
}

// PANZERS 0x6402f0 (grow 0x640280)
int SMarketListBox::AddItem(const char* name, const char* file, int font, int glyph,
                            unsigned int color, int price)
{
    if (ItemCount == ItemMax) {
        int n = ItemMax < 0x10 ? 0x10 : ItemMax * 6 / 5;
        Items = (SMarketListBoxItem*)realloc(Items, n * sizeof(SMarketListBoxItem));
        memset(Items + ItemMax, 0, (n - ItemMax) * sizeof(SMarketListBoxItem));
        ItemMax = n;
    }
    int i = ItemCount++;
    SetStr(&Items[i].Name, name);
    SetStr(&Items[i].FileName, file);
    Items[i].Font = font;
    Items[i].Glyph = glyph;
    Items[i].Color = color;
    Items[i].Price = price;
    Update();
    return i;
}

// PANZERS 0x644d40
int SMarketListBox::GetPrice(int index) const
{
    if (index < 0 || index >= ItemCount)
        Logger.g->Panic("SMarketListBox::GetPrice: Invalid index");
    return Items[index].Price;
}

// PANZERS 0x644c70
const char* SMarketListBox::GetFileName(int index) const
{
    if (index < 0 || index >= ItemCount)
        Logger.g->Panic("SMarketListBox::GetFileName: Invalid index");
    return Str(Items[index].FileName);
}

// PANZERS 0x64ae80
int SMarketListBox::SetCurSel(int index)
{
    if (index < ItemCount && index >= -1) {
        int old = CurSel;
        CurSel = index;
        Update();
        return old;
    }
    return -2;
}

// PANZERS 0x64b310
int SMarketListBox::SetTopIndex(int index)
{
    if (index >= 0 && index < (ItemCount - 1 + Columns) / Columns) {
        int old = TopIndex;
        TopIndex = index;
        Update();
        return old;
    }
    return -1;
}

// PANZERS 0x644b80
void SMarketListBox::EnsureVisible(int index)
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
    SendAction(0x4c422, 0);
}

static int MarketIndexAt(const SMarketListBox* l, int x, int y)
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

// PANZERS 0x648e30
bool SMarketListBox::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_UP) {
        if (!Selectable) {
            if (TopIndex > 0) {
                SetTopIndex(TopIndex - 1);
                SendAction(0x4c422, 0);
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
                SendAction(0x4c422, 0);
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

// PANZERS 0x648f00
void SMarketListBox::OnMouseDown(int button, int x, int y, int shift)
{
    if (!Selectable)
        return;
    if (button == 1) {
        float now = (float)((double)Timer.GetTickValue() / 1000.0);  // 0x661800
        if (LastClickTime != 0.0f && now - LastClickTime <= 0.75f && CurSel >= 0) {   // DAT_007f2fcc
            int col = CurSel % Columns;
            int row = CurSel / Columns - TopIndex;
            if (LineHeight * row + 6 <= y && y < (row + 1) * LineHeight + 6
                && (ColumnWidth == 0 || (col * ColumnWidth + 6 <= x && x < (col + 1) * ColumnWidth + 6))) {
                LastClickTime = 0.0f;
                SendAction(0x4c423, CurSel);
                return;
            }
        }
        LastClickTime = (float)(int)now;
        int idx = MarketIndexAt(this, x, y);
        if (idx < ItemCount && idx >= -1) {
            int old = CurSel;
            CurSel = idx;
            Update();
            if (old != -2) {
                if (Concert)
                    Concert->PlaySound("menu/button_down.wav", -12.0f, 0, -1);
                SendAction(0x4c421, CurSel);
                return;
            }
        }
    }
    SDXWidget::OnMouseDown(button, x, y, shift);                   // 0x539b10
}

// PANZERS 0x6490a0
void SMarketListBox::OnMouseMove(int x, int y, int shift)
{
    if (!Selectable)
        return;
    Hover = MarketIndexAt(this, x, y);
    Update();
    SDXWidget::OnMouseMove(x, y, shift);                           // 0x539b20
}

// PANZERS 0x649110
void SMarketListBox::OnMouseOut()
{
    if (!Selectable)
        return;
    Hover = -1;
    Update();
    SDXWidget::OnMouseOut();
}

// PANZERS 0x649130
void SMarketListBox::OnMouseWheel(int delta, int x, int y, int keys)
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

// PANZERS 0x647c50
bool SMarketListBox::OnAction(SWidget* source, int action, int param)
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
    SendAction(0x4c422, 0);
    return true;
}

// PANZERS 0x64b4e0
void SMarketListBox::Update()
{
    if (BackFrame < 0)
        return;
    Board->ShowFrame(HighlightFrame, false);
    for (int r = 0; r < VisibleLines; ++r) {
        int x = 6;
        for (int c = 0; c < Columns; ++c, x += 0x80) {
            int idx = (TopIndex + r) * Columns + c;
            Board->SetBoxColor(BoxFrames[c][r], 0);                // board +0x3c
            if (idx == CurSel) {
                Board->ShowFrame(HighlightFrame, true);
                Board->MoveFrame(HighlightFrame, x, LineHeight * r + 6);
            }
            if (idx < ItemCount) {
                const SMarketListBoxItem& it = Items[idx];
                Board->SetText(NameFrames[c][r], HwFont(Font), Align, Str(it.Name));
                Board->SetSpriteGlyph(IconFrames[c][r], it.Font, it.Glyph);
                Board->ShowFrame(IconFrames[c][r], true);
                Board->SetTextColor(NameFrames[c][r], 0xffffff);
                if (it.Price == 0) {
                    Board->SetText(PriceFrames[c][r], HwFont(Font), 1, "");
                } else {
                    char buf[32];
                    sprintf(buf, "%d", it.Price);
                    Board->SetText(PriceFrames[c][r], HwFont(Font), 1, buf);
                }
                Board->SetTextColor(PriceFrames[c][r], it.Color);
            } else {
                Board->SetText(NameFrames[c][r], HwFont(Font), Align, "");
                Board->ShowFrame(IconFrames[c][r], false);
                Board->SetText(PriceFrames[c][r], HwFont(Font), 1, "");
            }
        }
    }
    if (HasScrollbar)
        Scrollbar.SetRange((ItemCount - 1 + Columns) / Columns, TopIndex);  // 0x540c80
}

// ===========================================================================
// SMarketCategoryButton, SMarketSlotButton, SMarketView
// ===========================================================================

// PANZERS 0x63f6f0
SMarketCategoryButton::SMarketCategoryButton() : CountFrame(-1), Full(false) {}

// PANZERS 0x64b220
void SMarketCategoryButton::Redraw()
{
    pz::SButton::Redraw();                                         // 0x537d60
    if (CountFrame < 0)
        return;
    unsigned int color;
    if (!Checked && !Pressed) {
        Board->MoveFrame(CountFrame, Width - 8, 7);
        color = 0xffc29418;
    } else {
        Board->MoveFrame(CountFrame, Width - 8, 5);
        color = 0xff000000;
    }
    if (Full)
        color = 0xffff0000;
    Board->SetTextColor(CountFrame, color);                        // board +0x28
}

// PANZERS 0x63f750
SMarketSlotButton::SMarketSlotButton()
    : Font(-1), SpriteFrame(-1), OverlayFrame(-1), PriceFrame(-1)
{
    ToolTipFeatureEnabled = false;
}

// PANZERS 0x649080
void SMarketSlotButton::OnMouseDown(int button, int x, int y, int shift)
{
    (void)x; (void)y; (void)shift;
    if (button == 1)
        SendAction(PZA_MARKET_SLOT, 0);
}

SMarketView::SMarketView() { ToolTipFeatureEnabled = false; }

// PANZERS 0x6401a0
SMarketView::~SMarketView()
{
    if (Logic) {
        delete Logic;                                              // 0x55fe00 + delete 0x318
        Logic = nullptr;
    }
    if (World) {
        delete World;                                              // vtbl +0x00(1): releases its scene
        World = nullptr;
    }
}

// PANZERS 0x5f5280 (SWorld; the world agents may take it over):
// an empty, flat map of w x h tiles with the one terrain layer `layer`.
static void WorldCreateEmptyMap(pz::SWorld* w, int tw, int th, const char* layer)
{
    w->TerrainW = tw;                                              // +0xdc
    w->BlockH = th * 4;                                            // +0x74fc
    w->TerrainH = th;                                              // +0xe0
    w->BlockW = tw * 4;                                            // +0x74f8
    w->BlockSize = th * 4 * tw * 4;                                // +0x74f4
    w->Terrain = pz::g_Scene->CreateTerrain(tw, th, 4);            // scene +0x64
    pz::STerrainBuffers b;
    memset(&b, 0, sizeof(b));
    if (w->Terrain)
        w->Terrain->Acquire(&b);                                   // terrain +0x00
    w->Heights = b.Heights;                                        // +0xe8
    w->AltHeights = (float*)b.Buffer74;                            // +0xec
    w->Diffuse = b.Diffuse;                                        // +0xf4
    w->Blend = b.Blend;                                            // +0xf8
    w->TileMap = b.BufferB0;                                       // +0xfc
    w->WaterHeights = (float*)b.Buffer8C;                          // +0x100
    w->BlockMap = (unsigned*)b.Buffer11c68;                        // +0x74ec
    w->BlockMap2 = b.Buffer11c6c;                                  // +0x74f0
    pz::BlockMap_MarkDirty(w, 0, 0, w->BlockW, w->BlockH, 1);      // 0x5ef380
    pz::SHdArray<pz::STerrainLayer>& l = w->Layers;                // +0x104
    if (l.Size == l.Max) {                                         // 0x5d88a0 (SDArray::Add)
        int n = l.Max < 0x10 ? 0x10 : (l.Max * 6) / 5;
        l.Array = (pz::STerrainLayer*)realloc(l.Array, n * sizeof(pz::STerrainLayer));
        memset((void*)(l.Array + l.Max), 0, (n - l.Max) * sizeof(pz::STerrainLayer));
        l.Max = n;
    }
    pz::STerrainLayer& t = l.Array[l.Size++];
    SetStr(&t.Name, layer);                                        // 0x52c320
    t.Attributes = 0;                                              // +0x10
    w->SetTerrainLayers(false);                                    // 0x608360
    w->ResetCamera();                                              // 0x5ecc20
}

// PANZERS 0x5f7a50 (SWorld): removes every unit and empties the unit heap.
static void WorldRemoveAllUnits(pz::SWorld* w)
{
    for (int i = 0; i < w->Units.Size; ++i)
        if (w->Units.IsLive(i))
            w->RemoveUnit(i);                                      // 0x5f8060
    w->Units.Size = 0;                                             // +0x4d8
    w->Units.Free = -1;                                            // +0x4e0
    w->Units.Count = 0;                                            // +0x4e8
    *(int*)((unsigned char*)w + 0x508) = 0;                        // +0x508
}

// PANZERS 0x5e5580 (SWorld): one 50 ms tick of the units (the market preview's
// world; the mission runs SGameLogic instead): interpolation state, model,
// visuals, then the range overlay of the one selected unit and the sound.
static void WorldTickUnits(pz::SWorld* w, pz::SIViewport* vp)
{
    int sel1 = -1, n1 = 0, sel2 = -1, n2 = 0;
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        pz::SUnit* u = w->Units.Array[i].Unit;
        u->StoreInterpolationState();                              // +0x16c
        u->RefreshModel();                                         // +0x3c
        u->UpdateVisuals(vp);                                      // +0x40
        if (u->_104 & 1) { ++n1; sel1 = i; }                       // +0x104
        if (u->_104 & 2) { ++n2; sel2 = i; }
    }
    int sel = n2 == 1 ? sel2 : n1 == 1 ? sel1 : -1;
    w->ShowUnitRange(sel);                                         // 0x5fee00
    if (Concert)
        Concert->Update(false);                                    // Concert +0x0c(0)
}

static pz::SIViewport* PreviewPort()
{
    pz::SIViewport* vp = pz::PzGepard()->GetViewport(0);           // Gepard +0x3c(0)
    return vp->GetSubportCount() > 1 ? vp->GetSubport(1) : nullptr;   // +0x5c(1)
}

static unsigned MarketNowMs()
{
    return (unsigned)((double)Timer.GetTickValue());               // ftol(0x661800() * 1000.0)
}

// PANZERS 0x6449a0
void SMarketView::Create()
{
    PZ_M3_TRACE("SMarketView::Create (0x6449a0)");
    SDXWidget::Create(0);                                          // 0x539a10
    World = new pz::SWorld(0);                                     // new 0x7538, 0x5d2f90(0)
    WorldCreateEmptyMap(World, 0x70, 0x70, "vasarlomenu");         // 0x5f5280
    World->Initialize();                                           // 0x5eec90
    World->SetCameraLimits(false);                                 // 0x5efb40(0)
    World->MoveCamera(-0.15f, 0.35f);                              // 0x5f4dc0
    World->SetCameraAngles(2.3561945f, -0.5235988f);               // 0x5f83b0 (3/4 pi, -pi/6)
    float d = 5.5f;                                                // 0x6093d0 (clamped)
    World->CamDist = (World->CamDistMin <= d && d <= World->CamDistMax) ? d :
                     (World->CamDistMin <= d ? World->CamDistMax : World->CamDistMin);
    World->CamSmoothTarget[0] = World->CamTarget[0];               // 0x5ef360
    World->CamSmoothTarget[1] = World->CamTarget[1];
    World->CamSmoothTarget[2] = World->CamTarget[2];
    World->CamSmoothDist = World->CamDist;
    if (pz::SIViewport* vp = PreviewPort()) {
        World->ComputeCamera(vp);                                  // 0x5ddc30
        vp->SetProjection(0.7853982f, 0.1f, 30.0f);                // sub +0x28
    }
    PzDrawFrameBox(this);                                          // 0x543d90
    TickNext = TickLast = MarketNowMs();                           // +0x64, +0x60
}

// PANZERS 0x64b890
void SMarketView::Update()
{
    if (!World)
        return;
    unsigned now = MarketNowMs();
    int ms = (int)(now - TickLast);
    TickLast = now;
    pz::SIViewport* vp = PreviewPort();
    while (TickNext < now) {
        if (vp)
            WorldTickUnits(World, vp);                             // 0x5e5580
        TickNext += 0x32;
    }
    if (pz::g_Scene)
        pz::g_Scene->AdvanceTime(ms);                              // scene +0x1c
    // HD board +0xa0(ms): the board animation clock (the SWINE board keeps
    // its own).
    if (vp)
        World->ComputeCamera(vp);                                  // 0x5ddc30
}

// ===========================================================================
// SMarket
// ===========================================================================

// PANZERS 0x63f2f0
SMarket::SMarket()
{
    PZ_M3_TRACE("SMarket::SMarket (0x63f2f0)");
    Mode = 0;
    Race = 0;
    CostFrame = PrestigeFrame = -1;
    Cost = 0;
    HqFont = InterfaceFont = -1;                                   // [0xe5], [0xe6] = -1
    UnitFont[0] = UnitFont[1] = UnitFont[2] = -1;                  // [0xe7..0xe9] = -1
    for (int i = 0; i < 4; ++i) {
        ArmorFrames[i] = StarFrames[i] = InfoRowFrames[i] = -1;
    }
    HeaderSprite = HeaderFrame = HpFrame = AmmoFrame = CargoFrame = XpFrame = ThermoFrame = -1;
    Slot1 = Slot2 = 0;                                             // [0x336], [0x337]
    SlotsAllowed = 0;
    Army = nullptr;
    ArmyCount = ArmyMax = 0;
    ArmyCategory = 0;
    for (int i = 0; i < 5; ++i)
        CategoryCount[i] = 0;
    WarehouseCategory = 0;
    Prestige = StartPrestige = 0;                                  // [0x51d], [0x51e]
    SelArmy = -1;
    pz::SUnitRegistry* r = pz::g_UnitRegistry;                     // DAT_00929a4c
    MaxAll = r ? r->AllUnitsMaxNumber : 0;                         // [0x919] +0x114
    MaxTanks = r ? r->TankMaxNumber : 0;                           // [0x91a] +0x118
    MaxArtillery = r ? r->ArtilleryMaxNumber : 0;                  // [0x91b] +0x11c
    MaxSupport = r ? r->SupportMaxNumber : 0;                      // [0x91c] +0x120
    // HD: in multiplayer (DAT_008f1a74) the tank limit becomes 8 or 10.
}

// PANZERS 0x63fa10
SMarket::~SMarket()
{
    PZ_M3_TRACE("SMarket::~SMarket (0x63fa10)");
    if (Subport[0] >= 0) {
        pz::SIViewport* vp = pz::PzGepard()->GetViewport(0);       // Gepard +0x3c(0)
        vp->DestroySubport(Subport[1]);                            // +0x58(+0x2460)
        vp->DestroySubport(Subport[0]);                            // +0x58(+0x245c)
        Subport[0] = Subport[1] = -1;
    }
    for (int i = 0; i < 3; ++i)
        if (UnitFont[i] >= 0)
            Board->ReleaseFont(UnitFont[i]);                       // board +0x80
    if (InterfaceFont >= 0)
        PzReleaseCustomFont(InterfaceFont);
    if (HqFont >= 0)
        Board->ReleaseFont(HqFont);
    if (Army) {                                                    // 0x63f830
        for (int i = 0; i < ArmyCount; ++i) {
            FreeStr(&Army[i].Vehicle);
            Army[i].Def.~SUnitDef();
        }
        free(Army);
        Army = nullptr;
    }
    ArmyCount = ArmyMax = 0;
    FreeStr(&SelUnit);
    FreeStr(&SelVehicle);
}

// 0x6407d0 helper: the 0x60-byte info icons (SSpecInfoWidget over the
// interface font) of the info panel.
static void MakeInfoIcon(SDXWidget* panel, SSpecInfoWidget* w, int x, int y, int ifFont, int glyph, int hqFont)
{
    panel->InsertChild(w);                                         // vtbl +0x54
    w->SetPosition(x, y, 0, 0);                                    // vtbl +0x08
    w->SetBackgroundSprite(ifFont, glyph, false, false);           // 0x539ba0
    w->Create(hqFont);                                             // 0x61e2d0
    w->SetVisible(false);                                          // vtbl +0x6c(0)
}

// PANZERS 0x6407d0
void SMarket::Create()
{
    PZ_M3_TRACE("SMarket::Create (0x6407d0)");
    LoadUnitDisplayNames();
    pz::SPanzersCampaign* camp = pz::g_Campaign;
    // campaign +0x11c ("#Deck Making") and multiplayer ("#Army Making") titles are not lifted.
    SFullScreenMenu::Create(Tx("Headquarters"));                   // 0x64bd50
    // Single player (DAT_008f1a74 == 0): Buy and Start Mission.
    InsertChild(&BuyButton);
    BuyButton.SetPosition(0x180, 0x2d2, 0, 0);
    BuyButton.Create(2, Tx("Buy"));
    InsertChild(&StartButton);
    StartButton.SetPosition(0x300, 0x2d2, 0, 0);
    StartButton.Create(2, Tx("Start Mission"));
    Mode = 0;                                                      // [0x17]

    HqFont = Board->LoadCustomFont("menu/headquarters_hq.tga", 0x1e,
                                   const_cast<SCustomGlyph*>(kHeadquartersGlyphs), Default);   // board +0x74
    InterfaceFont = PzLoadCustomFont("menu/panzers_interface_hq.tga", 0x18b, kInterfaceGlyphs);

    // Info panel (0x130, 0x181, 0x1b6 x 0x7d).
    InsertChild(&InfoPanel);
    InfoPanel.SetPosition(0x130, 0x181, 0x1b6, 0x7d);
    InfoPanel.SDXWidget::Create(0);
    PzDrawFrameBox(&InfoPanel);
    // The unit-state button (+0x404): the preview unit's state icon.
    InfoPanel.InsertChild(&UnitState);                             // vtbl +0x54
    UnitState.SetPosition(0x15e, 0x2d, 0, 0);                      // vtbl +0x08
    UnitState.Create(InterfaceFont, 0xda, 0x187, 0xdb);            // 0x61e310
    UnitState.Cursor = 0;                                          // 0x543970(0, -1)
    int pf = InfoPanel.GetFrame();
    ArmorFrames[0] = Board->CreateFrame(FT_TEXT, pf, 0x179, 0x1e, 0, 1);
    ArmorFrames[1] = Board->CreateFrame(FT_TEXT, pf, 0x14e, 0x41, 0, 1);
    ArmorFrames[2] = Board->CreateFrame(FT_TEXT, pf, 0x198, 0x41, 0, 1);
    ArmorFrames[3] = Board->CreateFrame(FT_TEXT, pf, 0x179, 100, 0, 1);
    HeaderSprite = Board->CreateFrame(FT_SPRITE, pf, 5, 5, 0, 1);
    PzSetSpriteGlyph(HeaderSprite, InterfaceFont, 0xf1);
    HeaderFrame = Board->CreateFrame(FT_TEXT, pf, 0x1e, 7, 0, 1);
    for (int i = 0; i < 4; ++i)
        StarFrames[i] = Board->CreateFrame(FT_SPRITE, pf, 0x171 + 0xf * i, 6, 0, 1);
    MakeInfoIcon(&InfoPanel, &HpIcon, 5, 0x19, InterfaceFont, 0xfa, HqFont);
    HpFrame = Board->CreateFrame(FT_TEXT, pf, 0x27, 0x1d, 0, 1);
    MakeInfoIcon(&InfoPanel, &AmmoIcon, 5, 0x2d, InterfaceFont, 0xfb, HqFont);
    AmmoFrame = Board->CreateFrame(FT_TEXT, pf, 0x27, 0x31, 0, 1);
    MakeInfoIcon(&InfoPanel, &CargoIcon, 5, 0x2d, InterfaceFont, 0xfc, HqFont);
    CargoFrame = Board->CreateFrame(FT_TEXT, pf, 0x27, 0x31, 0, 1);
    MakeInfoIcon(&InfoPanel, &XpIcon, 5, 0x41, InterfaceFont, 0xfd, HqFont);
    XpFrame = Board->CreateFrame(FT_TEXT, pf, 0x27, 0x45, 0, 1);
    Board->ShowFrame(XpFrame, false);
    MakeInfoIcon(&InfoPanel, &ThermoIcon, 5, 0x55, InterfaceFont, 0xfe, HqFont);
    ThermoFrame = Board->CreateFrame(FT_TEXT, pf, 0x27, 0x59, 0, 1);
    Board->ShowFrame(ThermoFrame, true);
    // The speed / weapon rows use menu/selection_hq.tga, which HD releases
    // at the end of this loop (board +0x80) while the rows still show it;
    // the recompile's board frees the glyphs on release, so it is kept.
    int selFont = Board->LoadCustomFont("menu/selection_hq.tga", 0x37,
                                        const_cast<SCustomGlyph*>(kSelectionGlyphs), Default);
    for (int i = 0, y = 0x1b; i < 4; ++i, y += 0x17) {
        InfoRowFrames[i] = Board->CreateFrame(FT_TEXT, pf, 0xa0, y + 1, 0, 1);
        Board->SetText(InfoRowFrames[i], g_PzFont[PZF_SANS14], 0, "Egyeb Info: 67");
        Board->ShowFrame(InfoRowFrames[i], false);
        MakeInfoIcon(&InfoPanel, &InfoRows[i], 0x82, y, selFont, 0x1d, selFont);
    }

    // Unit icon fonts (0x50 glyphs of 0x80 x 0x33, 8 per row).
    static SCustomGlyph unitGlyphs[0x50];
    for (int i = 0; i < 0x50; ++i) {
        unitGlyphs[i].X = (i % 8) << 7;
        unitGlyphs[i].Y = (i / 8) * 0x33;
        unitGlyphs[i].Width = 0x80;
        unitGlyphs[i].Height = 0x33;
    }
    Race = camp ? camp->Race : 0;                                  // 0x592a00
    if (Race == 0)
        UnitFont[0] = Board->LoadCustomFont("menu/headquarters_units_german_hq.tga", 0x50, unitGlyphs, Default);
    else if (Race == 2)
        UnitFont[2] = Board->LoadCustomFont("menu/headquarters_units_russian_hq.tga", 0x50, unitGlyphs, Default);
    else
        UnitFont[1] = Board->LoadCustomFont("menu/headquarters_units_allied_hq.tga", 0x50, unitGlyphs, Default);

    // My army: list (10, 100) and five tabs.
    InsertChild(&ArmyList);
    ArmyList.SetPosition(10, 100, 0x122, 0);
    ArmyList.Create(0, 9, 2, true, true);                          // 0x644130
    for (int i = 0, x = 0x16; i < 5; ++i, x += 0x35) {
        SMarketCategoryButton* b = &ArmyTabs[i];
        InsertChild(b);
        b->SetPosition(x, 0x36, 0, 0);
        b->Create(g_MenuControlsFont, 0x38 + i, 0x38 + i + 5, -1, -1);   // 0x537a80
        b->CountFrame = Board->CreateFrame(FT_TEXT, b->SpriteFrame, b->Width - 8, 5, 0, 0);
        b->Full = false;
    }
    ArmyTabs[0].SetChecked(true);                                  // 0x537df0(1)
    // Warehouse: list (0x2ea, 100) and four tabs.
    InsertChild(&Warehouse);
    Warehouse.SetPosition(0x2ea, 100, 0x10c, 0);
    Warehouse.Create(0, 9, 2, true, false);
    for (int i = 0, x = 0x306; i < 4; ++i, x += 0x35) {
        SMarketCategoryButton* b = &WarehouseTabs[i];
        InsertChild(b);
        b->SetPosition(x, 0x36, 0, 0);
        b->Create(g_MenuControlsFont, 0x39 + i, 0x39 + i + 5, -1, -1);
        b->CountFrame = Board->CreateFrame(FT_TEXT, b->SpriteFrame, b->Width - 8, 5, 0, 0);
        b->Full = false;
    }
    WarehouseTabs[1].SetChecked(true);                             // tanks (category 2)

    // The campaign's start army (+0x3c) into the army list (a crew squad in
    // a vehicle becomes "XX Crew Squad" + the vehicle).
    unsigned char* c = (unsigned char*)camp;
    int startCount = c ? *(int*)(c + 0x40) : 0;
    pz::SUnitDef* start = c ? *(pz::SUnitDef**)(c + 0x3c) : nullptr;
    for (int i = 0; i < startCount; ++i) {
        const pz::SUnitDef& d = start[i];
        const char* cls = Str(d.ClassName);
        int len = (int)strlen(cls);
        bool squad = len >= 5 && _stricmp(cls + len - 5, "Squad") == 0;
        if (d.StoredCount < 1) {
            if (squad) {
                int k = AddArmyItem();
                CopyUnitDef(&Army[k].Def, &d);
                if (pz::SPUnit* p = PUnit(cls))
                    Army[k].Price = p->Price;
                FreeStr(&Army[k].Vehicle);
                continue;
            }
            pz::SPUnit* p = PUnit(cls);
            if (p && p->StorageCapacity < 1 && !p->Repairer && !p->Supporter) {
                Logger.g->Log(1, "SMarket::Create: Ez az egyeg elfogveszni");
                continue;
            }
            int k = AddArmyItem();
            CopyUnitDef(&Army[k].Def, &d);
            if (p)
                Army[k].Price = p->Price;
            FreeStr(&Army[k].Vehicle);
        } else {
            const pz::SUnitDef& crew = d.StoredUnits[0];
            const char* ccls = Str(crew.ClassName);
            int clen = (int)strlen(ccls);
            if (clen >= 5 && _stricmp(ccls + clen - 5, "Squad") == 0) {
                int k = AddArmyItem();
                CopyUnitDef(&Army[k].Def, &crew);
                if (pz::SPUnit* p = PUnit(ccls))
                    Army[k].Price = p->Price;
                SetStr(&Army[k].Vehicle, cls);
            }
        }
    }
    ArmyCategory = 0;                                              // [0x346]
    FillArmyList();                                                // 0x6492d0
    Prestige = camp ? camp->Prestige : 0;                          // 0x591e60
    StartPrestige = camp ? camp->StartPrestige : 0;                // 0x592110
    SelArmy = -1;
    FreeStr(&SelUnit);
    FreeStr(&SelVehicle);
    int t1 = Board->CreateFrame(FT_TEXT, BackFrame, 0x9b, 0x12, 0, 0);
    Board->SetText(t1, g_PzFont[PZF_SANS21_SHADOW], 2, Tx("My army"));
    int t2 = Board->CreateFrame(FT_TEXT, BackFrame, 0x370, 0x12, 0, 0);
    Board->SetText(t2, g_PzFont[PZF_SANS21_SHADOW], 2, Tx("Warehouse"));

    // Two subports: the whole screen with the board only, the 3D preview
    // with the scene only (drawn over the board).
    pz::SIViewport* vp = pz::PzGepard()->GetViewport(0);           // Gepard +0x3c(0)
    if (vp->GetSubportCount() == 0) {                              // +0x60
        int wx, wy, ww, wh;
        GetWindowParent()->GetPosition(&wx, &wy, &ww, &wh);        // 0x5435b0 +0x04
        Subport[0] = vp->CreateSubport(0, 0, ww, wh);              // +0x54
        int px = (ww * 0x136) / 0x400, py = (wh * 0x3c) / 0x300;
        int pw = (ww * 0x1aa) / 0x400, ph = (wh * 0x13f) / 0x300;
#if PANZERS_MOD_WIDESCREEN
        // MOD_WIDESCREEN: the preview where the centred HQ draws its frame.
        {
            int dx = 0x136, dy = 0x3c, dw = 0x1aa, dh = 0x13f;
            if (ModWidescreenDesignRect(static_cast<SSuperWindow*>(GetWindowParent()), &dx, &dy, &dw, &dh)) {
                px = dx; py = dy; pw = dw; ph = dh;
            }
        }
#endif
        Subport[1] = vp->CreateSubport(px, py, pw, ph);
        vp->GetSubport(0)->SetDrawScene(false);                    // +0x5c(0) +0x80(0)
        vp->GetSubport(1)->SetDrawBoard(false);                    // +0x5c(1) +0x7c(0)
    }
    InsertChild(&View);
    View.SetPosition(0x130, 0x36, 0x1b6, 0x14b);
    View.Create();                                                 // 0x6449a0

    InsertChild(&CostBar);
    CostBar.SetPosition(0x130, 0x2a4, 0x1b6, 0x26);
    CostBar.SDXWidget::Create(0);
    PzDrawFrameBox(&CostBar);
    CostFrame = Board->CreateFrame(FT_TEXT, CostBar.GetFrame(), 0x1ab, 6, 0, 1);
    PrestigeFrame = Board->CreateFrame(FT_TEXT, CostBar.GetFrame(), 0xb, 6, 0, 1);
    char buf[64];
    _snprintf(buf, sizeof buf, Tx("Prestige: %d"), Prestige);
    Board->SetText(PrestigeFrame, g_PzFont[PZF_SANS21], 0, buf);

    // Equipment slots (0x130, 0x26f).
    InsertChild(&SlotPanel);
    SlotPanel.SetPosition(0x130, 0x26f, 0x1b6, 0x34);
    SlotPanel.SDXWidget::Create(0);
    PzDrawFrameBox(&SlotPanel);
    SlotPanel.SetVisible(false);
    for (int i = 0, x = 9; i < 8; ++i, x += 0x30) {
        SMarketSlotButton* s = &Slots[i];
        SlotPanel.InsertChild(s);
        s->SetPosition(x, 6, 0x30, 0x28);
        s->Font = HqFont;
        s->SDXWidget::Create(0);
        s->SpriteFrame = Board->CreateFrame(FT_SPRITE, s->BackFrame, 0, 0, 0, 1);
        s->OverlayFrame = Board->CreateFrame(FT_SPRITE, s->BackFrame, 0, 0, 0, 1);
        s->PriceFrame = Board->CreateFrame(FT_TEXT, s->BackFrame, 0x2d, 0x19, 0, 1);
        Board->SetSpriteGlyph(s->SpriteFrame, HqFont, 3 + 3 * i);
        Board->SetSpriteGlyph(s->OverlayFrame, -1, 0);
        s->SetVisible(false);
    }
    InsertChild(&ChangeVehicle);
    ChangeVehicle.SetPosition(0x18b, 0x271, 0, 0);
    ChangeVehicle.Create(2, Tx("Change vehicle"));
    ChangeVehicle.SetVisible(false);
    // [0x339] vehicle animation frame (type 5) at (0x98, 0x24): only for the
    // vehicle menu (not lifted).
    Slot1 = Slot2 = 0;
    WarehouseCategory = 2;                                         // [0x443]
    FillWarehouse();                                               // 0x6499a0

    InsertChild(&Description);
    Description.SetPosition(0x130, 0x1ff, 0x1b6, 0);
    Description.Create(0, 7, true, true, 0, false);                // 0x542380
    if (ArmyCount == 0)
        StartButton.SetEnable(false);                              // vtbl +0x70(0)

    // SInputDialog "Save Army" (+0x1480, multiplayer) and the "Leave
    // Market" box (+0x2070) are not created; the "Start Mission" box is.
    InsertChild(&StartBox);
    StartBox.Create(Tx("Start Mission"), "", PZ_MB_YESNO, true);   // 0x53e3b0
    StartBox.SetText(Tx("Are you sure?"), 0xd0d0d0);               // 0x53e910
    Cursor = 0;
    StartBox.SetVisible(false);
    Cursor = 0;
}

// PANZERS 0x640210
int SMarket::AddArmyItem()
{
    if (ArmyCount == ArmyMax) {
        int n = ArmyMax < 0x10 ? 0x10 : ArmyMax * 6 / 5;
        Army = (SMarketArmyItem*)realloc((void*)Army, n * sizeof(SMarketArmyItem));
        memset((void*)(Army + ArmyMax), 0, (n - ArmyMax) * sizeof(SMarketArmyItem));
        ArmyMax = n;
    }
    int i = ArmyCount++;
    new (&Army[i].Def) pz::SUnitDef();                             // the SDArray memsets; HD copies an SUnitDef in next
    return i;
}

// PANZERS 0x64a080
void SMarket::RemoveArmyItem(int index)
{
    if (index < 0 || index >= ArmyCount)
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", "SArmyItem", index, ArmyCount);
    FreeStr(&Army[index].Vehicle);                                 // 0x63fde0
    Army[index].Def.~SUnitDef();
    --ArmyCount;
    if (ArmyCount - index)
        memmove((void*)&Army[index], (void*)&Army[index + 1], (ArmyCount - index) * sizeof(SMarketArmyItem));
    memset((void*)&Army[ArmyCount], 0, sizeof(SMarketArmyItem));
}

// PANZERS 0x644f00
// 0 none, 1 infantry squad, 2 crewed vehicle (tank), 3 support vehicle,
// 4 artillery (class 0xb).
int SMarket::UnitCategory(const char* unit)
{
    pz::SPUnit* p = PUnit(unit);
    if (!p)
        return 0;
    int ct = p->ClassType;
    if (ct == 0 || ct == 0xb) {
        if (p->OnlyCrew)
            return 2;
        if (ct == 0xb)
            return 4;
    }
    if (ct == 0)
        return 3;
    if (ct == 5) {
        const char* n = Str(p->Name);
        int len = (int)strlen(n);
        if (len >= 5 && _stricmp(n + len - 5, "Squad") == 0)
            return 1;
    }
    return 0;
}

// PANZERS 0x644d80: the icon font by the unit's nation (+0x80).
int SMarket::IconFont(const char* unit) const
{
    pz::SPUnit* p = PUnit(unit);
    switch (p ? p->Race : 0) {
    case 2: case 4: return UnitFont[0];
    case 5: case 7: return UnitFont[2];
    default:        return UnitFont[1];
    }
}

// PANZERS 0x644fc0 (single-player branch)
bool SMarket::Available(const pz::SPUnit* p)
{
    pz::SPanzersCampaign* c = pz::g_Campaign;
    bool ok = false;
    if (c && c->GameMode == pz::PZ_GM_TUTORIAL) {
        if (p->MarketBuyFirst != 0 && p->MarketBuyLast != 0)
            return true;
        return p->MarketMulti != 3;
    }
    // Campaign: the mission number range (0x5920f0 "Mission number").
    STUB_LOG("SMarket 0x644fc0: campaign mission-number range not lifted");
    if (p->MarketBuyFirst == 0)
        return false;
    if (p->MarketBuyLast == 0)
        return false;
    ok = true;
    return ok;
}

// PANZERS 0x64b940
void SMarket::ColorPrices()
{
    for (int i = 0; i < Warehouse.ItemCount; ++i)
        Warehouse.Items[i].Color = Prestige < Warehouse.Items[i].Price ? 0xff0000 : 0xff00;
    Warehouse.Update();
}

// PANZERS 0x6492d0
void SMarket::FillArmyList()
{
    ArmyList.CurSel = -1;                                          // [0xda0] = -1
    ArmyList.ResetContent(0);                                      // 0x640620
    ArmyList.TopIndex = 0;                                         // [0xd90]
    ArmyList.Update();
    for (int i = 0; i < 5; ++i)
        CategoryCount[i] = 0;
    for (int i = 0; i < ArmyCount; ++i) {
        const char* unit = Army[i].Vehicle.size == 0 ? Str(Army[i].Def.ClassName) : Str(Army[i].Vehicle);
        int cat = UnitCategory(unit);
        CategoryCount[cat]++;
        if (ArmyCategory == 0 || ArmyCategory == cat) {
            pz::SPUnit* p = PUnit(unit);
            int k = ArmyList.AddItem(p ? Str(p->IniName68) : unit, unit, IconFont(unit),
                                     p ? p->MarketPicture : 0, 0xd0d0d0, 0);
            ArmyList.Items[k].Data = i;
            if (i == SelArmy && k < ArmyList.ItemCount && k > -2) {
                ArmyList.CurSel = k;
                ArmyList.Update();
            }
        } else if (i == SelArmy) {
            if (ArmyList.ItemCount > -1) {
                ArmyList.CurSel = -1;
                ArmyList.Update();
            }
            SelArmy = -1;
            FreeStr(&SelUnit);
            FreeStr(&SelVehicle);
            Slot1 = Slot2 = 0;
            BuyButton.SetVisible(false);
            LoadUnitInfo();
        }
    }
    char buf[32];
    struct { SMarketCategoryButton* b; int n; int max; bool slash; } tabs[5] = {
        { &ArmyTabs[0], ArmyCount, MaxAll, true },
        { &ArmyTabs[1], CategoryCount[1], 0, false },
        { &ArmyTabs[2], CategoryCount[2], MaxTanks, true },
        { &ArmyTabs[3], CategoryCount[3], MaxSupport, true },
        { &ArmyTabs[4], CategoryCount[4], MaxArtillery, true },
    };
    for (auto& t : tabs) {
        if (t.slash)
            sprintf(buf, "%d/%d", t.n, t.max);                     // "%d/%d" 0x8029c0
        else
            sprintf(buf, "%d", t.n);
        if (t.b->CountFrame != -1) {
            t.b->Full = t.slash && t.max <= t.n;
            Board->SetText(t.b->CountFrame, g_PzFont[PZF_SANS14], 1, buf);
            t.b->Redraw();                                         // vtbl +0x7c
        }
    }
    ArmyTabs[1].Full = false;
}

// PANZERS 0x6499a0 (single-player branch)
void SMarket::FillWarehouse()
{
    Warehouse.CurSel = -1;                                         // [0x1180] = -1
    Warehouse.ResetContent(0);
    Warehouse.TopIndex = 0;                                        // [0x1170]
    Warehouse.Update();
    pz::SUnitRegistry* r = pz::g_UnitRegistry;
    pz::SPanzersCampaign* c = pz::g_Campaign;
    bool anyRace = c && ((unsigned char*)c)[0x11c] != 0;
    for (int i = 0; r && i < r->Count; ++i) {
        pz::SUnitRegistryEntry& e = r->Entries[i];
        pz::SPUnit* p = e.Type;
        if (!p || e.InGame || p->HeroPicture != -1)
            continue;
        if (!(p->Side == Race || anyRace))
            continue;
        bool avail = Available(p);
        const char* unit = Str(e.Name);
        if (!avail || UnitCategory(unit) != WarehouseCategory)
            continue;
        if (WarehouseCategory == 2) {
            // A crewed vehicle is sold with its crew: "XX Crew Squad" + the vehicle.
            char crew[64];
            _snprintf(crew, sizeof crew, "%.3sCrew Squad", Str(p->Name));
            crew[sizeof crew - 1] = 0;
            pz::SPUnit* cp = PUnit(crew);
            int price = (cp ? cp->Price : 0) + p->Price;
            Warehouse.AddItem(Str(p->IniName68), Str(p->Name), IconFont(Str(p->Name)),
                              p->MarketPicture, 0xd0d0d0, price);
        } else {
            Warehouse.AddItem(Str(p->IniName68), Str(p->Name), IconFont(Str(p->Name)),
                              p->MarketPicture, 0xd0d0d0, p->Price);
        }
    }
    // qsort by price (0x6406f0)
    qsort(Warehouse.Items, Warehouse.ItemCount, sizeof(SMarketListBoxItem),
          [](const void* a, const void* b) -> int {
              int pa = ((const SMarketListBoxItem*)a)->Price, pb = ((const SMarketListBoxItem*)b)->Price;
              return pa < pb ? -1 : (pb < pa ? 1 : 0);
          });
    Warehouse.Update();
    ColorPrices();                                                 // 0x64b940
    if (ArmyList.ItemCount > -1) {
        ArmyList.CurSel = -1;
        ArmyList.Update();
    }
    SelArmy = -1;
    FreeStr(&SelUnit);
    FreeStr(&SelVehicle);
    Slot1 = Slot2 = 0;
    BuyButton.SetVisible(false);
    LoadUnitInfo();
}

// PANZERS 0x64ab50
void SMarket::SelectWarehouse(int index)
{
    if (index < 0)
        return;
    SetStr(&SelUnit, Warehouse.GetFileName(index));
    pz::SPUnit* p = PUnit(Str(SelUnit));
    if (!p || !p->OnlyCrew) {
        FreeStr(&SelVehicle);
    } else {
        SetStr(&SelVehicle, Str(SelUnit));
        char crew[64];
        _snprintf(crew, sizeof crew, "%.3sCrew Squad", Str(SelUnit));   // Mid(0, 3) + "Crew Squad"
        crew[sizeof crew - 1] = 0;
        SetStr(&SelUnit, crew);
    }
    SetButtonText(&BuyButton, Tx("Buy"));                          // 0x538a10
    BuyButton.SetVisible(true);
    Slot1 = Slot2 = 0;
    LoadUnitInfo();
}

// PANZERS 0x64a950
void SMarket::SelectArmy(int index)
{
    if (index < 0) {
        if (ArmyList.ItemCount > -1) {
            ArmyList.CurSel = -1;
            ArmyList.Update();
        }
        SelArmy = -1;
        FreeStr(&SelUnit);
        FreeStr(&SelVehicle);
        Slot1 = Slot2 = 0;
        BuyButton.SetVisible(false);
        LoadUnitInfo();
        return;
    }
    if (index >= ArmyList.ItemCount)
        Logger.g->Panic("SMarketListBox::GetItemData: Invalid index");
    ArmyList.CurSel = index;
    ArmyList.Update();
    if (Warehouse.ItemCount > -1) {
        Warehouse.CurSel = -1;
        Warehouse.Update();
    }
    SelArmy = ArmyList.Items[index].Data;
    if (SelArmy < 0 || SelArmy >= ArmyCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SArmyItem", SelArmy);
    SetStr(&SelUnit, Str(Army[SelArmy].Def.ClassName));
    SetStr(&SelVehicle, Str(Army[SelArmy].Vehicle));
    Slot1 = Army[SelArmy].Def.Slots[0];
    Slot2 = Army[SelArmy].Def.Slots[1];
    SetButtonText(&BuyButton, Tx("Sell"));
    BuyButton.SetVisible(true);
    LoadUnitInfo();
}

// PANZERS 0x6403b0 (single-player branch)
void SMarket::Buy()
{
    if (Warehouse.CurSel < 0 || Prestige < Cost || MaxAll <= ArmyCount)
        return;
    int cat = UnitCategory(SelVehicle.size ? Str(SelVehicle) : Str(SelUnit));
    if (cat == 2 && MaxTanks <= CategoryCount[2]) return;
    if (cat == 3 && MaxSupport <= CategoryCount[3]) return;
    if (cat == 4 && MaxArtillery <= CategoryCount[4]) return;
    int k = AddArmyItem();
    SMarketArmyItem& it = Army[k];
    SetStr(&it.Def.ClassName, Str(SelUnit));
    it.Def.Slots[0] = Slot1;                                       // +0x40
    it.Def.Slots[1] = Slot2;                                       // +0x44
    SetStr(&it.Vehicle, Str(SelVehicle));                          // +0x80
    FillArmyList();                                                // 0x6492d0
    Prestige -= Cost;
    ColorPrices();                                                 // 0x64b940
    LoadUnitInfo();
    StartButton.SetEnable(true);                                   // vtbl +0x70(1)
}

// PANZERS 0x64ad10 (single-player branch)
void SMarket::Sell()
{
    if (SelArmy < 0)
        return;
    RemoveArmyItem(SelArmy);                                       // 0x64a080
    Prestige += Cost;
    ColorPrices();
    int sel = ArmyList.CurSel;
    SelArmy = -1;
    FillArmyList();
    if (ArmyList.ItemCount <= sel)
        --sel;
    SelectArmy(sel);                                               // 0x64a950
    if (ArmyCount == 0)
        StartButton.SetEnable(false);
}

// PANZERS 0x6450c0 (see the file header for what is left out)
void SMarket::LoadUnitInfo()
{
    PZ_M3_TRACE("SMarket::LoadUnitInfo (0x6450c0)");
    char buf[256];
    int rank = 1;
    bool selling = SelArmy >= 0 && BuyButton.Visible && BuyButton.Text.buf &&
                   _stricmp(BuyButton.Text.buf, Tx("Sell")) == 0;
    int xp = 0;
    if (selling) {
        xp = (int)Army[SelArmy].Def.XP;                           // cvttss2si item +0xc
        pz::SUnitRegistry* r = pz::g_UnitRegistry;
        if (xp < r->XpLevel[0]) rank = 1;
        else if (xp < r->XpLevel[1]) rank = 2;
        else if (xp < r->XpLevel[2]) rank = 3;
        else rank = (r->XpLevel[3] <= xp) + 4;
    }
    Cost = 0;
    Board->SetText(CostFrame, g_PzFont[PZF_SANS21], 2, "");
    for (int i = 0; i < 4; ++i)
        Board->ShowFrame(ArmorFrames[i], false);
    // The preview: the world's units go (0x5f7a50, twice in HD) and the
    // selected unit (a crew squad inside its vehicle) is created at 56, 56.
    if (View.World) {
        WorldRemoveAllUnits(View.World);                           // 0x5f7a50
        WorldRemoveAllUnits(View.World);
    }
    UnitState.SetVisible(false);                                   // +0x404 vtbl +0x6c(0)
    if (View.World && SelUnit.size != 0) {                         // +0x388
        pz::SUnitDef def;                                          // 0x5cfb10
        def.Pos[0] = def.Pos[1] = 56.0f;                           // 0x42600000
        if (SelVehicle.size == 0) {                                // +0x390
            SetStr(&def.ClassName, Str(SelUnit));                  // 0x52c2c0
        } else {
            SetStr(&def.ClassName, Str(SelVehicle));
            pz::SUnitDef crew;
            SetStr(&crew.ClassName, Str(SelUnit));
            crew.Pos[0] = crew.Pos[1] = 56.0f;
            pz::SArmyArray stored = { nullptr, 0, 0 };
            pz::UnitDefCopy(pz::ArmyAdd(&stored), &crew);          // 0x560d10, 0x560290
            def.StoredUnits = stored.Array;
            def.StoredCount = stored.Size;
            def.StoredMax = stored.Max;
        }
        int unit = View.World->CreateUnit(&def);                   // 0x5e2da0
        Logger.g->Log(1, "PZM3 market: preview unit %d %s", unit, Str(def.ClassName));
        UnitState.SetUnit(unit, false);                            // 0x626c40(unit, 0)
        UnitState.SetVisible(true);                                // vtbl +0x6c(1)
    }
    for (int i = 0; i < 4; ++i) {
        InfoRows[i].SetVisible(false);
        Board->ShowFrame(InfoRowFrames[i], false);
    }
    HpIcon.SetVisible(false);
    Board->ShowFrame(HpFrame, false);
    AmmoIcon.SetVisible(false);
    Board->ShowFrame(AmmoFrame, false);
    CargoIcon.SetVisible(false);
    Board->ShowFrame(CargoFrame, false);
    XpIcon.SetVisible(false);
    Board->ShowFrame(XpFrame, false);
    ThermoIcon.SetVisible(false);
    Board->ShowFrame(ThermoFrame, false);
    for (int i = 0; i < 4; ++i)
        Board->ShowFrame(StarFrames[i], false);
    Board->ShowFrame(HeaderFrame, false);
    Description.Clear();
    Board->ShowFrame(HeaderSprite, false);

    if (SelUnit.size == 0) {
        for (int i = 0; i < 8; ++i)
            Slots[i].SetVisible(false);
        SlotPanel.SetVisible(false);
        ChangeVehicle.SetVisible(false);
    } else {
        pz::SPUnit* p = PUnit(Str(SelUnit));
        pz::SPUnit* show = p;                                      // the unit the panel describes
        Board->ShowFrame(HeaderFrame, true);
        Board->ShowFrame(HeaderSprite, true);
        if (SelVehicle.size)
            show = PUnit(Str(SelVehicle));
        if (show) {
            Board->SetText(HeaderFrame, g_PzFont[PZF_SANS14], 0, Str(show->IniName70));
            Description.AddLine(*(const char**)((unsigned char*)show + 0x78) ?
                                *(const char**)((unsigned char*)show + 0x78) : "", 0xd0d0d0);  // +0x78 description
            // Armour values of a vehicle (proto +0xa0..+0xac).
            const float* armor = &show->FrontArmor;
            for (int i = 0; i < 4; ++i) {
                if (armor[i] > 0.0f) {
                    sprintf(buf, "%g", (double)armor[i]);
                    Board->SetText(ArmorFrames[i], g_PzFont[PZF_SANS14], 0, buf);
                    Board->ShowFrame(ArmorFrames[i], true);
                }
            }
        }
        HpIcon.SetVisible(true);
        Board->ShowFrame(HpFrame, true);
        if (p) {
            if (p->ClassType == 5) {
                // A squad: member HP + the rank bonus, times the members.
                const char* member = *(const char**)((unsigned char*)p + 0x140);
                unsigned char count = *((unsigned char*)p + 0x13c);
                pz::SPUnit* m = member ? PUnit(member) : nullptr;
                if (!m)
                    Logger.g->Panic("SMarket::LoadUnitInfo() Nincs member");
                int bonus = pz::g_UnitRegistry->SquadHpLevel[rank > 5 ? 4 : rank - 1];
                sprintf(buf, "%g", (double)(((float)bonus + m->HP) * (float)count));
            } else {
                sprintf(buf, "%g", (double)p->HP);
            }
            Board->SetText(HpFrame, g_PzFont[PZF_SANS14], 0, buf);
            Cost += p->Price;
        }
        // Equipment slots.
        for (int i = 0; i < 8; ++i)
            Board->SetSpriteGlyph(Slots[i].SpriteFrame, HqFont, 3 + 3 * i);
        if (Race == 1)
            Board->SetSpriteGlyph(Slots[0].SpriteFrame, HqFont, 0);
        static const int kSlotFlag[8] = { 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xbd, 0xba, 0xbc };
        static const char* kSlotHint[8] = { "Grenade", "Molotov Coctail", "Magnetic Mine", "Explosives",
                                            "Tank Mine", "Binoculars", "Boat", "Mine Detector" };
        (void)kSlotHint;   // SWidget hints (0x543bf0) are not stored by the recompile
        for (int i = 0; i < 8; ++i) {
            bool has = p && *((unsigned char*)p + kSlotFlag[i]) != 0;
            Slots[i].SetVisible(has);
            sprintf(buf, "%d", SlotPrice(i + 1));
            Board->SetText(Slots[i].PriceFrame, g_PzFont[PZF_SANS14], 1, buf);
        }
        SlotsAllowed = (p && p->UnitType == 0xe) ? 0 : 2;
        int chosen[2] = { Slot1, Slot2 };
        for (int s = 0; s < 2; ++s) {
            int j = chosen[s];
            if (j <= 0)
                continue;
            if (SlotsAllowed < s + 1 || !Slots[j - 1].Visible) {
                Logger.g->Panic("SMarket::LoadUnitInfo: %s has an equipment it shouldn't have!", Str(SelUnit));
            }
            Board->SetSpriteGlyph(Slots[j - 1].SpriteFrame, HqFont, j * 3 + 1);
            Cost += SlotPrice(j);
            if (Race == 1 && j == 1)
                Board->SetSpriteGlyph(Slots[0].SpriteFrame, HqFont, 1);
        }
        SlotPanel.SetVisible(!(p && p->UnitType == 0xe));
        ChangeVehicle.SetVisible(p && p->UnitType == 0xe && SelArmy >= 0);
        ChangeVehicle.SetEnable(SelArmy < 0 || SelVehicle.size != 0 || CategoryCount[2] < MaxTanks);

        if (SelVehicle.size) {
            pz::SPUnit* v = PUnit(Str(SelVehicle));
            if (v) {
                Cost += v->Price;
                sprintf(buf, "%g", (double)v->HP);
                Board->SetText(HpFrame, g_PzFont[PZF_SANS14], 0, buf);
                sprintf(buf, "%d", (int)v->Thermostat);            // "%d" of +0x9c
                Board->SetText(ThermoFrame, g_PzFont[PZF_SANS14], 0, buf);
                Board->ShowFrame(ThermoFrame, true);
                ThermoIcon.SetVisible(true);
            }
        }
        // Rank stars and XP.
        for (int i = 0; i < 4; ++i) {
            PzSetSpriteGlyph(StarFrames[i], InterfaceFont, 0xf3);
            Board->ShowFrame(StarFrames[i], true);
        }
        for (int i = 0; i < rank - 1 && i < 4; ++i)
            PzSetSpriteGlyph(StarFrames[i], InterfaceFont, 0xf2);
        sprintf(buf, "%d", xp);
        Board->SetText(XpFrame, g_PzFont[PZF_SANS14], 0, buf);
        XpIcon.SetVisible(true);
        Board->ShowFrame(XpFrame, true);
        // Speed (driver 0 max speed +0x08) and the weapon rows.
        pz::SPUnit* d = show;
        if (d && d->PDrivers.Size > 0) {
            float speed = *(float*)((unsigned char*)d->PDrivers.Array[0] + 8);
            speed = speed * 3.6f / 100.0f * 20.0f / 0.005f;       // 0x7f5a68, 0x7ee558, 0x7f35d8, 0x7f5994
            InfoRows[0].SetVisible(true);
            Board->ShowFrame(InfoRowFrames[0], true);
            sprintf(buf, "%s %g", Tx("Speed:"), (double)speed);
            Board->SetText(InfoRowFrames[0], g_PzFont[PZF_SANS14], 0, buf);
        }
        int rows = 1;
        for (int g = 0; d && g < d->PGunners.Size && rows < 4; ++g) {
            const unsigned char* gp = (const unsigned char*)d->PGunners.Array[g];
            int type = *(const int*)(gp + 0x24);
            static const int kGlyph[4] = { 0x1e, 0x1f, 0x35, 0x36 };
            if (type >= 0 && type <= 3)
                (&InfoRows[rows])->SetGlyph(kGlyph[type]);   // 0x6260e0
            float dmg = *(const float*)(gp + 0x58);
            if (dmg == -1.0f)                                      // DAT_007f5a98
                continue;
            InfoRows[rows].SetVisible(true);
            Board->ShowFrame(InfoRowFrames[rows], true);
            if (SelVehicle.size == 0) {
                // HD 0x64741a (no vehicle, +0x390 == 0): one row. A squad
                // shows its member's first weapon (SquadMemberName +0x140),
                // with the squad bonus of the rank (registry +0x7c) unless
                // the unit is a crew (+0x44 == 0xe); others the first weapon.
                const pz::SPUnit* member = nullptr;
                if (p && p->ClassType == 5) {
                    const pz::SPPanzersSquadUnit* sq = static_cast<const pz::SPPanzersSquadUnit*>(p);
                    if (sq->SquadMemberName.size != 0) {
                        member = PUnit(Str(sq->SquadMemberName));
                        if (member && member->PGunners.Size <= 0)
                            member = nullptr;
                    }
                }
                if (member) {
                    dmg = *(const float*)((const unsigned char*)member->PGunners.Array[0] + 0x58);
                    if (rank >= 2 && p->UnitType != 0xe) {
                        float bonus = pz::g_UnitRegistry->SquadDamageBonus[rank - 1];   // +0x80..+0x8c
                        sprintf(buf, "%s %g + %.02f", Tx("Damage:"), (double)dmg, (double)(bonus * dmg - dmg));
                    } else {
                        sprintf(buf, "%s %g", Tx("Damage:"), (double)dmg);
                    }
                } else {
                    dmg = *(const float*)((const unsigned char*)d->PGunners.Array[0] + 0x58);   // 0x64773e
                    sprintf(buf, "%s %g", Tx("Damage:"), (double)dmg);
                }
                Board->SetText(InfoRowFrames[rows], g_PzFont[PZF_SANS14], 0, buf);
                ++rows;
                break;
            }
            // HD 0x646fc8 (a vehicle chosen for the crew): every weapon,
            // with the crew bonus of the rank (registry +0x90) for a crew.
            if (rank >= 2 && p && p->UnitType == 0xe) {
                float bonus = pz::g_UnitRegistry->CrewDamageBonus[rank - 1];   // +0x94..+0xa0
                sprintf(buf, "%s %g + %.02f", Tx("Damage:"), (double)dmg, (double)(bonus * dmg - dmg));
            } else {
                sprintf(buf, "%s %g", Tx("Damage:"), (double)dmg);
            }
            Board->SetText(InfoRowFrames[rows], g_PzFont[PZF_SANS14], 0, buf);
            ++rows;
        }
        if (d && d->PGunners.Size > 0) {
            const unsigned char* g0 = (const unsigned char*)d->PGunners.Array[0];
            if (*(const int*)(g0 + 0x24) == 0 || d->ClassType == 5)
                strcpy(buf, Tx("Unlimited"));
            else
                sprintf(buf, "%d", *(const int*)(g0 + 0x48));
            Board->SetText(AmmoFrame, g_PzFont[PZF_SANS14], 0, buf);
            Board->ShowFrame(AmmoFrame, true);
            AmmoIcon.SetVisible(true);
        } else if (d && d->Cargo > 0) {
            sprintf(buf, "%d", d->Cargo);
            Board->SetText(CargoFrame, g_PzFont[PZF_SANS14], 0, buf);
            Board->ShowFrame(CargoFrame, true);
            AmmoIcon.SetVisible(false);
            CargoIcon.SetVisible(true);
        }
    }
    // Cost and prestige lines.
    sprintf(buf, Tx("Cost: %d"), Cost);
    Board->SetText(CostFrame, g_PzFont[PZF_SANS21], 1, buf);
    bool buying = BuyButton.Visible && BuyButton.Text.buf && _stricmp(BuyButton.Text.buf, Tx("Buy")) == 0;
    Board->SetTextColor(CostFrame, buying ? (Prestige < Cost ? 0xff0000 : 0xff00) : 0xffffff);
    sprintf(buf, Tx("Prestige: %d"), Prestige);
    Board->SetText(PrestigeFrame, g_PzFont[PZF_SANS21], 0, buf);
}

// PANZERS 0x64a180 (multiplayer army file; not used in single player)
void SMarket::SaveArmy(int p1)
{
    STUB_LOG("SMarket::SaveArmy (0x64a180)");
    PZ_M3_TRACE("SMarket::SaveArmy (0x64a180)");
    (void)p1;
}

// PANZERS 0x64b0a0
void SMarket::SetArmyCategory(int category)
{
    for (int i = 0; i < 5; ++i)
        ArmyTabs[i].SetChecked(category == i);
    ArmyCategory = category;
    FillArmyList();
}

// PANZERS 0x647d00 (single-player cases)
bool SMarket::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("SMarket::OnAction (0x647d00)");
    if (action == 0x42541) {                                       // a tab pressed
        for (int i = 0; i < 5; ++i) {
            if (source == &ArmyTabs[i]) {
                for (int j = 0; j < 5; ++j)
                    ArmyTabs[j].SetChecked(i == j);
                ArmyCategory = i;
                FillArmyList();
                return true;
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (source == &WarehouseTabs[i]) {
                for (int j = 0; j < 4; ++j)
                    WarehouseTabs[j].SetChecked(i == j);
                WarehouseCategory = i + 1;
                FillWarehouse();
                return true;
            }
        }
    }
    if (source == &ChangeVehicle && action == 0x42542) {
        STUB_LOG("SMarketVehicleMenu (0x648c20): the vehicle menu is not lifted");
        return true;
    }
    if (source == &StartBox) {
        if (action == PZA_MSGBOX_YES) {                            // 0x4d582
            // The army in campaign form (+0xd00): a crew squad goes into
            // its vehicle's stored units.
            pz::SArmyArray army = { nullptr, 0, 0 };              // +0xd00
            for (int i = 0; i < ArmyCount; ++i) {
                pz::SUnitDef* d = pz::ArmyAdd(&army);              // 0x560d10
                if (Army[i].Vehicle.size != 0) {
                    SetStr(&d->ClassName, Str(Army[i].Vehicle));
                    pz::SArmyArray crew = { nullptr, 0, 0 };
                    pz::UnitDefCopy(pz::ArmyAdd(&crew), &Army[i].Def);
                    d->StoredUnits = crew.Array;
                    d->StoredCount = crew.Size;
                    d->StoredMax = crew.Max;
                } else {
                    pz::UnitDefCopy(d, &Army[i].Def);              // 0x560290
                }
            }
            if (pz::g_Campaign)
                pz::g_Campaign->SetArmy(&army, true);              // 0x5971b0(+0xd00, 1)
            pz::ArmyFree(&army);                                   // 0x51de90
            SendAction(PZA_MARKET_START, 0);                       // 0x4d542
        } else {
            StartBox.SetVisible(false);
        }
        return true;
    }
    if (source == &ArmyList) {
        if (action == 0x4c421)
            SelectArmy(ArmyList.CurSel);                           // 0x64a950
        return true;
    }
    if (source == &Warehouse) {
        if (action == 0x4c421) {
            SetButtonText(&BuyButton, Tx("Buy"));
            BuyButton.SetVisible(true);
            if (ArmyList.CurSel >= 0) {
                ArmyList.CurSel = -1;
                ArmyList.Update();
            }
            SelArmy = -1;
            FreeStr(&SelUnit);
            FreeStr(&SelVehicle);
            Slot1 = Slot2 = 0;
            BuyButton.SetVisible(false);
            LoadUnitInfo();
            SelectWarehouse(Warehouse.CurSel);                     // 0x64ab50
        }
        return true;
    }
    if (action == PZA_MARKET_SLOT) {
        for (int j = 1; j <= 8; ++j) {
            if (source != &Slots[j - 1])
                continue;
            bool buyMode = BuyButton.Text.buf && _stricmp(BuyButton.Text.buf, Tx("Buy")) == 0 &&
                           BuyButton.Visible && Warehouse.CurSel >= 0;
            bool sellMode = BuyButton.Text.buf && _stricmp(BuyButton.Text.buf, Tx("Sell")) == 0 &&
                            BuyButton.Visible && SelArmy >= 0;
            if (buyMode) {
                if (Slot1 == j) { Slot1 = Slot2; Slot2 = 0; }
                else if (Slot2 == j) Slot2 = 0;
                else if (Slot1 == 0 && SlotsAllowed > 0) Slot1 = j;
                else if (Slot2 != 0 || SlotsAllowed < 2) return true;
                else Slot2 = j;
            } else if (sellMode) {
                if (Slot1 == j) {
                    Prestige += SlotPrice(Slot1);
                    Slot1 = Slot2;
                    Slot2 = 0;
                } else if (Slot2 == j) {
                    Prestige += SlotPrice(Slot2);
                    Slot2 = 0;
                } else if (Slot1 == 0 && SlotsAllowed > 0 && SlotPrice(j) <= Prestige) {
                    Prestige -= SlotPrice(j);
                    Slot1 = j;
                } else if (Slot2 != 0 || SlotsAllowed < 2 || Prestige < SlotPrice(j)) {
                    return true;
                } else {
                    Prestige -= SlotPrice(j);
                    Slot2 = j;
                }
                Army[SelArmy].Def.Slots[0] = Slot1;
                Army[SelArmy].Def.Slots[1] = Slot2;
                ColorPrices();
            }
            LoadUnitInfo();
            return true;
        }
        return false;
    }
    if (action == 0x42542) {
        if (source == &BuyButton) {
            if (BuyButton.Text.buf && _stricmp(BuyButton.Text.buf, Tx("Sell")) == 0)
                Sell();                                            // 0x64ad10
            else
                Buy();                                             // 0x6403b0
            return true;
        }
        if (source == &StartButton) {
            StartBox.SetVisible(true);                             // vtbl +0x6c(1); 0x64afb0 reshapes the 3D view
            return true;
        }
    }
    // Not handled here: SendAction starts at this widget, so the market's own
    // 0x4d541 / 0x4d542 go on to SSuperWindow.
    return false;
}

// PANZERS 0x64b380 (the multiplayer connection polling is not lifted)
void SMarket::Update()
{
    View.Update();                                                 // +0x2b4 vtbl +0x78
}
