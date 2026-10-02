// window/droplist.h
// SDropList header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SDROPLIST_H
#define WINDOW_SDROPLIST_H

#include "darray.h"
#include "dxwidget.h"

struct SDropListItem {
    char ItemText[261];
    unsigned int ItemData;
    bool ItemEnabled;
};

struct SDropList : SDXWidget {
    int Font;
    int ButtonFont;
    int *DropLineFrames;
    int DropFrame;
    int DropBoxFrame;
    int DropBlackFrame;
    bool Dropped;
    bool Dropable;
    unsigned int DroppedBackgroundColor;
    int ButtonFrame;
    int TextFrame;
    int FontHeight;
    bool Active;
    bool Pressed;
    SDropListItem m_ListBoxItem;
    SDArray<SDropListItem> m_ListBoxItems;
    int m_nCurSel;
    int m_nCurActive;
    int m_nTopIndex;

    SDropList();
    ~SDropList() override;

    bool OnKeyDown(int keycode, bool repeat = false) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void SetFocus() override;
    void Update() override;

    int AddItem(int a2, const char *lpszItem, bool itemEnabled, unsigned int dwItemData);
    void Create(int a2, int font, bool background, unsigned int droppedbackground_color);
    int DeleteItem(int nIndex);
    void Drop();
    int GetCount();
    int GetCurSel();
    unsigned int GetItemData(int nIndex);
    unsigned int GetText(int nIndex, char *lpszBuffer, int nSizeOfBuffer);
    unsigned int GetTextLen(int nIndex);
    int GetTopIndex();
    bool IsItemEnabled(int nIndex);
    void ResetContent();
    void SetActive(bool active);
    int SetCurActive(int nSelect);
    int SetCurSel(int nSelect);
    void SetDropable(bool dropable);
    unsigned int SetItemData(int nIndex, unsigned int dwItemData);
    int SetItemEnabled(int nIndex, bool enabled);
    unsigned int SetText(int nIndex, char *lpszBuffer);
    int SetTopIndex(int nIndex);
};

#endif // WINDOW_SDROPLIST_H
