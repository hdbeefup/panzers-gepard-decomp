// window/matchlistbox.h
// SMatchListBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SMATCHLISTBOX_H
#define WINDOW_SMATCHLISTBOX_H

#include "darray.h"
#include "dxwidget.h"
#include "sliderv.h"
#include "string2.h"
#include "swineversion.h"

#ifndef MATCHINFO_DEFINED
#define MATCHINFO_DEFINED
struct MatchInfo {
    int id;
    SString name;
    SString hostName;
    SString hostAddress;
    SString hostPrivateAddress;
    SString hostGuid;
    int hostPort;
    unsigned char numPlayers;
    unsigned char maxPlayers;
    int clientVersion;
    unsigned char gameType;
    unsigned char isClosed;

    MatchInfo();
    ~MatchInfo();
    MatchInfo &operator=(const MatchInfo &src);
};
#endif

struct SFilter {
    int Status;
    int ActPlayers;
    int GameType;
    SVersion Version;
    int On;
    bool Changed;
};

struct SMatchListBoxItem {
    bool Status;
    char GameName[261];
    char Map[261];
    int NumPlayers;
    int MaxPlayers;
    char GameType[261];
    SVersion Version;
    unsigned int ServerCode;
    MatchInfo matchInfo;
    unsigned int Color;

    ~SMatchListBoxItem();
    SMatchListBoxItem &operator=(const SMatchListBoxItem &other);
};

struct SMatchListBox : SDXWidget {
    int Font;
    int VisibleTextLines;
    int MaxInnerTextLines;
    int *TextLineBackFrames;
    int *StatusFrame;
    int *GameNameFrame;
    int *PlayerFrame;
    int *GameTypeFrame;
    int *MapFrame;
    int *VersionFrame;
    SSliderV Slider;
    bool SelectAble;
    SFilter Filter;
    int StateFont;
    int CurActive;
    int LastClickTime;
    int RowHeight;
    SMatchListBoxItem m_ListBoxItem;
    SDArray<int> m_GoodWhenFilteredItems;
    SDArray<SMatchListBoxItem> m_ListBoxItems;
    int m_nCurSel;
    int m_nTopIndex;

    SMatchListBox();
    ~SMatchListBox() override;

    bool OnAction(SWidget *sender, int action, int param) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseWheel(int button, int x, int y, int delta) override;
    void Update() override;

    int AddItem(bool Status, const char *GameName, const char *Map, int NumPlayers, int MaxPlayers, const char *GameType, SVersion Version, unsigned int ServerCode, const MatchInfo *matchInfo, unsigned int Color);
    void Create();
    int DeleteItem(int nIndex);
    int EnsureIndexIsVisible(int nIndex, bool bRolling = false);
    int GetCount();
    int GetCountWithoutFiltering();
    int GetCurSel();
    char *GetGameName(int nIndex);
    const MatchInfo *GetMatchInfo(int nIndex);
    int GetMaxPlayers(int nIndex);
    unsigned int GetServerCode(int nIndex);
    bool GetStatus(int nIndex);
    int GetTopIndex();
    SVersion GetVersion(int nIndex);
    char GoodWhenFiltered(int Index);
    void ResetContent();
    void ResetFilter();
    int SetCurSel(int nSelect);
    void SetFilter(bool Status, int ActPlayers, int GameType, SVersion Version);
    void SetGoodWhenFilteredItems();
    int SetTopIndex(int nIndex);
};

#endif // WINDOW_SMATCHLISTBOX_H
