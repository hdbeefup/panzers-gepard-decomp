// window/dxwidget.cpp
// DirectX widget base
// Decompiled from: gameSplit/sdxwidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <string.h>

extern unsigned char g_MenuRace;
#include "dxwidget.h"
#include "scaler.h"
#include "dxwindow.h"
#include "logger.h"

// Forward declarations for classes defined elsewhere
struct SWorld;


struct SMenu : SDXWidget {
  int FaceFont;
  int FaceFrame;
  int VersionFrame;
};

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

struct STextButton;
struct SListBox;

struct SResultsMenu : SMenu {
  int CarFont;
  char _SResultsMenu_padding[2048]; // placeholder for member fields

  bool OnKeyDown(int keycode, bool repeat = false);
};

// Classes: SDXWidget
// Function count: 23

//----- (004863B0) --------------------------------------------------------

SDXWidget::SDXWidget()

{
  this->BackFrame = -1;
  *(_WORD *)&this->BackNoresize = 0;
  this->TooltipBackFrame = -1;
  this->TooltipFrame = -1;
  this->BackFont = -1;
  this->BackColor = 0;
  this->Gravity = 0;
  this->TooltipTimer = -1;
  this->TooltipEndTimer = -1;
  this->TooltipText[0] = 0;
  this->TooltipFont = -1;
  this->ToolTipTextWidth = 0;
  this->ToolTipTextHeight = 0;
  this->ToolTipFeatureEnabled = 1;
}

//----- (00486430) --------------------------------------------------------

SDXWidget::~SDXWidget()

{
  int BackFrame;
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
    Board->DestroyFrame(BackFrame);
  if ( this->TooltipBackFrame >= 0 )
    Board->DestroyFrame(this->TooltipBackFrame);
  Board->ReleaseFont(this->TooltipFont);
  // base destructor called automatically
}

//----- (00486550) --------------------------------------------------------

void SDXWidget::Create(int a2)

{
  int v5;
  int v6;
  int v8;
  int v10;
  SWidget *WindowOrScalerParent; // eax
  int v13;
  int v14;
  int v15;
  int v17;
  int v18;
  int v19;
  int v20;
  int v21;
  int X;
  int Y;
  int Gravity;
  int v25;
  int BackGlyph;
  int v27;
  int v28;
  int height;
  int width;
  SCustomGlyph menu_glyphs[3];
  Gravity = this->Gravity;
  Y = this->Y;
  X = this->X;
  if ( this->BackFont >= 0 )
  {
    v25 = this->Parent->GetFrame();
    v5 = 1;
    if ( this->Back9Slice )
      v5 = 8;
    v6 = Board->CreateFrame((SFrameType)v5, v25, X, Y, Gravity, 1);
    BackGlyph = this->BackGlyph;
    this->BackFrame = v6;
    Board->SetSpriteGlyph(this->BackFrame, this->BackFont, BackGlyph);
    if ( !this->BackNoresize )
    {
      if ( this->Back9Slice )
      {
LABEL_8:
        Board->ResizeFrame(this->BackFrame, this->Width, this->Height);
        goto LABEL_12;
      }
      Board->GetFrameSize(this->BackFrame, &width, &height);
      this->Resize(width, height);
    }
    if ( !this->Back9Slice )
      goto LABEL_12;
    goto LABEL_8;
  }
  if ( this->BackColor )
  {
    v25 = this->Parent->GetFrame();
    v8 = Board->CreateFrame(FT_BOX, v25, X, Y, Gravity, 1);
    v27 = this->Height;
    this->BackFrame = v8;
    Board->ResizeFrame(v8, this->Width, v27);
    Board->SetBoxColor(this->BackFrame, this->BackColor);
  }
  else
  {
    v25 = this->Parent->GetFrame();
    v10 = Board->CreateFrame(FT_EMPTY, v25, X, Y, Gravity, 1);
    v28 = this->Height;
    this->BackFrame = v10;
    Board->ResizeFrame(v10, this->Width, v28);
  }
LABEL_12:
  Board->ShowFrame(this->BackFrame, this->Visible);
  if ( this->ToolTipFeatureEnabled )
  {
    Board->GetTextExtent(0,
      this->TooltipText,
      strlen(this->TooltipText),
      &this->ToolTipTextWidth,
      &this->ToolTipTextHeight,
      1.0f);
    WindowOrScalerParent = this->GetWindowOrScalerParent();
    v13 = WindowOrScalerParent->GetFrame();
    v14 = Board->CreateFrame(FT_BOX, v13, 0, 0, 0, 1);
    v15 = this->ToolTipTextHeight + 2;
    this->TooltipBackFrame = v14;
    Board->ResizeFrame(v14, this->ToolTipTextWidth + 6, v15);
    // Glyph rectangles from binary (0x54C060/0x54C080/0x54C070):
    // Left cap, middle stretch, right cap — 25px tall tooltip background
    menu_glyphs[0] = {0, 0, 10, 25};   // left cap
    menu_glyphs[1] = {10, 0, 12, 25};  // middle (stretchable)
    menu_glyphs[2] = {22, 0, 10, 25};  // right cap
    if ( g_MenuRace )
      v17 = Board->LoadCustomFont(
              "menu/pig_tooltip.png",
              3,
              menu_glyphs,
              (HDMode)1);
    else
      v17 = Board->LoadCustomFont(
              "menu/rabbit_tooltip.png",
              3,
              menu_glyphs,
              (HDMode)1);
    this->TooltipFont = v17;
    v18 = Board->CreateFrame(FT_SPRITE, this->TooltipBackFrame, -5, 0, 0, 1);
    this->TooltipFrameLeft = v18;
    Board->SetSpriteGlyph(v18, this->TooltipFont, 0);
    v19 = Board->CreateFrame(FT_SPRITE, this->TooltipBackFrame, 5, 0, 0, 1);
    this->TooltipFrameMiddle = v19;
    Board->SetSpriteGlyph(v19, this->TooltipFont, 1);
    Board->ResizeFrame(this->TooltipFrameMiddle, this->ToolTipTextWidth - 3, 25);
    v20 = Board->CreateFrame(FT_SPRITE, this->TooltipBackFrame, this->ToolTipTextWidth + 2, 0, 0, 1);
    this->TooltipFrameRight = v20;
    Board->SetSpriteGlyph(v20, this->TooltipFont, 2);
    v21 = Board->CreateFrame(FT_TEXT, this->TooltipBackFrame, 4, 1, 0, 1);
    this->TooltipFrame = v21;
    Board->SetText(v21, 0, 0, this->TooltipText);
    Board->ShowFrame(this->TooltipBackFrame, 0);
  }
}

