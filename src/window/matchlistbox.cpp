// window/matchlistbox.cpp
// Match browser list box
// Decompiled from: gameSplit/slistbox.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "matchlistbox.h"
#include "logger.h"
#include "timer.h"

// Classes: SMatchListBox
// Function count: 33

//----- (0048A110) --------------------------------------------------------

SMatchListBox::SMatchListBox()

{
  this->Filter.Version = SVersion();
  this->m_ListBoxItem.Version = SVersion();
  this->m_ListBoxItem.matchInfo.name.buf = 0;
  this->m_ListBoxItem.matchInfo.name.size = 0;
  this->m_ListBoxItem.matchInfo.hostName.buf = 0;
  this->m_ListBoxItem.matchInfo.hostName.size = 0;
  this->m_ListBoxItem.matchInfo.hostAddress.buf = 0;
  this->m_ListBoxItem.matchInfo.hostAddress.size = 0;
  this->m_ListBoxItem.matchInfo.hostPrivateAddress.buf = 0;
  this->m_ListBoxItem.matchInfo.hostPrivateAddress.size = 0;
  this->m_ListBoxItem.matchInfo.hostGuid.buf = 0;
  this->m_ListBoxItem.matchInfo.hostGuid.size = 0;
  this->m_GoodWhenFilteredItems.array = 0;
  this->m_GoodWhenFilteredItems.size = 0;
  this->m_GoodWhenFilteredItems.maxsize = 0;
  this->m_ListBoxItems.array = 0;
  this->m_ListBoxItems.size = 0;
  this->m_ListBoxItems.maxsize = 0;
  this->m_nCurSel = -1;
  this->m_nTopIndex = 0;
  this->MaxInnerTextLines = 3000;
  this->Filter.Status = 0;
  this->Filter.ActPlayers = 0;
  this->Filter.GameType = 0;
  this->Filter.Version = SVersion(0);
  this->Filter.On = 0;
  this->Filter.Changed = 0;
  this->SetGoodWhenFilteredItems();
  this->StateFont = -1;
  this->CurActive = -1;
  this->LastClickTime = 0;
}

//----- (0048A320) --------------------------------------------------------

SMatchListBox::~SMatchListBox()

