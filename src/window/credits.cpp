// window/credits.cpp
// Credits display
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "credits.h"
#include "textbox.h"
#include "logger.h"
#include "properties.h"
#include "timer.h"

// Classes: SCredits
// Function count: 7

//----- (00483BA0) --------------------------------------------------------

SCredits::SCredits()

{
  int SpeechBeginDelay;
  float MusicDuration; // xmm1_4
  char *item;
  this->Widgets.array = 0;
  this->Widgets.size = 0;
  this->Widgets.maxsize = 0;
  this->ImageFonts.array = 0;
  this->ImageFonts.size = 0;
  this->ImageFonts.maxsize = 0;
  this->AutoScroll = 1;
  this->SpeechPaths.array = 0;
  this->SpeechPaths.size = 0;
  this->SpeechPaths.maxsize = 0;
  this->SpeechBeginDelay = 3000;
  this->SpeechEndDelay = 5000;
  this->MusicDuration = 216000.0;
  this->CreditsIni = new SProperties("credits.ini", 1, 1);
  item = "speech/r02-vontatos/262.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p02-parancsnok/588.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p02-pancelkocsi/318.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r10-pancelkocsi/227.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p04-normaltank/557.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r05-tank/055.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p06-raketas/314.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r07-mozsar/390.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r04-loveges/493.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r01-parancsnok/472.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p08-aknarako/335.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r11-aknarako/485.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p05-hardtank/586.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r04-loveges/410.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p06-raketas/332.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p07-vontatos/543.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p03-loveges/124.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r07-mozsar/222.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r12-movingforce/221.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p02-pancelkocsi/542.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r04-loveges/052.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p12-movingforce/369.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r-heroes/571.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r06-raketas/234.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r03-tankkiller/232.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p04-normaltank/023.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/r05-tank/224.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p09-mozsar/311.mp3";
  this->SpeechPaths.Add(&item);
  item = "speech/p-heroes/489.mp3";
  this->SpeechPaths.Add(&item);
  SpeechBeginDelay = this->SpeechBeginDelay;
  MusicDuration = this->MusicDuration;
  this->ContentHeight = 0.0;
  this->NextSpeechTime = SpeechBeginDelay;
  this->NextSpeechIndex = 0;
  this->SpeechInterval = (int)(float)((float)((float)(MusicDuration - (float)SpeechBeginDelay)
                                            - (float)this->SpeechEndDelay)
                                    / (float)this->SpeechPaths.size);
}

//----- (00483F50) --------------------------------------------------------

SCredits::~SCredits()

{
  SProperties *CreditsIni; // edi
  int i;
  SDXWidget *v4; // ecx
  int j;
  int size;
  int maxsize;
  char **v8; // eax
  CreditsIni = this->CreditsIni;
  if ( CreditsIni )
  {
    CreditsIni->~SProperties();
    ::operator delete(CreditsIni);
    this->CreditsIni = 0;
  }
  Board->SetPixelRounding(1);
  for ( i = 0; i < this->Widgets.size; ++i )
  {
    v4 = this->Widgets.array[i];
    if ( v4 )
    {
      delete v4;
      this->Widgets.array[i] = 0;
    }
  }
  for ( j = 0; j < this->ImageFonts.size; ++j )
    Board->ReleaseFont(this->ImageFonts.array[j]);
  Board->ReleaseFont(this->BottomGradientFont);
  Board->ReleaseFont(this->TopGradientFont);
  Concert->Update(1);
  size = this->SpeechPaths.size;
  if ( size && !this->SpeechPaths.array )
    Logger.g->Panic("SDArray::Clear: array is damaged");
  maxsize = this->SpeechPaths.maxsize;
  this->SpeechPaths.size = size;
  if ( size > maxsize )
  {
    this->SpeechPaths.maxsize = size;
    // x64: literal `4` is x86 sizeof(char*); on x64 a pointer is 8.
    v8 = (char **)realloc(this->SpeechPaths.array, sizeof(char *) * size);
    maxsize = this->SpeechPaths.maxsize;
    this->SpeechPaths.array = v8;
  }
  memset(this->SpeechPaths.array, 0, sizeof(char *) * maxsize);
  Concert->StopStreamingPlayback();
  if ( this->SpeechPaths.array )
  {
    free(this->SpeechPaths.array);
    this->SpeechPaths.array = 0;
  }
  if ( this->ImageFonts.array )
  {
    free(this->ImageFonts.array);
    this->ImageFonts.array = 0;
  }
  if ( this->Widgets.array )
  {
    free(this->Widgets.array);
    this->Widgets.array = 0;
  }
  // base destructor called automatically
  // base destructor called automatically
}

