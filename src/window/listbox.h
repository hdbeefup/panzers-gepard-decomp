// window/listbox.h
// SListBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SLISTBOX_H
#define WINDOW_SLISTBOX_H

#include "darray.h"
#include "dxwidget.h"
#include "sliderv.h"

struct SListBoxItem {
    char ItemText[261];
    unsigned int ItemData;
    unsigned int Color;
};

struct SListBox : SDXWidget {
    int Font;
    int VisibleTextLines;
    int MaxInnerTextLines;
    int MarginLeft;
    int MarginRight;
    int MarginTop;
    int MarginBottom;
    int SliderMarginLeft;
    int SliderMarginRight;
    int SliderMarginTop;
    int SliderMarginBottom;
    int *TextLineFrames;
    int *TextLineBackFrames;
    bool scrollbars;
    bool SelectAble;
    SSliderV Slider;
    bool nobreaklines;
    int aligntext;
    bool alignverticalcenter;
    int CurActive;
    int RowHeight;
    int LastClickTime;
    SListBoxItem m_ListBoxItem;
    SDArray<SListBoxItem> m_ListBoxItems;
    int m_nCurSel;
    int m_nTopIndex;

    SListBox();
    ~SListBox() override;

    bool OnAction(SWidget *sender, int action, int param) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseWheel(int button, int x, int y, int delta) override;
    void Update() override;

    int AddItem(const char *lpszItem, unsigned int dwColor, unsigned int dwItemData);
    void Create(int font, unsigned char NumberOfTextLines, bool bSelectAble, char bBackGround, int bScrollbars, int bNoBreakLines, int bAlignText, int bAlignVerticalCenter);
    int DeleteItem(int nIndex);
    int EnsureIndexIsVisible(int nIndex, bool bRolling);
    unsigned int GetColor(int nIndex);
    int GetCount();
    int GetCurSel();
    unsigned int GetItemData(int nIndex);
    unsigned int GetText(int nIndex, char *lpszBuffer, int nSizeOfBuffer);
    unsigned int GetTextLen(int nIndex);
    int GetTopIndex();
    int GetVisibleTextLines();
    bool IsIndexVisible(int nIndex);
    void ResetContent();
    unsigned int SetColor(int nIndex, unsigned int dwColor);
    int SetCurSel(int nSelect);
    unsigned int SetItemData(int nIndex, unsigned int dwItemData);
    void SetMargin(int left, int right, int top, int bottom);
    void SetSliderMargin(int left, int right, int top, int bottom);
    unsigned int SetText(int nIndex, char *lpszBuffer);
    int SetTopIndex(int nIndex);
};

#endif // WINDOW_SLISTBOX_H