//----- (00486850) --------------------------------------------------------

int SDXWidget::GetFrame()

{
  return this->BackFrame;
}

//----- (00486860) --------------------------------------------------------

int SDXWidget::MessageBox(const char *text, int type)

{
  SWidget *wosParent;
  SDXWindow *winParent;
  wosParent = this->GetWindowOrScalerParent();
  winParent = (SDXWindow *)this->GetWindowParent();
  return winParent->MessageBoxA(wosParent, text, type);
}

//----- (00486890) --------------------------------------------------------

void SDXWidget::OnMouseDown(int button, int x, int y, int shift)

{
  int TooltipBackFrame;
  TooltipBackFrame = this->TooltipBackFrame;
  if ( TooltipBackFrame >= 0 )
  {
    Board->ShowFrame(TooltipBackFrame, 0);
    this->KillTimer(&this->TooltipTimer);
    this->KillTimer(&this->TooltipEndTimer);
  }
}

//----- (004868D0) --------------------------------------------------------

void SDXWidget::OnMouseMove(int x, int y, int shift)

{
  int TooltipBackFrame;
  int v7;
  float scaleFactor;
  float Scaling; // xmm2_4
  int v10;
  float v11; // xmm1_4
  int FrameYsize;
  int FrameXsize;
  int winY;
  int winX;
  TooltipBackFrame = this->TooltipBackFrame;
  v7 = TooltipBackFrame;
  if ( TooltipBackFrame >= 0 )
  {
    v7 = this->TooltipBackFrame;
    if ( !strlen(this->TooltipText) )
    {
      Board->ShowFrame(TooltipBackFrame, 0);
      this->KillTimer(&this->TooltipTimer);
      this->KillTimer(&this->TooltipEndTimer);
      v7 = this->TooltipBackFrame;
    }
  }
  if ( v7 >= 0 && strlen(this->TooltipText) )
  {
    if ( x < 0 || y < 0 || x >= this->Width || y >= this->Height )
    {
      Board->ShowFrame(v7, 0);
      this->KillTimer(&this->TooltipTimer);
      this->KillTimer(&this->TooltipEndTimer);
    }
    else
    {
      this->GetWindowOrParentScalerPosition(&winX, &winY, &scaleFactor);
      Board->GetFrameSize(0, &FrameXsize, &FrameYsize);
      SWindow *wp = this->GetWindowParent();
      if ( !wp ) return;
      Scaling = wp->Scaling;
      v10 = (int)(float)((float)((float)(Scaling * 18.0f) / scaleFactor) + (float)(x + winX));
      int v10y = (int)(float)((float)((float)(Scaling * 28.0f) / scaleFactor) + (float)(winY + y));
      v11 = (float)FrameXsize / scaleFactor;
      if ( (float)(v10 + this->ToolTipTextWidth + 7) > v11 )
        v10 = (int)(float)((float)((float)(v11 - (float)this->ToolTipTextWidth) - 2.0f) - 5.0f);
      float v11y = (float)FrameYsize / scaleFactor;
      if ( (float)(v10y + this->ToolTipTextHeight + 2) > v11y )
        v10y = (int)(float)((float)(v11y - (float)this->ToolTipTextHeight) - 2.0f);
      Board->MoveFrame(this->TooltipBackFrame, v10, v10y);
      this->KillTimer(&this->TooltipTimer);
      this->TooltipTimer = this->SetTimer(0x12Cu);
      this->KillTimer(&this->TooltipEndTimer);
      this->TooltipEndTimer = this->SetTimer(0x2710u);
    }
  }
}