//----- (00484200) --------------------------------------------------------

void SCredits::Create(int a2)

{
  SCredits *v2; // ebx
  char *i; // esi
  double Float; // st7
  SProperties *CreditsIni; // ecx
  double v6; // st7
  SProperties *v7; // ecx
  double v8; // st7
  SProperties *v9; // ecx
  char *String; // eax
  SProperties *v11; // ecx
  const char *v12; // edi
  char *v13; // eax
  const char *v14; // edx
  char *v15; // ecx
  bool v16;
  bool v17;
  unsigned char v18; // bl
  int v19;
  int v20;
  int v21;
  int v22;
  int v23;
  int *v24; // eax
  int v25;
  int v26;
  int v28;
  int v29;
  int v30;
  char *v31; // eax
  const char *v32; // edx
  char *v33; // ecx
  bool v34;
  SCredits *v35; // ebx
  unsigned char v36; // bl
  int v37;
  int v38;
  int v39;
  SProperties *v40; // ecx
  char *v41; // eax
  const char *v42; // edx
  char *v43; // ecx
  bool v44;
  SCredits *v45; // ebx
  unsigned char v46; // bl
  int v47;
  float v48; // xmm1_4
  SHorizontalAlign v49;
  int v50;
  char *v51; // eax
  const char *v52; // edx
  char *v53; // ecx
  bool v54;
  unsigned char v55; // bl
  int v56;
  int v57;
  SVerticalAlign v58;
  STextBox *v59; // eax
  STextBox *v61; // edi
  int size;
  int maxsize;
  int v64;
  SDXWidget **v65; // eax
  int v66;
  int v67;
  int v68;
  int v69;
  int v70;
  int v71;
  char *text;
  int font;
  SHorizontalAlign hAlign;
  SVerticalAlign vAlign;
  float posY;
  float sizeX;
  float sizeY;
  float posX;
  SCredits *v82;
  char v83;
  v2 = this;
  v82 = this;
  SSkippableDXWidget::Create(a2, 275745, 1);
  Board->SetPixelRounding(0);
  v2->InsertChild(&v2->ContentParent);
  v2->ContentParent.SetPosition((v2->Width - 800) / 2, -500, 0, 0);
  v2->ContentParent.Create((int)&v2->ContentParent);
  v2->CreditsIni->EnumPropertyClasses();
  for ( i = v2->CreditsIni->GetNextPropertyClass(); i; i = v2->CreditsIni->GetNextPropertyClass() )
  {
  char fullpath[260] = {0};
    Float = v2->CreditsIni->GetFloat(i, "PosX", 0.0);
    CreditsIni = v2->CreditsIni;
    posX = (float)(Float);

    v6 = CreditsIni->GetFloat(i, "PosY", 0.0);
    v7 = v2->CreditsIni;
    posY = (float)(v6);

    v8 = v7->GetFloat(i, "SizeX", 0.0);
    v9 = v2->CreditsIni;
    sizeX = (float)(v8);

    sizeY = v9->GetFloat(i, "SizeY", 0.0);
    if ( (float)(sizeY + posY) > v2->ContentHeight )
      v2->ContentHeight = sizeY + posY;
    String = v2->CreditsIni->GetString(i, "Content", fullpath);
    v11 = v2->CreditsIni;
    v12 = String;
    text = String;
    v13 = v11->GetString(i, "Type", fullpath);
    v14 = "Image";
    v15 = v13;
    while ( 1 )
    {
      v16 = (unsigned char)*v15 < (unsigned int)*v14;
      v17 = *v15 == *v14;
      v83 = *v15;
      v2 = v82;
      if ( !v17 )
        break;
      if ( !v83 )
        goto LABEL_9;
      v18 = v15[1];
      v16 = v18 < (unsigned int)v14[1];
      v17 = v18 == v14[1];
      v83 = v18;
      v2 = v82;
      if ( !v17 )
        break;
      v15 += 2;
      v14 += 2;
      if ( !v83 )
      {
LABEL_9:
        v19 = 0;
        goto LABEL_11;
      }
    }
    v19 = v16 ? -1 : 1;
LABEL_11:
    if ( v19 )
    {
      v30 = strcmp(v13, "Text");
      if ( v30 )
        v30 = v30 < 0 ? -1 : 1;
      if ( !v30 )
      {
        v31 = v2->CreditsIni->GetString(i, "TextStyle", fullpath);
        v32 = "Title";
        v33 = v31;
        while ( 1 )
        {
          v34 = (unsigned char)*v33 < (unsigned int)*v32;
          v17 = *v33 == *v32;
          v83 = *v33;
          v35 = v82;
          if ( !v17 )
            break;
          if ( !v83 )
            goto LABEL_26;
          v36 = v33[1];
          v34 = v36 < (unsigned int)v32[1];
          v17 = v36 == v32[1];
          v83 = v36;
          v35 = v82;
          if ( !v17 )
            break;
          v33 += 2;
          v32 += 2;
          if ( !v83 )
          {
LABEL_26:
            v37 = 0;
            goto LABEL_28;
          }
        }
        v37 = v34 ? -1 : 1;
LABEL_28:
        v38 = 3;
        if ( v37 )
          v38 = 0;
        v39 = strcmp(v31, "Name");
        if ( v39 )
          v39 = v39 < 0 ? -1 : 1;
        v40 = v35->CreditsIni;
        if ( !v39 )
          v38 = 4;
        font = v38;
        v41 = v40->GetString(i, "HorizontalAlignment", fullpath);
        v42 = "Center";
        v43 = v41;
        while ( 1 )
        {
          v44 = (unsigned char)*v43 < (unsigned int)*v42;
          v17 = *v43 == *v42;
          v83 = *v43;
          v45 = v82;
          if ( !v17 )
            break;
          if ( !v83 )
            goto LABEL_39;
          v46 = v43[1];
          v44 = v46 < (unsigned int)v42[1];
          v17 = v46 == v42[1];
          v83 = v46;
          v45 = v82;
          if ( !v17 )
            break;
          v43 += 2;
          v42 += 2;
          if ( !v83 )
          {
LABEL_39:
            v47 = 0;
            goto LABEL_41;
          }
        }
        v47 = v44 ? -1 : 1;
LABEL_41:
        if ( v47 )
          v48 = posX;
        else
          v48 = (float)(sizeX * 0.5) + posX;
        posX = v48;
        v49 = ALIGN_CENTER;
        if ( v47 )
          v49 = ALIGN_LEFT;
        hAlign = v49;
        v50 = strcmp(v41, "Right");
        if ( v50 )
          v50 = v50 < 0 ? -1 : 1;
        if ( !v50 )
        {
          hAlign = ALIGN_RIGHT;
          posX = sizeX + v48;
        }
        v51 = v45->CreditsIni->GetString(i, "VerticalAlignment", fullpath);
        v52 = "Middle";
        v53 = v51;
        while ( 1 )
        {
          v54 = (unsigned char)*v53 < (unsigned int)*v52;
          v17 = *v53 == *v52;
          v83 = *v53;
          v2 = v82;
          if ( !v17 )
            break;
          if ( !v83 )
            goto LABEL_55;
          v55 = v53[1];
          v54 = v55 < (unsigned int)v52[1];
          v17 = v55 == v52[1];
          v83 = v55;
          v2 = v82;
          if ( !v17 )
            break;
          v53 += 2;
          v52 += 2;
          if ( !v83 )
          {
LABEL_55:
            v56 = 0;
            goto LABEL_57;
          }
        }
        v56 = v54 ? -1 : 1;
LABEL_57:
        vAlign = (SVerticalAlign)(v56 == 0);
        v57 = strcmp(v51, "Bottom");
        if ( v57 )
          v57 = v57 < 0 ? -1 : 1;
        v17 = v57 == 0;
        v58 = vAlign;
        if ( v17 )
          v58 = Bottom;
        vAlign = v58;
        v59 = new STextBox();
        v61 = v59;
        size = v2->Widgets.size;
        maxsize = v2->Widgets.maxsize;
        if ( size == maxsize )
        {
          if ( maxsize >= 16 )
            v64 = 6 * maxsize / 5;
          else
            v64 = 16;
          // x64: literal `4` is x86 sizeof(SDXWidget*); on x64 a pointer is 8.
          v65 = (SDXWidget **)realloc(v2->Widgets.array, sizeof(SDXWidget *) * v64);
          v66 = v2->Widgets.maxsize;
          v2->Widgets.array = v65;
          memset(&v65[v66], 0, sizeof(SDXWidget *) * (v64 - v66));
          size = v2->Widgets.size;
          v2->Widgets.maxsize = v64;
        }
        v2->Widgets.size = size + 1;
        v2->Widgets.array[size] = v61;
        v2->ContentParent.InsertChild(v61);
        v61->SetPosition((int)posX, (int)posY, (int)sizeX, (int)sizeY);
        v61->Create(font, 0xFFu, 0, 0, hAlign, vAlign);
        v61->SetText(text);
      }
    }
    else
    {
      v20 = Board->LoadSingleFont(v12, Default);
      v21 = v2->ImageFonts.maxsize;
      v22 = v20;
      if ( v2->ImageFonts.size == v21 )
      {
        if ( v21 >= 16 )
          v23 = 6 * v21 / 5;
        else
          v23 = 16;
        v24 = (int *)realloc(v2->ImageFonts.array, 4 * v23);
        v25 = v2->ImageFonts.maxsize;
        v2->ImageFonts.array = v24;
        memset(&v24[v25], 0, 4 * (v23 - v25));
        v2->ImageFonts.maxsize = v23;
      }
      v26 = v2->ImageFonts.size;
      v2->ImageFonts.size = v26 + 1;
      v2->ImageFonts.array[v26] = v22;
      v28 = v2->ContentParent.GetFrame();
      v29 = Board->CreateFrame(FT_SPRITE, v28, 0, 0, 0, 1);
      Board->SetSpriteGlyph(v29, v22, 0);
      Board->MoveFrame(v29, (int)posX, (int)posY);
      Board->ResizeFrame(v29, (int)sizeX, (int)sizeY);
    }
  }
  v2->BottomGradientFont = Board->LoadSingleFont("menu/gradient_bottom.png", Default);
  v67 = Board->CreateFrame(FT_SPRITE, v2->BackFrame, 0, 0, 0, 1);
  v2->BottomGradientBackground = v67;
  Board->SetSpriteGlyph(v67, v2->BottomGradientFont, 0);
  v2->TopGradientFont = Board->LoadSingleFont("menu/gradient_top.png", Default);
  v68 = Board->CreateFrame(FT_SPRITE, v2->BackFrame, 0, 0, 0, 1);
  v2->TopGradientBackground = v68;
  Board->SetSpriteGlyph(v68, v2->TopGradientFont, 0);
  v69 = Board->CreateFrame(FT_BOX, v2->BackFrame, 0, 0, 0, 1);
  v2->BottomFillBackground = v69;
  Board->SetBoxColor(v69, 0xFF000000);
  v70 = Board->CreateFrame(FT_BOX, v2->BackFrame, 0, 0, 0, 1);
  v2->TopFillBackground = v70;
  Board->SetBoxColor(v70, 0xFF000000);
  v2->InsertSkipButton();
  int p_x, p_y, p_w, p_h;
  v2->Parent->GetPosition(&p_x, &p_y, &p_w, &p_h);
  v2->SetPosition(0, 0, p_w, p_h);
  v71 = (v2->Height - 600) / 2;
  Board->ResizeFrame(v2->TopFillBackground, v2->Width, v71 + 1);
  Board->MoveFrame(v2->TopGradientBackground, 0, v71);
  Board->ResizeFrame(v2->TopGradientBackground, v2->Width, 150);
  Board->MoveFrame(v2->BottomGradientBackground, 0, v71 + 450);
  Board->ResizeFrame(v2->BottomGradientBackground, v2->Width, 150);
  Board->MoveFrame(v2->BottomFillBackground, 0, v71 + 599);
  Board->ResizeFrame(v2->BottomFillBackground, v2->Width, v71 + 1);
  v2->SetFocus();
  Concert->StartStreamingPlayback("music/credits.mp3", 0);
  v2->StartTick = (unsigned int)Timer.GetTickValue();

  v2->ElapsedMs = 0;
}