{
  int size;
  int v4;
  int v5;
  int maxsize;
  int *TextLineBackFrames;
  int *StatusFrame;
  int *GameNameFrame;
  int *MapFrame;
  int *PlayerFrame;
  int *GameTypeFrame;
  int *VersionFrame;
  SMatchListBoxItem *array;
  this->SetCurSel(-1);
  size = this->m_ListBoxItems.size;
  if ( size && !this->m_ListBoxItems.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  v4 = 0;
  if ( size > 0 )
  {
    v5 = 0;
    do
    {
      this->m_ListBoxItems.array[v5].matchInfo.~MatchInfo();
      ++v4;
      ++v5;
    }
    while ( v4 < this->m_ListBoxItems.size );
  }
  maxsize = this->m_ListBoxItems.maxsize;
  this->m_ListBoxItems.size = 0;
  if ( maxsize < 0 )
  {
    array = this->m_ListBoxItems.array;
    this->m_ListBoxItems.maxsize = 0;
    this->m_ListBoxItems.array = (SMatchListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 876 * maxsize);
  this->m_GoodWhenFilteredItems.Clear();
  this->m_nTopIndex = 0;
  this->Update();
  if ( this->BackFrame >= 0 )
  {
    TextLineBackFrames = this->TextLineBackFrames;
    if ( TextLineBackFrames )
    {
      delete[] TextLineBackFrames;
      this->TextLineBackFrames = 0;
    }
    StatusFrame = this->StatusFrame;
    if ( StatusFrame )
    {
      delete[] StatusFrame;
      this->StatusFrame = 0;
    }
    GameNameFrame = this->GameNameFrame;
    if ( GameNameFrame )
    {
      delete[] GameNameFrame;
      this->GameNameFrame = 0;
    }
    MapFrame = this->MapFrame;
    if ( MapFrame )
    {
      delete[] MapFrame;
      this->MapFrame = 0;
    }
    PlayerFrame = this->PlayerFrame;
    if ( PlayerFrame )
    {
      delete[] PlayerFrame;
      this->PlayerFrame = 0;
    }
    GameTypeFrame = this->GameTypeFrame;
    if ( GameTypeFrame )
    {
      delete[] GameTypeFrame;
      this->GameTypeFrame = 0;
    }
    VersionFrame = this->VersionFrame;
    if ( VersionFrame )
    {
      delete[] VersionFrame;
      this->VersionFrame = 0;
    }
  }
  Board->ReleaseFont(this->StateFont);
  // Clean up m_ListBoxItems array
  {
    int i = 0;
    if ( this->m_ListBoxItems.size > 0 )
    {
      int j = 0;
      do
      {
        this->m_ListBoxItems.array[j].matchInfo.~MatchInfo();
        ++i;
        ++j;
      }
      while ( i < this->m_ListBoxItems.size );
    }
    if ( this->m_ListBoxItems.array )
    {
      free(this->m_ListBoxItems.array);
      this->m_ListBoxItems.array = 0;
    }
  }
  if ( this->m_GoodWhenFilteredItems.array )
  {
    free(this->m_GoodWhenFilteredItems.array);
    this->m_GoodWhenFilteredItems.array = 0;
  }
  this->m_ListBoxItem.matchInfo.~MatchInfo();
  // base destructor called automatically
  // base destructor called automatically
}

//----- (0048A560) --------------------------------------------------------

SMatchListBoxItem::~SMatchListBoxItem()

{
  this->matchInfo.~MatchInfo();
}

//----- (0048A610) --------------------------------------------------------

SMatchListBoxItem &SMatchListBoxItem::operator=(const SMatchListBoxItem &__that)

{
  int v3;
  char *GameName_ptr;
  int v5;
  char v6;
  char *Map_ptr;
  int v8;
  char v9;
  char *GameType_ptr;
  int v11;
  char v12;
  const char *v13;
  const char *buf;
  const char *v15;
  const char *v16;
  const char *v17;
  v3 = 261;
  GameName_ptr = this->GameName;
  this->Status = __that.Status;
  v5 = (const char *)&__that - (char *)this;
  do
  {
    v6 = (GameName_ptr)[v5];
    *(GameName_ptr) = v6;
    GameName_ptr++;
    --v3;
  }
  while ( v3 );
  Map_ptr = this->Map;
  v8 = 261;
  do
  {
    v9 = Map_ptr[v5];
    *Map_ptr++ = v9;
    --v8;
  }
  while ( v8 );
  GameType_ptr = this->GameType;
  v11 = 261;
  this->NumPlayers = __that.NumPlayers;
  this->MaxPlayers = __that.MaxPlayers;
  do
  {
    v12 = GameType_ptr[v5];
    *GameType_ptr++ = v12;
    --v11;
  }
  while ( v11 );
  v13 = "";
  this->Version = __that.Version;
  buf = "";
  this->ServerCode = __that.ServerCode;
  this->matchInfo.id = __that.matchInfo.id;
  if ( __that.matchInfo.name.buf )
    buf = __that.matchInfo.name.buf;
  this->matchInfo.name = (char *)buf;
  v15 = "";
  if ( __that.matchInfo.hostName.buf )
    v15 = __that.matchInfo.hostName.buf;
  this->matchInfo.hostName = (char *)v15;
  v16 = "";
  if ( __that.matchInfo.hostAddress.buf )
    v16 = __that.matchInfo.hostAddress.buf;
  this->matchInfo.hostAddress = (char *)v16;
  v17 = "";
  if ( __that.matchInfo.hostPrivateAddress.buf )
    v17 = __that.matchInfo.hostPrivateAddress.buf;
  this->matchInfo.hostPrivateAddress = (char *)v17;
  if ( __that.matchInfo.hostGuid.buf )
    v13 = __that.matchInfo.hostGuid.buf;
  this->matchInfo.hostGuid = (char *)v13;
  this->matchInfo.hostPort = __that.matchInfo.hostPort;
  this->matchInfo.numPlayers = __that.matchInfo.numPlayers;
  this->matchInfo.maxPlayers = __that.matchInfo.maxPlayers;
  this->matchInfo.clientVersion = __that.matchInfo.clientVersion;
  this->matchInfo.gameType = __that.matchInfo.gameType;
  this->matchInfo.isClosed = __that.matchInfo.isClosed;
  this->Color = __that.Color;
  return *this;
}

//----- (0048A7E0) --------------------------------------------------------

int SMatchListBox::AddItem(bool Status, const char *GameName, const char *Map, int NumPlayers, int MaxPlayers, const char *GameType, SVersion Version, unsigned int ServerCode, const MatchInfo *matchInfo, unsigned int Color)

{
  SMatchListBoxItem *p_m_ListBoxItem;
  int size;
  int v14;
  int v15;
  int v16;
  int maxsize;
  int v18;
  SMatchListBoxItem *v19;
  int v20;
  int v21;
  int v22;
  p_m_ListBoxItem = &this->m_ListBoxItem;
  this->m_ListBoxItem.Status = Status;
  strncpy(this->m_ListBoxItem.GameName, GameName, 0x104u);
  strncpy(this->m_ListBoxItem.Map, Map, 0x104u);
  this->m_ListBoxItem.NumPlayers = NumPlayers;
  this->m_ListBoxItem.MaxPlayers = MaxPlayers;
  strncpy(this->m_ListBoxItem.GameType, GameType, 0x104u);
  this->m_ListBoxItem.Version = Version;
  this->m_ListBoxItem.ServerCode = ServerCode;
  this->m_ListBoxItem.matchInfo = *matchInfo;
  size = this->m_ListBoxItems.size;
  this->m_ListBoxItem.Color = Color;
  if ( size > this->MaxInnerTextLines )
  {
    do
    {
      if ( size <= 0 )
        Logger.g->Panic("SDArray::operator[]: invalid index (%d)", 0);
      v14 = size - 1;
      v15 = 0;
      this->m_ListBoxItems.size = size - 1;
      if ( size - 1 > 0 )
      {
        v16 = 0;
        do
        {
          this->m_ListBoxItems.array[v16] = this->m_ListBoxItems.array[v16 + 1];
          v14 = this->m_ListBoxItems.size;
          ++v15;
          ++v16;
        }
        while ( v15 < v14 );
      }
      this->m_ListBoxItems.array[v14].matchInfo.~MatchInfo();
      memset(
        &this->m_ListBoxItems.array[this->m_ListBoxItems.size],
        0,
        sizeof(this->m_ListBoxItems.array[this->m_ListBoxItems.size]));
      size = this->m_ListBoxItems.size;
    }
    while ( size > this->MaxInnerTextLines );
    p_m_ListBoxItem = &this->m_ListBoxItem;
  }
  maxsize = this->m_ListBoxItems.maxsize;
  if ( size == maxsize )
  {
    if ( maxsize >= 16 )
      v18 = 6 * maxsize / 5;
    else
      v18 = 16;
    v19 = (SMatchListBoxItem *)realloc(this->m_ListBoxItems.array, sizeof(SMatchListBoxItem) * v18);
    v20 = this->m_ListBoxItems.maxsize;
    this->m_ListBoxItems.array = v19;
    memset(&v19[v20], 0, sizeof(SMatchListBoxItem) * (v18 - v20));
    this->m_ListBoxItems.maxsize = v18;
    size = this->m_ListBoxItems.size;
  }
  this->m_ListBoxItems.size = size + 1;
  this->m_ListBoxItems.array[size] = *p_m_ListBoxItem;
  if ( this->GoodWhenFiltered(size) )
    this->m_GoodWhenFilteredItems.array[this->m_GoodWhenFilteredItems.Add()] = size;
  if ( !this->SelectAble )
  {
    v21 = this->m_GoodWhenFilteredItems.size;
    if ( !this->Filter.On )
    {
      v22 = this->m_ListBoxItems.size;
      if ( v22 != v21 )
      {
        Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
        v22 = this->m_ListBoxItems.size;
      }
      v21 = v22;
    }
    this->EnsureIndexIsVisible(v21 - 1);
  }
  this->Update();
  return size;
}

//----- (0048AAC0) --------------------------------------------------------

void SMatchListBox::Create()

{
  int v2;
  int width;
  int height;
  this->Font = 0;
  this->VisibleTextLines = 9;
  this->SelectAble = 1;
  Board->GetTextExtent(0, 0, 0, &width, &height, 1.0f);
  this->Resize(744, height * this->VisibleTextLines + 12);
  this->RowHeight = height;
  SDXWidget::Create((int)this);
  this->TextLineBackFrames = (int *)operator new[](4 * this->VisibleTextLines);
  this->StatusFrame = (int *)operator new[](4 * this->VisibleTextLines);
  this->GameNameFrame = (int *)operator new[](4 * this->VisibleTextLines);
  this->MapFrame = (int *)operator new[](4 * this->VisibleTextLines);
  this->PlayerFrame = (int *)operator new[](4 * this->VisibleTextLines);
  this->GameTypeFrame = (int *)operator new[](4 * this->VisibleTextLines);
  v2 = 0;
  for ( this->VersionFrame = (int *)operator new[](4 * this->VisibleTextLines); v2 < this->VisibleTextLines; ++v2 )
  {
    this->TextLineBackFrames[v2] = Board->CreateFrame(FT_BOX, this->BackFrame, 6, height * v2 + 6, 0, 1);
    Board->ResizeFrame(this->TextLineBackFrames[v2], 732, height);
    this->StatusFrame[v2] = Board->CreateFrame(FT_SPRITE, this->TextLineBackFrames[v2], 5, 3, 0, 1);
    Board->ResizeFrame(this->StatusFrame[v2], 13, height);
    this->GameNameFrame[v2] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v2], 23, 0, 0, 1);
    Board->ResizeFrame(this->GameNameFrame[v2], 217, height);
    this->MapFrame[v2] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v2], 245, 0, 0, 1);
    Board->ResizeFrame(this->MapFrame[v2], 140, height);
    this->PlayerFrame[v2] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v2], 390, 0, 0, 1);
    Board->ResizeFrame(this->PlayerFrame[v2], 50, height);
    this->GameTypeFrame[v2] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v2], 445, 0, 0, 1);
    Board->ResizeFrame(this->GameTypeFrame[v2], 180, height);
    this->VersionFrame[v2] = Board->CreateFrame(FT_FIXTEXT, this->TextLineBackFrames[v2], 630, 0, 0, 1);
    Board->ResizeFrame(this->VersionFrame[v2], 75, height);
  }
  this->StateFont = Board->LoadFixedFont("menu/multi_status.png", 13, 13, 5, 5, 0, X2);
  this->InsertChild(&this->Slider);
  this->Slider.Create((int)this, 744, this->Height, this->VisibleTextLines);
  this->Update();
}