//----- (00486AE0) --------------------------------------------------------

void SDXWidget::OnMouseOut()

{
  int TooltipBackFrame;
  TooltipBackFrame = this->TooltipBackFrame;
  if ( TooltipBackFrame >= 0 )
  {
    Board->ShowFrame(TooltipBackFrame, 0);
    this->KillTimer(&this->TooltipTimer);
    this->KillTimer(&this->TooltipEndTimer);
  }
}

//----- (00486B10) --------------------------------------------------------

void SDXWidget::OnTimer(int id, unsigned int time)

{
  int *p_TooltipTimer; // edi
  int TooltipBackFrame;
  int v6;
  p_TooltipTimer = &this->TooltipTimer;
  if ( id == this->TooltipTimer )
  {
    TooltipBackFrame = this->TooltipBackFrame;
    if ( TooltipBackFrame >= 0 && strlen(this->TooltipText) )
      Board->ShowFrame(TooltipBackFrame, 1);
    this->KillTimer(p_TooltipTimer);
  }
  if ( id == this->TooltipEndTimer )
  {
    v6 = this->TooltipBackFrame;
    if ( v6 >= 0 )
    {
      if ( strlen(this->TooltipText) )
        Board->ShowFrame(v6, 0);
    }
    this->KillTimer(&this->TooltipEndTimer);
  }
}

//----- (00486BA0) --------------------------------------------------------

void SDXWidget::Resize(int width, int height)

{
  int BackFrame;
  SWidget::Resize(width, height);
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
    Board->ResizeFrame(BackFrame, this->Width, this->Height);
}

//----- (00486BD0) --------------------------------------------------------

void SDXWidget::SetBackgroundColor(unsigned int color)

{
  this->BackColor = color;
}

//----- (00486BE0) --------------------------------------------------------

void SDXWidget::SetBackgroundSprite(int font, int glyph, bool noresize, bool nineslice)

{
  this->BackFont = font;
  this->BackGlyph = glyph;
  this->BackNoresize = noresize;
  this->Back9Slice = nineslice;
}

//----- (00486C00) --------------------------------------------------------

void SDXWidget::SetDefaultBackgroundSprite()

{
  this->BackFont = 5;
  this->BackGlyph = 0;
  *(_WORD *)&this->BackNoresize = 257;
}

//----- (00486C20) --------------------------------------------------------

void SDXWidget::SetGravity(int gravity)

{
  int BackFrame;
  SWidget::SetGravity(gravity);
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
    Board->GravitateFrame(BackFrame, this->Gravity);
}

//----- (00486C50) --------------------------------------------------------

void SDXWidget::SetPosition(int x, int y, int width, int height)

{
  int BackFrame;
  SWidget::SetPosition(x, y, width, height);
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
  {
    Board->MoveFrame(BackFrame, this->X, this->Y);
    Board->ResizeFrame(this->BackFrame, this->Width, this->Height);
  }
}

//----- (00486CA0) --------------------------------------------------------

void SDXWidget::SetTooltipText(const char *tooltiptext)