//----- (00484A00) --------------------------------------------------------

bool SCredits::OnKeyDown(int keycode, bool repeat)

{
  return SSkippableDXWidget::OnKeyDown(keycode) != 0;
}

//----- (00484A20) --------------------------------------------------------

bool SCredits::OnKeyUp(int keycode)

{
  return 0;
}

//----- (00484A50) --------------------------------------------------------

void SCredits::OnSize(int w, int h)

{
  int v2;
  int p_x;
  int p_y;
  int p_width;
  int p_height;
  this->Parent->GetPosition(&p_x, &p_y, &p_width, &p_height);
  this->SetPosition(0, 0, p_width, p_height);
  v2 = (this->Height - 600) / 2;
  Board->ResizeFrame(this->TopFillBackground, this->Width, v2 + 1);
  Board->MoveFrame(this->TopGradientBackground, 0, v2);
  Board->ResizeFrame(this->TopGradientBackground, this->Width, 150);
  Board->MoveFrame(this->BottomGradientBackground, 0, v2 + 450);
  Board->ResizeFrame(this->BottomGradientBackground, this->Width, 150);
  Board->MoveFrame(this->BottomFillBackground, 0, v2 + 599);
  Board->ResizeFrame(this->BottomFillBackground, this->Width, v2 + 1);
}

