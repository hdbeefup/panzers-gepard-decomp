// src/panzers/pzwidgets.h
// Panzers' own option widgets (HD PANZERS.exe): slider, check box, drop list,
// scroll bar and list box, plus the 9-slice box helper they share.
//
// The SWINE classes of the same names (src/window) draw SWINE remaster skins
// (menu/rabbit_arrows_medium.png, csuszka_gomb.png, ...) that the Panzers
// paks do not contain, and their glyph indices, sounds and action codes
// differ. These lifts draw from the Panzers controls atlas
// (menu/controls_hq.tga, g_MenuControlsFont) and menu/widgets/*.tga, so they
// live in namespace pz to keep the HD class names without clashing with the
// SWINE ones. As with SComplexButton (mainmenu.h), each class derives from
// the SWINE SDXWidget for behaviour; the HD field offsets are given per field.

#ifndef PANZERS_PZWIDGETS_H
#define PANZERS_PZWIDGETS_H

#include "dxwidget.h"
#include "string2.h"

// Action codes sent by these widgets (SWidget::SendAction 0x543930).
enum PzWidgetAction {
    PZA_CHECKBOX_DOWN    = 0x42541,   // same codes as SComplexButton
    PZA_CHECKBOX_CLICK   = 0x42542,
    PZA_CHECKBOX_OVER    = 0x42543,
    PZA_DROPLIST_SELECT  = 0x444c1,   // param = new selection
    PZA_LISTBOX_SELECT   = 0x4c421,   // param = new selection
    PZA_LISTBOX_SCROLLED = 0x4c422,
    PZA_LISTBOX_DBLCLICK = 0x4c423,   // param = selection
    PZA_SCROLL_UP        = 0x53421,   // param = rows
    PZA_SCROLL_DOWN      = 0x53422,   // param = rows
    PZA_SCROLL_TO        = 0x53423,   // param = top row
    PZA_SLIDER_LEFT      = 0x53481,
    PZA_SLIDER_RIGHT     = 0x53482,
    PZA_SLIDER_CHANGED   = 0x53484,   // param = new position
};

// HD board +0x5c (0x6c4fb0): move a frame and each of its ancestors to the
// end of its parent's child list, so it draws over its siblings.
void PzBringFrameToFront(int frame);
// HD board +0x8c (0x6c5260): a font's line height.
int PzGetFontHeight(int font);
// HD 0x543d90: a 9-slice frame (controls glyphs 0x2f..0x37) over a widget.
void PzDrawFrameBox(SWidget* w);

namespace pz {

// HD SSliderH, vftable 0x7f3514, 0x98 bytes (ctor 0x540ef0).
struct SSliderH : SDXWidget {
    int  SliderFont;        // 0x58 menu/widgets/slider_button_hq.tga
    int  ButtonFont;        // 0x5c menu/widgets/arrows_medium_hq.tga
    int  SliderFrame;       // 0x60
    int  SliderBox;         // 0x64
    int  ButtonLeftFrame;   // 0x68
    int  ButtonRightFrame;  // 0x6c
    int  FirstClickTimer;   // 0x70
    int  TickTimer;         // 0x74
    int  NumberOfFixPos;    // 0x78
    int  SliderPos;         // 0x7c
    int* Ticks;             // 0x80
    bool PressedLeft;       // 0x84
    bool PressedRight;      // 0x85
    bool ActiveLeft;        // 0x86
    bool ActiveRight;       // 0x87
    bool KnobDown;          // 0x88
    int  KnobX;             // 0x8c
    int  KnobDelta;         // 0x90
    int  KnobDownX;         // 0x94

    SSliderH();                                                      // 0x540ef0
    ~SSliderH() override;                                            // 0x540f70

    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x5412a0
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x5414f0
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x541460
    void OnMouseOver() override;                                     // +0x34 0x5414e0
    void OnMouseOut() override;                                      // +0x38 0x5414d0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x541240
    void OnTimer(int id, unsigned int elapsed) override;             // +0x50 0x541620
    void Update() override;                                          // +0x78 0x5416d0

    void Create(int x, int y, int w, int numberOfFixPos, int pos);   // 0x541040
    int  GetValue() const { return SliderPos; }                      // 0x541230
    void SetValue(int pos);                                          // 0x5416b0
};

// HD SCheckBox, vftable 0x7f262c, 0x70 bytes (ctor 0x537e20).
struct SCheckBox : SDXWidget {
    SString Text;           // 0x58
    int  TextFrame;         // 0x60
    int  BoxFrame;          // 0x64
    int  _68;               // 0x68 (-1)
    bool Over;              // 0x6c
    bool Pressed;           // 0x6d
    bool Checked;           // 0x6e

    SCheckBox();                                                     // 0x537e20
    ~SCheckBox() override;                                           // 0x537ea0

    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x537f90
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x538050
    void OnMouseOver() override;                                     // +0x34 0x538000
    void OnMouseOut() override;                                      // +0x38 0x537fe0
    void Update() override;                                          // +0x78 0x538130

