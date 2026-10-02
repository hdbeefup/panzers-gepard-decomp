// window/saveloadlistbox.h
// SSaveLoadListBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SSAVELOADLISTBOX_H
#define WINDOW_SSAVELOADLISTBOX_H

#include "darray.h"
#include "dxwidget.h"
#include "hdbeefup.h"
#include "sliderv.h"
#include <windows.h>

struct SSaveLoadListBoxItem {
    char ItemText1[261];
    char ItemText2[261];
    char ItemText3[261];
    char ItemText4[261];
    char DifficultyText[261];
    SYSTEMTIME time;
    unsigned int Color;
};

struct SSaveLoadListBox : SDXWidget {
    int Font;
    int RowHeight;
    int VisibleTextLines;
    int MarginLeft;
    int MarginRight;
    int MarginTop;
    int MarginBottom;
    int SliderMarginLeft;
    int SliderMarginRight;
    int SliderMarginTop;
    int SliderMarginBottom;
    int MaxInnerTextLines;
    int *TextLineFrames1;
    int *TextLineFrames2;
    int *TextLineFrames3;
    int *DifficultyTextFrames;
    int *TextLineBackFrames;
    SSliderV Slider;
    bool SelectAble;
    int CurActive;
    int LastClickTime;
    int WidthMiddle;
    SSaveLoadListBoxItem m_ListBoxItem;
    SDArray<SSaveLoadListBoxItem> m_ListBoxItems;
    int m_nCurSel;
    int m_nTopIndex;
#ifdef HD_BULK_DELETE_SAVES
    SDArray<bool> m_MultiSelected;
    int m_nAnchorSel;
#endif

    SSaveLoadListBox();
    ~SSaveLoadListBox() override;

    bool OnAction(SWidget *sender, int action, int param) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseWheel(int button, int x, int y, int delta) override;
    void Update() override;

    const char *AddItem(const char *lpszItem1, const char *lpszItem2, const char *lpszItem3, const char *lpszItem4, char *difficultyText, _SYSTEMTIME time, unsigned int dwColor);
    void Create(int font, unsigned char NumberOfTextLines, bool bSelectAble);
    int DeleteItem(int nIndex);
    int EnsureIndexIsVisible(int nIndex, bool bRolling);
    unsigned int GetColor(int nIndex);
    int GetCount();
    int GetCurSel();
    _SYSTEMTIME GetItemData(int nIndex);
    unsigned int GetText(int nIndex, char *lpszBuffer1, char *lpszBuffer2, char *lpszBuffer3, char *lpszBuffer4, char *difficultyText, int nSizeOfBuffer);
    unsigned int GetTextLen(int a2, int nIndex);
    int GetTopIndex();
    void QSort(int InitialRow);
    void ResetContent();
    unsigned int SetColor(int nIndex, unsigned int dwColor);
    int SetCurSel(int nSelect);
    int SetItemData(int nIndex, _SYSTEMTIME time);
    void SetMargin(int left, int right, int top, int bottom);
    void SetSliderMargin(int left, int right, int top, int bottom);
    unsigned int SetText(int nIndex, char *lpszBuffer1, char *lpszBuffer2, char *lpszBuffer3, char *lpszBuffer4, char *difficultyText);
    int SetTopIndex(int nIndex);
#ifdef HD_BULK_DELETE_SAVES
    int GetMultiSelectCount();
    bool IsMultiSelected(int i);
    void ClearMultiSelection();
#endif
};

#endif // WINDOW_SSAVELOADLISTBOX_H