//----- (0048AE50) --------------------------------------------------------

int SMatchListBox::DeleteItem(int nIndex)

{
  int m_nCurSel;
  int size;
  int nIndexa;
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  if ( nIndex == this->m_nCurSel )
    this->SetCurSel(-1);
  this->m_ListBoxItems.Remove(nIndex);
  this->SetGoodWhenFilteredItems();
  m_nCurSel = this->m_nCurSel;
  nIndexa = m_nCurSel;
  if ( this->Filter.On )
  {
    size = this->m_GoodWhenFilteredItems.size;
  }
  else
  {
    size = this->m_ListBoxItems.size;
    if ( size != this->m_GoodWhenFilteredItems.size )
    {
      Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
      size = this->m_ListBoxItems.size;
      m_nCurSel = nIndexa;
    }
  }
  if ( m_nCurSel >= size )
    this->SetCurSel(-1);
  this->Update();
  return nIndex;
}

//----- (0048AF00) --------------------------------------------------------

int SMatchListBox::EnsureIndexIsVisible(int nIndex, bool bRolling)

{
  int size;
  int v5;
  int v6;
  int m_nTopIndex;
  int v8;
  int v9;
  if ( this->Filter.On )
  {
    size = this->m_GoodWhenFilteredItems.size;
  }
  else
  {
    size = this->m_ListBoxItems.size;
    if ( size != this->m_GoodWhenFilteredItems.size )
    {
      Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
      size = this->m_ListBoxItems.size;
    }
  }
  v5 = nIndex;
  if ( nIndex < size && nIndex >= 0 )
  {
    if ( this->Filter.On )
    {
      v6 = this->m_GoodWhenFilteredItems.size;
    }
    else
    {
      v6 = this->m_ListBoxItems.size;
      if ( v6 != this->m_GoodWhenFilteredItems.size )
      {
        Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
        v6 = this->m_ListBoxItems.size;
      }
    }
    if ( v6 < this->VisibleTextLines )
      return v5;
  }
  if ( !bRolling )
  {
    m_nTopIndex = this->m_nTopIndex;
    if ( nIndex < m_nTopIndex + this->VisibleTextLines && nIndex > m_nTopIndex )
      return v5;
  }
  if ( this->Filter.On )
  {
    v8 = this->m_GoodWhenFilteredItems.size;
  }
  else
  {
    v8 = this->m_ListBoxItems.size;
    if ( v8 != this->m_GoodWhenFilteredItems.size )
    {
      Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
      v8 = this->m_ListBoxItems.size;
    }
  }
  if ( nIndex < v8 && nIndex >= 0 )
  {
    while ( 1 )
    {
      if ( this->Filter.On )
      {
        v9 = this->m_GoodWhenFilteredItems.size;
      }
      else
      {
        v9 = this->m_ListBoxItems.size;
        if ( v9 != this->m_GoodWhenFilteredItems.size )
        {
          Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
          v9 = this->m_ListBoxItems.size;
        }
      }
      if ( v5 + this->VisibleTextLines <= v9 )
        break;
      --v5;
    }
    v5 = this->SetTopIndex(v5);
    this->Update();
    return v5;
  }
  return -1;
}