{
  char *v4; // ebx
  v4 = this->TooltipText;
  strcpy(v4, tooltiptext);
  if ( this->TooltipBackFrame >= 0 && strlen(this->TooltipText) )
  {
    Board->GetTextExtent(0, v4, strlen(v4), &this->ToolTipTextWidth, &this->ToolTipTextHeight, 1.0f);
    Board->ResizeFrame(this->TooltipBackFrame, this->ToolTipTextWidth + 6, this->ToolTipTextHeight + 2);
    Board->SetText(this->TooltipFrame, 0, 0, v4);
    int TooltipFrameMiddle = this->TooltipFrameMiddle;
    if ( TooltipFrameMiddle >= 0 )
    {
      Board->ResizeFrame(TooltipFrameMiddle, this->ToolTipTextWidth - 3, 25);
      Board->MoveFrame(this->TooltipFrameRight, this->ToolTipTextWidth + 2, 0);
    }
  }
}

//----- (00486D90) --------------------------------------------------------

void SDXWidget::SetVisible(bool visible)

{
  SWidget::SetVisible(visible);
  if ( this->BackFrame >= 0 )
    Board->ShowFrame(this->BackFrame, this->Visible);
}

//----- (004D8080) --------------------------------------------------------

SMenuBackGroundView::SMenuBackGroundView()

{
  // SScaler constructors called automatically
  this->World = 0;
  this->LastUpdate = 0;
}

//----- (004D82C0) --------------------------------------------------------

void SMenuBackGroundView::Create(int a2)

{
  SDXWidget::Create(a2);
  this->InsertChild(&this->PanelBackgroundScaler);
  this->InsertChild(&this->MenuBackgroundScaler);
  this->SetScalerPosition();
  this->PanelBackgroundScaler.Create();
  this->MenuBackgroundScaler.Create();
  this->SetEnable(0);
}

//----- (004D85B0) --------------------------------------------------------

void SMenuBackGroundView::SetPosition(int x, int y, int width, int height)

{
  SDXWidget::SetPosition(x, y, width, height);
  this->SetScalerPosition();
}

//----- (004ED4E0) --------------------------------------------------------

bool SResultsMenu::OnKeyDown(int keycode, bool repeat)

{
  char *Text; // eax
  if ( keycode == 27 )
  {
    Text = GetText("SWINE_AREYOUSURE_QUIT");
    if ( this->MessageBox(Text, 4) == 6 )
    {
      this->SendAction(337107, 0);
      return 1;
    }
  }
  else
  {
    if ( !Options->GetKeyboardMode() )
    {
      switch ( keycode )
      {
        case '&':
          this->SendAction(324866, 0);
          return 1;
        case '(':
          this->SendAction(324866, 1);
          return 1;
        case '%':
          this->SendAction(324866, 2);
          return 1;
        case '\'':
          this->SendAction(324866, 3);
          return 1;
      }
    }
    if ( keycode != 82 && keycode != 114 )
      return 0;
    this->SendAction(337106, 0);
  }
  return 1;
}

void SDXWidget::Update() {} // Base virtual — intentionally empty

//----- (004D85E0) --------------------------------------------------------

void SMenuBackGroundView::SetScalerPosition()
{
  // MenuBackgroundScaler: scale to fill screen based on 800x600 base
  float scaleX = (float)this->Width / 800.0f;
  float scaleY = (float)this->Height / 600.0f;
  float menuScale = 1.0f;
  float maxScale = (scaleX <= scaleY) ? scaleX : scaleY;
  if (maxScale > 1.0f)
    menuScale = maxScale;
  this->MenuBackgroundScaler.SetScaleFactor(menuScale);
  this->MenuBackgroundScaler.SetPosition(
    0, 0,
    (int)((float)this->Width / menuScale + 0.5f),
    (int)((float)this->Height / menuScale + 0.5f));

  // PanelBackgroundScaler: scale based on 1024x768 base (0.0009765625 = 1/1024)
  float panelScaleX = (float)this->Width * 0.0009765625f;
  float panelScaleY = (float)this->Height / 768.0f;
  float panelScale = 0.78125f;
  float panelMax = (panelScaleX <= panelScaleY) ? panelScaleX : panelScaleY;
  if (panelMax > 0.78125f)
    panelScale = panelMax;
  this->PanelBackgroundScaler.SetScaleFactor(panelScale);
  this->PanelBackgroundScaler.SetPosition(
    0, 0,
    (int)((float)this->Width / panelScale + 0.5f),
    (int)((float)this->Height / panelScale + 0.5f));
}