    void Create();                                                   // 0x537f20
    void SetText(const char* text);                                  // 0x538110
    bool GetCheck() const { return Checked; }                        // 0x537f70
    void SetCheck(bool checked);                                     // 0x5380f0
};

// HD SDropList item (SDropListItem, stride 0xc).
struct SDropListItem {
    SString      Text;      // +0x00
    unsigned int Data;      // +0x08
};

// HD SDropList, vftable 0x7f27f4, 0x8c bytes (ctor 0x538bb0).
struct SDropList : SDXWidget {
    int  Font;              // 0x58 (2)
    int  ItemHeight;        // 0x5c (0x18)
    bool Dropable;          // 0x60
    bool Over;              // 0x61
    bool Dropped;           // 0x62
    int  CurSel;            // 0x64
    int  HoverSel;          // 0x68
    int  MainFrame;         // 0x6c
    int  TextFrame;         // 0x70
    int* ItemFrames;        // 0x74
    int* ItemTextFrames;    // 0x78
    int  BottomFrame;       // 0x7c
    SDropListItem* Items;   // 0x80 SDArray {array, size, maxsize}
    int  ItemCount;         // 0x84
    int  ItemMax;           // 0x88

    SDropList();                                                     // 0x538bb0
    ~SDropList() override;                                           // 0x538c90

    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x539370
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x539550
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x539460
    void OnMouseOver() override;                                     // +0x34 0x539500
    void OnMouseOut() override;                                      // +0x38 0x5394e0
    void Update() override;                                          // +0x78 0x5397f0

    void Create();                                                   // 0x538f20
    int  AddItem(const char* text, unsigned int data);               // 0x538dd0
    void ResetContent();                                             // 0x5396a0
    int  GetCount() const { return ItemCount; }                      // 0x539280
    int  GetCurSel() const { return CurSel; }                        // 0x539290
    unsigned int GetItemData(int index) const;                       // 0x5392a0
    int  SetCurSel(int index);                                       // 0x5396f0
    void ToggleDrop();                                               // 0x539020

private:
    void FreeItems();                                                // 0x538c20
};

// HD SScrollbar, vftable 0x7f348c, 0x98 bytes (ctor 0x540630).
struct SScrollbar : SDXWidget {
    int  KnobTopFrame;      // 0x58
    int  KnobMidFrame;      // 0x5c
    int  KnobBottomFrame;   // 0x60
    int  UpFrame;           // 0x64
    int  DownFrame;         // 0x68
    int  FirstClickTimer;   // 0x6c
    int  TickTimer;         // 0x70
    bool PressedUp;         // 0x74
    bool PressedDown;       // 0x75
    bool ActiveUp;          // 0x76
    bool ActiveDown;        // 0x77
    bool Dragging;          // 0x78
    int  PageSize;          // 0x7c
    int  Pos;               // 0x80
    int  Range;             // 0x84
    int  KnobY;             // 0x88
    int  KnobH;             // 0x8c
    int  DragOffset;        // 0x90

    SScrollbar();                                                    // 0x540630
    ~SScrollbar() override;                                          // 0x540690

    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x540910
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x540b60
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x540a30
    void OnMouseOver() override;                                     // +0x34 0x540b50
    void OnMouseOut() override;                                      // +0x38 0x540b40
    void OnTimer(int id, unsigned int elapsed) override;             // +0x50 0x540bf0
    void Update() override;                                          // +0x78 0x540ca0

    void Create(int x, int y, int h, int pageSize);                  // 0x540770
    void SetRange(int range, int pos);                               // 0x540c80
};

// HD SListBoxItem (SGenericListBox<SListBoxItem>, stride 0x18).
struct SListBoxItem {
    SString      Text;      // +0x00
    SString      Text2;     // +0x08
    unsigned int Data;      // +0x10
    unsigned int Color;     // +0x14
};

// HD SListBox (over SGenericListBox<SListBoxItem>), vftable 0x7f2e1c,
// 0x134 bytes (ctor 0x53bdc0 over 0x53bd10).
struct SListBox : SDXWidget {
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
    SListBoxItem* Items;    // 0x114 SDArray {array, size, maxsize}
    int  ItemCount;         // 0x118
    int  ItemMax;           // 0x11c
    int  Font;              // 0x120
    int* TextFrames;        // 0x124
    int* BoxFrames;         // 0x128
    int  Align;             // 0x12c
    unsigned int SelColor;  // 0x130 (0x80808080)
    SString Hint;           // HD SWidget-level hint text (0x543bf0)

    SListBox();                                                      // 0x53bdc0
    ~SListBox() override;                                            // 0x53be80

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x53c6d0
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x53c7a0
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x53c920
    void OnMouseWheel(int delta, int x, int y, int keys) override;   // +0x30 0x53c9b0
    void OnMouseOut() override;                                      // +0x38 0x53c990
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x53c620
    void Update() override;                                          // +0x78 0x53cc70

    void Create(int font, int lines, bool background, bool scrollbars,
                int align, bool selectable);                         // 0x53c260
    int  AddItem(const char* text, const char* text2, unsigned int color,
                 unsigned int data);                                 // 0x53c020
    void ResetContent();                                             // 0x53c0c0(0)
    int  GetCount() const { return ItemCount; }
    int  GetCurSel() const { return CurSel; }
    int  SetCurSel(int index);                                       // 0x53caf0
    int  SetTopIndex(int index);                                     // 0x53cc00
    void EnsureVisible(int index);                                   // 0x53c3c0
    const char* GetItemText(int index) const;                        // 0x53c580
    void SetHint(const char* text);                                  // 0x543bf0

private:
    void GenericCreate(int lineHeight, int lines, int columnWidth, int columns,
                       bool selectable, bool background, bool scrollbars); // 0x53c1c0
    void FreeItems();                                                // 0x53bdf0
};

} // namespace pz

#endif // PANZERS_PZWIDGETS_H