//----- (0048B060) --------------------------------------------------------

int SMatchListBox::GetCount()

{
  int result;
  if ( this->Filter.On )
    return this->m_GoodWhenFilteredItems.size;
  result = this->m_ListBoxItems.size;
  if ( result != this->m_GoodWhenFilteredItems.size )
  {
    Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
    return this->m_ListBoxItems.size;
  }
  return result;
}

//----- (0048B0A0) --------------------------------------------------------

int SMatchListBox::GetCountWithoutFiltering()

{
  return this->m_ListBoxItems.size;
}

//----- (0048B0B0) --------------------------------------------------------

int SMatchListBox::GetCurSel()

{
  return this->m_nCurSel;
}

//----- (0048B0C0) --------------------------------------------------------

char *SMatchListBox::GetGameName(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return (char *)"";
  else
    return this->m_ListBoxItems.array[nIndex].GameName;
}

//----- (0048B0F0) --------------------------------------------------------

const MatchInfo *SMatchListBox::GetMatchInfo(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return 0;
  else
    return &this->m_ListBoxItems.array[nIndex].matchInfo;
}

//----- (0048B120) --------------------------------------------------------

int SMatchListBox::GetMaxPlayers(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].MaxPlayers;
}

//----- (0048B150) --------------------------------------------------------

unsigned int SMatchListBox::GetServerCode(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
    return -1;
  else
    return this->m_ListBoxItems.array[nIndex].ServerCode;
}

//----- (0048B180) --------------------------------------------------------

bool SMatchListBox::GetStatus(int nIndex)

{
  return nIndex < this->m_ListBoxItems.size && nIndex >= 0 && this->m_ListBoxItems.array[nIndex].Status;
}

//----- (0048B1B0) --------------------------------------------------------

int SMatchListBox::GetTopIndex()

{
  return this->m_nTopIndex;
}

//----- (0048B1C0) --------------------------------------------------------

SVersion SMatchListBox::GetVersion(int nIndex)

{
  if ( nIndex >= this->m_ListBoxItems.size || nIndex < 0 )
  {
    return SVersion(0);
  }
  else
  {
    return this->m_ListBoxItems.array[nIndex].Version;
  }
}

//----- (0048B210) --------------------------------------------------------

char SMatchListBox::GoodWhenFiltered(int Index)

