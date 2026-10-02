// src/stubs/stub_window.cpp
// Member-function stubs for gameplay classes that imported window/ code
// references but whose bodies lived in SWINE gameplay code:
//   SMenuBackGroundView — game/menubackgroundview.cpp (class declared locally
//                         in window/dxwidget.cpp; replicated here, keep in sync)
//   MatchInfo           — network/matchmaking.cpp (declared in
//                         window/matchlistbox.h)
// Each body is a minimal stand-in, not a copy. See docs/IMPORT_NOTES.md.

#include <windows.h>
#include "stub_log.h"
#include "dxwidget.h"
#include "scaler.h"
#include "matchlistbox.h"

struct SWorld;

// Must match the declaration in window/dxwidget.cpp exactly (ODR).
struct SMenuBackGroundView : SDXWidget {
  SWorld *World;
  unsigned int LastUpdate;
  unsigned int LastServerUpdate;
  SScaler MenuBackgroundScaler;
  SScaler PanelBackgroundScaler;

  SMenuBackGroundView();
  ~SMenuBackGroundView() override;

  void Update() override;

  void Create(int a2);
  void SetPosition(int x, int y, int width, int height);
  void SetScalerPosition();
  void LoadMap();
};

SMenuBackGroundView::~SMenuBackGroundView()
{
    STUB_LOG("SMenuBackGroundView::~SMenuBackGroundView");
}

void SMenuBackGroundView::Update()
{
    STUB_LOG("SMenuBackGroundView::Update");
}

static void FreeSString(SString& s)
{
    if (s.buf)
        operator delete[](s.buf);
    s.buf = nullptr;
    s.size = 0;
}

MatchInfo::MatchInfo()
    : id(0), hostPort(0), numPlayers(0), maxPlayers(0),
      clientVersion(0), gameType(0), isClosed(0)
{
    STUB_LOG("MatchInfo::MatchInfo");
}

MatchInfo::~MatchInfo()
{
    STUB_LOG("MatchInfo::~MatchInfo");
    FreeSString(name);
    FreeSString(hostName);
    FreeSString(hostAddress);
    FreeSString(hostPrivateAddress);
    FreeSString(hostGuid);
}

MatchInfo& MatchInfo::operator=(const MatchInfo& src)
{
    STUB_LOG("MatchInfo::operator=");
    if (this != &src) {
        id = src.id;
        name = src.name;
        hostName = src.hostName;
        hostAddress = src.hostAddress;
        hostPrivateAddress = src.hostPrivateAddress;
        hostGuid = src.hostGuid;
        hostPort = src.hostPort;
        numPlayers = src.numPlayers;
        maxPlayers = src.maxPlayers;
        clientVersion = src.clientVersion;
        gameType = src.gameType;
        isClosed = src.isClosed;
    }
    return *this;
}
