// window/playerlistbox.h
// SPlayerListBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SPLAYERLISTBOX_H
#define WINDOW_SPLAYERLISTBOX_H

#include "darray.h"
#include "dxwidget.h"
#include "sliderv.h"

#ifndef USERSTATUS_DEFINED
#define USERSTATUS_DEFINED
enum UserStatus : unsigned char {
    Available = 0,
    InLobby = 1,
    Playing = 2,
    Away = 3,
};
#endif

struct SPlayerListBoxItem {
    UserStatus status;
    char PlayerName[261];
    unsigned int Color;
};

struct SPlayerListBox : SDXWidget {
    int Font;
    int VisibleTextLines;
    int MaxInnerTextLines;
    int *TextLineBackFrames;
    int *StatusFrame;
    int *ReadyFrame;
    int *PlayerNameFrame;
    int RowHeight;
    SSliderV Slider;
    bool SelectAble;
    int StateFont;
    int CurActive;
    int LastClickTime;
    SPlayerListBoxItem m_ListBoxItem;
    SDArray<SPlayerListBoxItem> m_ListBoxItems;
    int m_nCurSel;
    int m_nTopIndex;

    SPlayerListBox();
    ~SPlayerListBox() override;

    bool OnAction(SWidget *sender, int action, int param) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseWheel(int button, int x, int y, int delta) override;
    void Update() override;

    int AddItem(UserStatus status, const char *PlayerName, unsigned int Color);
    void Create(int numberofrows, int statuswidth, int playernamewidth);
    int DeleteItem(int nIndex);
    int DeletePlayer(const char *PlayerName);
    int EnsureIndexIsVisible(int nIndex, bool bRolling);
    unsigned int GetColor(int nIndex);
    int GetCount();
    int GetCurSel();
    int GetPlayerIndex(const char *PlayerName);
    const char *GetPlayerName(int nIndex);
    UserStatus GetStatus(int nIndex);
    int GetTopIndex();
    void QSort(int InitialRow);
    void ResetContent();
    unsigned int SetColor(int nIndex, unsigned int dwColor);
    int SetCurSel(int nSelect);
    int SetPlayerName(const char *oldPlayerName, const char *newPlayerName);
    int SetTopIndex(int nIndex);
    int UpdateItem(int nIndex, UserStatus status, const char *PlayerName, unsigned int Color);
};

#endif // WINDOW_SPLAYERLISTBOX_H