{
  char v4;
  int v5;
  int ActPlayers;
  int GameType;
  bool *v8;
  char *Text;
  int v10;
  bool *v11;
  char *v12;
  int Indexa;
  if ( !this->Filter.On )
    return 1;
  v4 = 1;
  if ( Index >= this->m_ListBoxItems.size )
    return 0;
  v5 = 876 * Index;
  Indexa = Index;
  if ( this->Filter.Status )
    v4 = *(&this->m_ListBoxItems.array->Status + v5);
  ActPlayers = this->Filter.ActPlayers;
  if ( ActPlayers && *(int *)&this->m_ListBoxItems.array->Map[v5 + 262] >= ActPlayers )
    v4 = 0;
  GameType = this->Filter.GameType;
  if ( GameType )
  {
    if ( GameType == 1 )
    {
      v8 = &this->m_ListBoxItems.array->Status + v5;
      Text = GetText("SWINE_CHATROOM_DEATHMATCH");
      v10 = _stricmp((const char *)v8 + 532, Text);
      v5 = Indexa * 876;
      if ( v10 )
        v4 = 0;
    }
    if ( this->Filter.GameType == 2 )
    {
      v11 = &this->m_ListBoxItems.array->Status + v5;
      v12 = GetText("SWINE_CHATROOM_CAPTURETHEFLAG");
      if ( _stricmp((const char *)v11 + 532, v12) )
        v4 = 0;
    }
  }
  if ( this->Filter.Version.GetInt() > 0 )
    return this->Filter.Version.IsGoodVersion(this->m_ListBoxItems.array[Indexa].Version) ? v4 : 0;
  return v4;
}

//----- (0048B340) --------------------------------------------------------

bool SMatchListBox::OnAction(SWidget *sender, int action, int param)

{
  switch ( action )
  {
    case 341345:
      this->EnsureIndexIsVisible(this->m_nTopIndex - 1, 1);
      return 1;
    case 341346:
      this->EnsureIndexIsVisible(this->m_nTopIndex + 1, 1);
      return 1;
    case 341347:
      this->SetTopIndex(param);
      return 1;
    default:
      return 0;
  }
}

//----- (0048B3A0) --------------------------------------------------------

void SMatchListBox::OnMouseDown(int button, int x, int y, int shift)

{
  int v6;
  int Height;
  int m_nCurSel;
  if ( this->SelectAble )
  {
    v6 = y;
    if ( button == 1 )
    {
      Height = this->Height;
      if ( y >= Height - 6 )
        v6 = Height - 7;
      if ( this->m_nCurSel == this->m_nTopIndex + (v6 - 6) / this->RowHeight
        && (unsigned int)Timer.GetTickValue() - this->LastClickTime <= 0x2EE )
      {
        m_nCurSel = this->m_nCurSel;
        this->LastClickTime = 0;
        this->SendAction(81052707, m_nCurSel);
        return;
      }
      if ( this->SetCurSel(this->m_nTopIndex + (v6 - 6) / this->RowHeight) != -2 )
      {
        Concert->PlaySound(
          "menu/button_down.wav",
          -12.0f,
          0,
          -1);
        this->SendAction(81052705, this->m_nCurSel);
        this->LastClickTime = (int)Timer.GetTickValue();
      }
    }
    SDXWidget::OnMouseDown(button, x, v6, shift);
  }
}

//----- (0048B4A0) --------------------------------------------------------

void SMatchListBox::OnMouseMove(int x, int y, int shift)

{
  if ( this->SelectAble )
  {
    this->LastClickTime = 0;
    this->CurActive = this->m_nTopIndex + (y - 6) / this->RowHeight;
    this->Update();
    SDXWidget::OnMouseMove(x, y, shift);
  }
}

//----- (0048B4F0) --------------------------------------------------------

void SMatchListBox::OnMouseOut()

{
  if ( this->SelectAble )
  {
    this->CurActive = -1;
    this->Update();
  }
  SDXWidget::OnMouseOut();
}

//----- (0048B520) --------------------------------------------------------

void SMatchListBox::OnMouseWheel(int button, int x, int y, int delta)

{
  this->SendAction((button <= 0) + 341345, 0);
}

//----- (0048B5D0) --------------------------------------------------------

void SMatchListBox::ResetContent()

