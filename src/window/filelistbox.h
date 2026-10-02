// window/filelistbox.h
// SFileListBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SFILELISTBOX_H
#define WINDOW_SFILELISTBOX_H

#include "darray.h"
#include "dxwidget.h"
#include "sliderv.h"

struct SFileListBoxItem {
    char VisibleText[261];
    char Filename[261];
    unsigned int ItemData;
    unsigned int Color;
};

struct SFileListBox : SDXWidget {
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
    int CurActive;
    int RowHeight;
    SFileListBoxItem m_ListBoxItem;
    SDArray<SFileListBoxItem> m_ListBoxItems;
    int m_nCurSel;
    int m_nTopIndex;

    SFileListBox();
    ~SFileListBox() override;

    bool OnAction(SWidget *sender, int action, int param) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseWheel(int button, int x, int y, int delta) override;
    void Update() override;

    void Create(int font, int NumberOfTextLines, bool bSelectAble, int bBackGround, int bScrollbars);
    int DeleteItem(int nIndex);
    int EnsureIndexIsVisible(int nIndex, bool bRolling);
    unsigned int GetColor(int nIndex);
    int GetCount();
    int GetCurSel();
    unsigned int GetFileName(int nIndex, char *lpszBuffer, int nSizeOfBuffer);
    unsigned int GetFileNameLen(int nIndex);
    unsigned int GetItemData(int nIndex);
    unsigned int GetText(int nIndex, char *lpszBuffer, int nSizeOfBuffer);
    unsigned int GetTextLen(int nIndex);
    int GetTopIndex();
    void QSort(int InitialRow);
    void ResetContent();
    unsigned int SetColor(int nIndex, unsigned int dwColor);
    int SetCurSel(int nSelect);
    unsigned int SetFileName(int nIndex, char *lpszBuffer);
    unsigned int SetItemData(int nIndex, unsigned int dwItemData);
    void SetMargin(int left, int right, int top, int bottom);
    void SetSliderMargin(int left, int right, int top, int bottom);
    unsigned int SetText(int nIndex, char *lpszBuffer);
    int SetTopIndex(int nIndex);
    const char *AddItem(const char *lpszVisibleText, const char *lpszFileName, unsigned int dwColor, unsigned int dwItemData);
};

#endif // WINDOW_SFILELISTBOX_H