//----- (00484B50) --------------------------------------------------------

void SCredits::Update()

{
  int ElapsedMs;
  int NextSpeechTime;
  int NextSpeechIndex;
  SSkippableDXWidget::Update();
  if ( this->AutoScroll )
    this->ContentParent.SetPosition(
      (this->Width - 800) / 2,
      (int)(float)((float)(this->Height / 2 + 300)
                 - (float)((float)((float)this->ElapsedMs / (float)this->MusicDuration)
                         * (float)(this->ContentHeight + 600.0))),
      800,
      600);
  ElapsedMs = this->ElapsedMs;
  NextSpeechTime = this->NextSpeechTime;
  if ( ElapsedMs > NextSpeechTime )
  {
    NextSpeechIndex = this->NextSpeechIndex;
    if ( NextSpeechIndex < this->SpeechPaths.size )
    {
      this->NextSpeechIndex = NextSpeechIndex + 1;
      Concert->PlaySound(
        this->SpeechPaths.array[NextSpeechIndex],
        0,
        0,
        -1);
      NextSpeechTime = this->NextSpeechTime;
      ElapsedMs = this->ElapsedMs;
    }
    this->NextSpeechTime = NextSpeechTime + this->SpeechInterval;
  }
  if ( (float)ElapsedMs > this->MusicDuration )
    this->SendAction(275746, 0);
}