{
  int size;
  int v3;
  int v4;
  int maxsize;
  SMatchListBoxItem *array;
  this->SetCurSel(-1);
  size = this->m_ListBoxItems.size;
  if ( size && !this->m_ListBoxItems.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  v3 = 0;
  if ( size > 0 )
  {
    v4 = 0;
    do
    {
      this->m_ListBoxItems.array[v4].matchInfo.~MatchInfo();
      ++v3;
      ++v4;
    }
    while ( v3 < this->m_ListBoxItems.size );
  }
  maxsize = this->m_ListBoxItems.maxsize;
  this->m_ListBoxItems.size = 0;
  if ( maxsize < 0 )
  {
    array = this->m_ListBoxItems.array;
    this->m_ListBoxItems.maxsize = 0;
    this->m_ListBoxItems.array = (SMatchListBoxItem *)realloc(array, 0);
    maxsize = this->m_ListBoxItems.maxsize;
  }
  memset(this->m_ListBoxItems.array, 0, 876 * maxsize);
  this->m_GoodWhenFilteredItems.Clear();
  this->m_nTopIndex = 0;
  this->Update();
}

//----- (0048B6B0) --------------------------------------------------------

void SMatchListBox::ResetFilter()

{
  this->Filter.Status = 0;
  this->Filter.ActPlayers = 0;
  this->Filter.GameType = 0;
  this->Filter.Version = SVersion(0);
  this->Filter.On = 0;
  this->Filter.Changed = 0;
  this->SetGoodWhenFilteredItems();
}

//----- (0048B720) --------------------------------------------------------

int SMatchListBox::SetCurSel(int nSelect)

{
  int size;
  int v4;
  int v5;
  int m_nCurSel;
  int m_nTopIndex;
  int VisibleTextLines;
  size = this->m_GoodWhenFilteredItems.size;
  if ( !this->Filter.On )
  {
    v4 = this->m_ListBoxItems.size;
    if ( v4 != size )
    {
      Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
      v4 = this->m_ListBoxItems.size;
    }
    size = v4;
  }
  v5 = nSelect;
  if ( nSelect < size && nSelect >= -1 )
  {
    m_nCurSel = this->m_nCurSel;
    this->m_nCurSel = nSelect;
    this->Update();
    m_nTopIndex = this->m_nTopIndex;
    if ( m_nTopIndex <= nSelect )
    {
      VisibleTextLines = this->VisibleTextLines;
      if ( VisibleTextLines + m_nTopIndex > nSelect )
        return m_nCurSel;
      v5 = nSelect - VisibleTextLines + 1;
    }
    this->SetTopIndex(v5);
    return m_nCurSel;
  }
  return -2;
}

//----- (0048B7C0) --------------------------------------------------------

void SMatchListBox::SetFilter(bool Status, int ActPlayers, int GameType, SVersion Version)

{
  SwineVersionType VersionType;
  int Int;
  VersionType = Version.VersionType;
  *(long long *)&this->Filter.Version.MajorVersion = *(long long *)&Version.MajorVersion;
  this->Filter.Status = Status;
  this->Filter.ActPlayers = ActPlayers;
  this->Filter.GameType = GameType;
  this->Filter.Version.VersionType = VersionType;
  Int = Version.GetInt();
  this->Filter.Changed = 1;
  this->Filter.On = (Int > 0) + Status + (ActPlayers > 0) + (GameType > 0);
  this->SetGoodWhenFilteredItems();
  this->Update();
}

//----- (0048B850) --------------------------------------------------------

void SMatchListBox::SetGoodWhenFilteredItems()

{
  SDArray<int> *p_m_GoodWhenFilteredItems;
  int i;
  int size;
  int maxsize;
  int v6;
  int *v7;
  int v8;
  p_m_GoodWhenFilteredItems = &this->m_GoodWhenFilteredItems;
  this->m_GoodWhenFilteredItems.Clear();
  for ( i = 0; i < this->m_ListBoxItems.size; ++i )
  {
    if ( this->GoodWhenFiltered(i) )
    {
      size = p_m_GoodWhenFilteredItems->size;
      maxsize = p_m_GoodWhenFilteredItems->maxsize;
      if ( p_m_GoodWhenFilteredItems->size == maxsize )
      {
        if ( maxsize >= 16 )
          v6 = 6 * maxsize / 5;
        else
          v6 = 16;
        v7 = (int *)realloc(p_m_GoodWhenFilteredItems->array, 4 * v6);
        v8 = p_m_GoodWhenFilteredItems->maxsize;
        p_m_GoodWhenFilteredItems->array = v7;
        memset(&v7[v8], 0, 4 * (v6 - v8));
        size = p_m_GoodWhenFilteredItems->size;
        p_m_GoodWhenFilteredItems->maxsize = v6;
      }
      p_m_GoodWhenFilteredItems->size = size + 1;
      this->m_GoodWhenFilteredItems.array[size] = i;
    }
  }
}

//----- (0048B910) --------------------------------------------------------

int SMatchListBox::SetTopIndex(int nIndex)

{
  int size;
  int v4;
  int m_nTopIndex;
  size = this->m_GoodWhenFilteredItems.size;
  if ( !this->Filter.On )
  {
    v4 = this->m_ListBoxItems.size;
    if ( v4 != size )
    {
      Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
      v4 = this->m_ListBoxItems.size;
    }
    size = v4;
  }
  if ( nIndex >= size || nIndex < 0 )
    return -1;
  m_nTopIndex = this->m_nTopIndex;
  this->m_nTopIndex = nIndex;
  this->Update();
  return m_nTopIndex;
}

//----- (0048B980) --------------------------------------------------------

void SMatchListBox::Update()

{
  int VisibleTextLines;
  int v5;
  int *TextLineBackFrames;
  int size;
  int v9;
  int v10;
  SMatchListBoxItem *v11;
  int v12;
  SString ShortString;
  const char *v14;
  char *buf;
  int v16;
  int v17;
  int v18;
  SMatchListBoxItem *v19;
  int v20;
  SString v21;
  const char *v22;
  char *v23;
  int *v24;
  int v26;
  SMatchListBoxItem *v27;
  int v28;
  SString v29;
  const char *v30;
  char *v31;
  int v32;
  SMatchListBoxItem *v33;
  int v34;
  SString v35;
  const char *v36;
  char *v37;
  int v38;
  int v39;
  SString v42;
  SString result;
  BOOL v44;
  char strBuf[264];
  if ( this->BackFrame >= 0 )
  {
    if ( this->Filter.Changed )
    {
      this->Filter.Changed = 0;
      this->m_nTopIndex = 0;
      this->m_nCurSel = -1;
    }
    VisibleTextLines = this->VisibleTextLines;
    v5 = 0;
    if ( this->Filter.On )
    {
      if ( VisibleTextLines > 0 )
      {
        do
        {
          TextLineBackFrames = this->TextLineBackFrames;
          if ( v5 + this->m_nTopIndex == this->m_nCurSel )
            Board->SetBoxColor(
              TextLineBackFrames[v5],
              0x80000000);
          else
            Board->SetBoxColor(TextLineBackFrames[v5], 0);
          if ( !this->SelectAble )
            goto LABEL_24;
          size = this->m_GoodWhenFilteredItems.size;
          if ( !this->Filter.On )
          {
            v9 = this->m_ListBoxItems.size;
            if ( v9 != size )
            {
              Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
              v9 = this->m_ListBoxItems.size;
            }
            size = v9;
          }
          v10 = v5 + this->m_nTopIndex;
          if ( v10 < size && v10 == this->CurActive )
          {
            v11 = &this->m_ListBoxItems.array[this->m_GoodWhenFilteredItems.array[v10]];
            if ( v11->Status )
            {
              v44 = v11->NumPlayers == v11->MaxPlayers;
              v12 = v44;
            }
            else
            {
              v12 = 3;
            }
            Board->SetSpriteGlyph(this->StatusFrame[v5], this->StateFont, v12);
            Board->ShowFrame(this->StatusFrame[v5], 1);
            Board->SetText(this->GameNameFrame[v5], this->Font + 1, 0, v11->GameName);
            Board->SetText(this->MapFrame[v5], this->Font + 1, 0, v11->Map);
            sprintf(strBuf, "%d/%d", v11->NumPlayers, v11->MaxPlayers);
            Board->SetText(this->PlayerFrame[v5], this->Font + 1, 0, strBuf);
            Board->SetText(this->GameTypeFrame[v5], this->Font + 1, 0, v11->GameType);
            ShortString = v11->Version.GetShortString();
            v14 = "";
            buf = ShortString.buf;
            if ( buf )
              v14 = buf;
            sprintf(strBuf, "%s", v14);
            if ( ShortString.buf )
            {
              delete[] ShortString.buf;
              ShortString.buf = 0;
            }
            Board->SetText(this->VersionFrame[v5], this->Font + 1, 0, strBuf);
            Board->SetTextColor(this->GameNameFrame[v5], 0xFFFFFFu);
            Board->SetTextColor(this->MapFrame[v5], 0xFFFFFFu);
            Board->SetTextColor(this->PlayerFrame[v5], 0xFFFFFFu);
            Board->SetTextColor(this->GameTypeFrame[v5], 0xFFFFFFu);
            Board->SetTextColor(this->VersionFrame[v5], 0xFFFFFFu);
          }
          else
          {
LABEL_24:
            v16 = this->m_GoodWhenFilteredItems.size;
            if ( !this->Filter.On )
            {
              v17 = this->m_ListBoxItems.size;
              if ( v17 != v16 )
              {
                Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
                v17 = this->m_ListBoxItems.size;
              }
              v16 = v17;
            }
            v18 = v5 + this->m_nTopIndex;
            if ( v18 >= v16 )
            {
              Board->ShowFrame(this->StatusFrame[v5], 0);
              Board->SetText(this->GameNameFrame[v5], this->Font, 0, "");
              Board->SetText(this->MapFrame[v5], this->Font, 0, "");
              Board->SetText(this->PlayerFrame[v5], this->Font, 0, "");
              Board->SetText(this->GameTypeFrame[v5], this->Font, 0, "");
              Board->SetText(this->VersionFrame[v5], this->Font, 0, "");
            }
            else
            {
              v19 = &this->m_ListBoxItems.array[this->m_GoodWhenFilteredItems.array[v18]];
              if ( v19->Status )
              {
                v44 = v19->NumPlayers == v19->MaxPlayers;
                v20 = v44;
              }
              else
              {
                v20 = 3;
              }
              Board->SetSpriteGlyph(this->StatusFrame[v5], this->StateFont, v20);
              Board->ShowFrame(this->StatusFrame[v5], 1);
              Board->SetText(this->GameNameFrame[v5], this->Font, 0, v19->GameName);
              Board->SetText(this->MapFrame[v5], this->Font, 0, v19->Map);
              sprintf(strBuf, "%d/%d", v19->NumPlayers, v19->MaxPlayers);
              Board->SetText(this->PlayerFrame[v5], this->Font, 0, strBuf);
              Board->SetText(this->GameTypeFrame[v5], this->Font, 0, v19->GameType);
              v21 = v19->Version.GetShortString();
              v22 = "";
              v23 = v21.buf;
              if ( v23 )
                v22 = v23;
              sprintf(strBuf, "%s", v22);
              if ( v21.buf )
              {
                delete[] v21.buf;
                v21.buf = 0;
              }
              Board->SetText(this->VersionFrame[v5], this->Font, 0, strBuf);
              Board->SetTextColor(this->GameNameFrame[v5], v19->Color);
              Board->SetTextColor(this->MapFrame[v5], v19->Color);
              Board->SetTextColor(this->PlayerFrame[v5], v19->Color);
              Board->SetTextColor(this->GameTypeFrame[v5], v19->Color);
              Board->SetTextColor(this->VersionFrame[v5], v19->Color);
            }
          }
          ++v5;
        }
        while ( v5 < this->VisibleTextLines );
      }
    }
    else if ( VisibleTextLines > 0 )
    {
      do
      {
        v24 = this->TextLineBackFrames;
        if ( v5 + this->m_nTopIndex == this->m_nCurSel )
          Board->SetBoxColor(v24[v5], 0x80000000);
        else
          Board->SetBoxColor(v24[v5], 0);
        if ( this->SelectAble
          && (v26 = v5 + this->m_nTopIndex, v26 < this->m_ListBoxItems.size)
          && v26 == this->CurActive )
        {
          v27 = &this->m_ListBoxItems.array[v26];
          if ( v27->Status )
          {
            v44 = v27->NumPlayers == v27->MaxPlayers;
            v28 = v44;
          }
          else
          {
            v28 = 3;
          }
          Board->SetSpriteGlyph(this->StatusFrame[v5], this->StateFont, v28);
          Board->ShowFrame(this->StatusFrame[v5], 1);
          Board->SetText(this->GameNameFrame[v5], this->Font + 1, 0, v27->GameName);
          Board->SetText(this->MapFrame[v5], this->Font + 1, 0, v27->Map);
          sprintf(strBuf, "%d/%d", v27->NumPlayers, v27->MaxPlayers);
          Board->SetText(this->PlayerFrame[v5], this->Font + 1, 0, strBuf);
          Board->SetText(this->GameTypeFrame[v5], this->Font + 1, 0, v27->GameType);
          v29 = v27->Version.GetShortString();
          v30 = "";
          v31 = v29.buf;
          if ( v31 )
            v30 = v31;
          sprintf(strBuf, "%s", v30);
          if ( v29.buf )
          {
            delete[] v29.buf;
            v29.buf = 0;
          }
          Board->SetText(this->VersionFrame[v5], this->Font + 1, 0, strBuf);
          Board->SetTextColor(this->GameNameFrame[v5], 0xFFFFFFu);
          Board->SetTextColor(this->MapFrame[v5], 0xFFFFFFu);
          Board->SetTextColor(this->PlayerFrame[v5], 0xFFFFFFu);
          Board->SetTextColor(this->GameTypeFrame[v5], 0xFFFFFFu);
          Board->SetTextColor(this->VersionFrame[v5], 0xFFFFFFu);
        }
        else
        {
          v32 = v5 + this->m_nTopIndex;
          if ( v32 >= this->m_ListBoxItems.size )
          {
            Board->ShowFrame(this->StatusFrame[v5], 0);
            Board->SetText(this->GameNameFrame[v5], this->Font, 0, "");
            Board->SetText(this->MapFrame[v5], this->Font, 0, "");
            Board->SetText(this->PlayerFrame[v5], this->Font, 0, "");
            Board->SetText(this->GameTypeFrame[v5], this->Font, 0, "");
            Board->SetText(this->VersionFrame[v5], this->Font, 0, "");
          }
          else
          {
            v33 = &this->m_ListBoxItems.array[v32];
            if ( v33->Status )
            {
              v44 = v33->NumPlayers == v33->MaxPlayers;
              v34 = v44;
            }
            else
            {
              v34 = 3;
            }
            Board->SetSpriteGlyph(this->StatusFrame[v5], this->StateFont, v34);
            Board->ShowFrame(this->StatusFrame[v5], 1);
            Board->SetText(this->GameNameFrame[v5], this->Font, 0, v33->GameName);
            Board->SetText(this->MapFrame[v5], this->Font, 0, v33->Map);
            sprintf(strBuf, "%d/%d", v33->NumPlayers, v33->MaxPlayers);
            Board->SetText(this->PlayerFrame[v5], this->Font, 0, strBuf);
            Board->SetText(this->GameTypeFrame[v5], this->Font, 0, v33->GameType);
            v35 = v33->Version.GetShortString();
            v36 = "";
            v37 = v35.buf;
            if ( v37 )
              v36 = v37;
            sprintf(strBuf, "%s", v36);
            if ( v35.buf )
            {
              delete[] v35.buf;
              v35.buf = 0;
            }
            Board->SetText(this->VersionFrame[v5], this->Font, 0, strBuf);
            Board->SetTextColor(this->GameNameFrame[v5], v33->Color);
            Board->SetTextColor(this->MapFrame[v5], v33->Color);
            Board->SetTextColor(this->PlayerFrame[v5], v33->Color);
            Board->SetTextColor(this->GameTypeFrame[v5], v33->Color);
            Board->SetTextColor(this->VersionFrame[v5], v33->Color);
          }
        }
        ++v5;
      }
      while ( v5 < this->VisibleTextLines );
    }
    v38 = this->m_GoodWhenFilteredItems.size;
    if ( !this->Filter.On )
    {
      v39 = this->m_ListBoxItems.size;
      if ( v39 != v38 )
      {
        Logger.g->Warning("m_ListBoxItems.GetSize() != m_GoodWhenFilteredItems.GetSize()");
        v39 = this->m_ListBoxItems.size;
      }
      v38 = v39;
    }
    this->Slider.SetRows(v38, this->m_nTopIndex);
  }
}
