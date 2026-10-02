// common/options.cpp
// Game options and settings
// Decompiled from: gameSplit/soptions.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "options.h"
#ifdef HDB_MODLOADER_SYSTEM
#include "modmanager.h"
#endif

// Classes: SOptions
// Function count: 79

//----- (0046B900) --------------------------------------------------------

SOptions::SOptions()
{
  this->PlayerName.buf = 0;
  this->PlayerName.size = 0;
  this->LastHostIP.buf = 0;
  this->LastHostIP.size = 0;
  this->keyboardmode = 0;

  const char* path = FileSystem.MakeAppDataPath("options.ini");
  this->OptionsIni = new SProperties(path, 0, 1);

  CurrentLanguage = this->OptionsIni->GetInt("Language settings", "Language", 0);
  int Int = this->OptionsIni->GetInt("Game settings", "Mouse scroll speed", 5);
  this->MouseScrollSpeed = (float)((double)(Int - 5) * 0.002f + 0.025f);

  int v7 = this->OptionsIni->GetInt("Game settings", "Keyboard scroll speed", 5);
  this->KeyboardScrollSpeed = (float)((double)(v7 - 5) * 0.002f + 0.02f);

  this->MouseRestriction = this->OptionsIni->GetInt("Game settings", "Mouse restriction", 1);
  this->ShowTipsAtStartup = this->OptionsIni->GetInt("Game settings", "Show tips at startup", 1);
  this->Subtitles = this->OptionsIni->GetInt("Game settings", "Subtitles", 2);
  this->FogOfWarView = this->OptionsIni->GetInt("Game settings", "Fog of war view", 1);
  this->UnitAcknowledgement = this->OptionsIni->GetInt("Game settings", "Unit acknowledgement", 2);
  this->OtherUnitVoice = this->OptionsIni->GetInt("Game settings", "Other unit voice", 2);
  this->Monitor = this->OptionsIni->GetInt("Graphics settings", "Monitor", 0);
  this->DisplayMode = this->OptionsIni->GetInt("Graphics settings", "Display mode", 1);
  this->ScreenResolution = this->OptionsIni->GetInt("Graphics settings", "Screen resolution", 1);
  int v25 = this->OptionsIni->GetInt("Graphics settings", "Resolution scale", 1000);
  this->ResolutionScale = (int)(float)((float)((float)v25 / 1000.0f) * 1000.0f);
  this->VSync = this->OptionsIni->GetInt("Graphics settings", "VSync", 1);
  this->CursorMode = this->OptionsIni->GetInt("Graphics settings", "Cursor mode", 0);
  this->Antialiasing = this->OptionsIni->GetInt("Graphics settings", "Antialiasing", 2);
  this->AnisotropicFiltering = this->OptionsIni->GetInt("Graphics settings", "Anisotropic filtering", 1);
  this->Shadows = this->OptionsIni->GetInt("Graphics settings", "Shadows", 3);
  this->TexturesDetail = this->OptionsIni->GetInt("Graphics settings", "Textures detail", 1);
  this->MusicVolume = this->OptionsIni->GetInt("Audio settings", "Music volume", 7);
  this->SoundEffectVolume = this->OptionsIni->GetInt("Audio settings", "Sound effect volume", 8);
  this->VoiceVolume = this->OptionsIni->GetInt("Audio settings", "Voice volume", 8);
  this->ReverseChannels = this->OptionsIni->GetInt("Audio settings", "Reverse channels", 0);
  this->PlaySoundInBackGround = this->OptionsIni->GetInt("Audio settings", "Play sound in background", 1);
  this->StartSp = this->OptionsIni->GetInt("Multiplayer settings", "Start SP limit", 0);
  this->PeriodicSp = this->OptionsIni->GetInt("Multiplayer settings", "Periodic SP limit", 0);
  this->GameType = (unsigned char)this->OptionsIni->GetInt("Multiplayer settings", "Game type", 0);
  this->CountMinute = (unsigned char)this->OptionsIni->GetInt("Multiplayer settings", "Countdown minute", 0);
  this->equipment = this->OptionsIni->GetInt("Multiplayer settings", "Equipment", 1) != 0;
  this->limitedammo = this->OptionsIni->GetInt("Multiplayer settings", "Limited ammo", 1) != 0;
  this->limitedfuel = this->OptionsIni->GetInt("Multiplayer settings", "Limited fuel", 1) != 0;
  this->movingforce = this->OptionsIni->GetInt("Multiplayer settings", "Moving Fortress", 1) != 0;
  this->bomber = this->OptionsIni->GetInt("Multiplayer settings", "Bomber", 1) != 0;
  this->buyingingame = this->OptionsIni->GetInt("Multiplayer settings", "Buying in game", 1) != 0;
  this->randomstartposition = this->OptionsIni->GetInt("Multiplayer settings", "Random start position", 1) != 0;
  this->domination = this->OptionsIni->GetInt("Multiplayer settings", "Domination", 0) != 0;
  char* String = this->OptionsIni->GetString("Multiplayer settings", "Player Name", "");
  this->PlayerName = String;
  char* v72 = this->OptionsIni->GetString("Multiplayer settings", "Last host IP", "");
  this->LastHostIP = v72;

#ifdef HD_HDBEEFUP_SETTINGS
  this->CameraRotation = this->OptionsIni->GetInt("HDBeefup settings", "Camera rotation", 1) != 0;
  this->EnableCheats = this->OptionsIni->GetInt("HDBeefup settings", "Enable cheats", 0) != 0;
  this->MapBorder = this->OptionsIni->GetInt("HDBeefup settings", "Map border", 1) != 0;
  this->SkipIntro = this->OptionsIni->GetInt("Developer", "SkipIntro", 0) != 0;
  this->FastMenu  = this->OptionsIni->GetInt("Developer", "FastMenu",  0) != 0;
#endif

#ifdef HDB_MODLOADER_SYSTEM
  this->ActiveMods = nullptr;
  this->ActiveModCount = 0;
  this->ActiveModCapacity = 0;
  // Parse "Active mods = A;B;C" into ActiveMods[] (index 0 = highest priority).
  // Filters out empty entries and unknown folder names so a renamed/removed
  // mod silently drops out instead of breaking the whole list.
  const char* raw = this->OptionsIni->GetString("HDBeefup settings", "Active mods", "");
  if (raw && raw[0]) {
    SString working;
    working = raw;                                  // safe deep copy
    char* p = working.buf;
    char* names[64];
    int nameCount = 0;
    while (p && *p && nameCount < 64) {
      while (*p == ' ' || *p == '\t') ++p;
      char* start = p;
      while (*p && *p != ';') ++p;
      char* end = p;
      while (end > start && (end[-1] == ' ' || end[-1] == '\t')) --end;
      char term = *p;
      *end = 0;
      if (*start && ModManager.FindModIndex(start) >= 0) {
        names[nameCount++] = start;
      }
      if (term == ';') ++p; else break;
    }
    if (nameCount > 0) {
      this->ActiveMods = (SString*)calloc(nameCount, sizeof(SString));
      this->ActiveModCapacity = nameCount;
      for (int i = 0; i < nameCount; ++i) {
        this->ActiveMods[i] = names[i];
        ++this->ActiveModCount;
      }
    }
  }
  // Push to ModManager (handles empty list as "base game only").
  {
    const char* tmp[64];
    int n = this->ActiveModCount > 64 ? 64 : this->ActiveModCount;
    for (int i = 0; i < n; ++i)
      tmp[i] = this->ActiveMods[i].buf ? this->ActiveMods[i].buf : "";
    ModManager.SetActiveMods(tmp, n);
  }
  const char* langPack = this->OptionsIni->GetString("Language settings", "LangPack", "");
  if (langPack && langPack[0]) {
    this->LanguageLangPack = langPack;
    ModManager.SetActiveLangPack(langPack);
  }
#endif

#ifdef HD_KEYBINDS
  this->ControlScheme = this->OptionsIni->GetInt("HDBeefup settings", "Control scheme", CS_CLASSIC);
  this->CustomBindings.LoadFromIni(this->OptionsIni);
  this->RefreshActiveBindings();
#endif
}

//----- (0046BD90) --------------------------------------------------------

SOptions::~SOptions()
{
  if ( this->OptionsIni )
  {
    delete this->OptionsIni;
    this->OptionsIni = 0;
  }
  if ( this->LastHostIP.buf )
  {
    delete[] this->LastHostIP.buf;
    this->LastHostIP.buf = 0;
  }
  if ( this->PlayerName.buf )
  {
    delete[] this->PlayerName.buf;
    this->PlayerName.buf = 0;
  }
#ifdef HDB_MODLOADER_SYSTEM
  if ( this->ActiveMods )
  {
    for (int i = 0; i < this->ActiveModCount; ++i) {
      if (this->ActiveMods[i].buf) {
        delete[] this->ActiveMods[i].buf;
        this->ActiveMods[i].buf = 0;
      }
    }
    free(this->ActiveMods);
    this->ActiveMods = 0;
    this->ActiveModCount = 0;
    this->ActiveModCapacity = 0;
  }
  if ( this->LanguageLangPack.buf )
  {
    delete[] this->LanguageLangPack.buf;
    this->LanguageLangPack.buf = 0;
  }
#endif
}

//----- (0046BE60) --------------------------------------------------------

int SOptions::GetAnisotropicFiltering()
{
  return this->AnisotropicFiltering;
}

//----- (0046BE70) --------------------------------------------------------

int SOptions::GetAntialiasing()
{
  return this->Antialiasing;
}

//----- (0046BE80) --------------------------------------------------------

unsigned char SOptions::GetCountMinute()
{
  return this->CountMinute;
}

//----- (0046BE90) --------------------------------------------------------

int SOptions::GetCursorMode()
{
  return this->CursorMode;
}

//----- (0046BEA0) --------------------------------------------------------

int SOptions::GetDisplayMode()
{
  return this->DisplayMode;
}

//----- (0046BEB0) --------------------------------------------------------

int SOptions::GetFogOfWarView()
{
  return this->FogOfWarView;
}

//----- (0046BEC0) --------------------------------------------------------

unsigned char SOptions::GetGameType()
{
  return 0;
}

//----- (0046BED0) --------------------------------------------------------

bool SOptions::GetKeyboardMode()
{
  return this->keyboardmode;
}

//----- (0046BEE0) --------------------------------------------------------

float SOptions::GetKeyboardScrollSpeed()
{
  return this->KeyboardScrollSpeed;
}

//----- (0046BEF0) --------------------------------------------------------

char *SOptions::GetLastHostIP()
{
  char *buf = this->LastHostIP.buf;
  if ( buf )
    return buf;
  return (char*)"";
}

//----- (0046BF00) --------------------------------------------------------

int SOptions::GetMonitor()
{
  return this->Monitor;
}

//----- (0046BF10) --------------------------------------------------------

int SOptions::GetMouseRestriction()
{
  return this->MouseRestriction;
}

//----- (0046BF20) --------------------------------------------------------

float SOptions::GetMouseScrollSpeed()
{
  return this->MouseScrollSpeed;
}

//----- (0046BF30) --------------------------------------------------------

int SOptions::GetMusicVolume()
{
  return this->MusicVolume;
}

//----- (0046BF40) --------------------------------------------------------

int SOptions::GetOtherUnitVoice()
{
  return this->OtherUnitVoice;
}

//----- (0046BF50) --------------------------------------------------------

int SOptions::GetPeriodicSp()
{
  return this->PeriodicSp;
}

//----- (0046BF60) --------------------------------------------------------

int SOptions::GetPlaySoundInBackground()
{
  return this->PlaySoundInBackGround;
}

//----- (0046BF70) --------------------------------------------------------

char *SOptions::GetPlayerName()
{
  char *buf = this->PlayerName.buf;
  if ( buf )
    return buf;
  return (char*)"";
}

//----- (0046BF80) --------------------------------------------------------

float SOptions::GetResolutionScale()
{
  float scale = (float)this->ResolutionScale / 1000.0f;
  if ( scale >= 2.0f )
    scale = 2.0f;
  if ( scale > 0.5f )
    return scale;
  return 0.5f;
}

//----- (0046BFE0) --------------------------------------------------------

int SOptions::GetReverseChannels()
{
  return this->ReverseChannels;
}

//----- (0046BFF0) --------------------------------------------------------

int SOptions::GetScreenResolution()
{
  return this->ScreenResolution;
}

//----- (0046C000) --------------------------------------------------------

int SOptions::GetShadows()
{
  return this->Shadows;
}

//----- (0046C010) --------------------------------------------------------

int SOptions::GetShowTipsAtStartup()
{
  return this->ShowTipsAtStartup;
}

//----- (0046C020) --------------------------------------------------------

int SOptions::GetSoundEffectVolume()
{
  return this->SoundEffectVolume;
}

//----- (0046C030) --------------------------------------------------------

int SOptions::GetStartSp()
{
  return this->StartSp;
}

//----- (0046C040) --------------------------------------------------------

int SOptions::GetSubtitles()
{
  return this->Subtitles;
}

//----- (0046C050) --------------------------------------------------------

int SOptions::GetTexturesDetail()
{
  return this->TexturesDetail;
}

//----- (0046C060) --------------------------------------------------------

int SOptions::GetUnitAcknowledgement()
{
  return this->UnitAcknowledgement;
}

//----- (0046C070) --------------------------------------------------------

int SOptions::GetVSync()
{
  return this->VSync;
}

//----- (0046C080) --------------------------------------------------------

int SOptions::GetVoiceVolume()
{
  return this->VoiceVolume;
}

//----- (0046C090) --------------------------------------------------------

void SOptions::SetAnisotropicFiltering(int num)
{
  this->AnisotropicFiltering = num;
}

//----- (0046C0A0) --------------------------------------------------------

void SOptions::SetAntialiasing(int num)
{
  this->Antialiasing = num;
}

//----- (0046C0B0) --------------------------------------------------------

void SOptions::SetBomberEnabled(bool enabled)
{
  this->bomber = enabled;
}

//----- (0046C0C0) --------------------------------------------------------

void SOptions::SetBuyingingameEnabled(bool enabled)
{
  this->buyingingame = enabled;
}

//----- (0046C0D0) --------------------------------------------------------

void SOptions::SetCountMinute(unsigned char countMinute)
{
  this->CountMinute = countMinute;
}

//----- (0046C0E0) --------------------------------------------------------

void SOptions::SetCursorMode(int num)
{
  this->CursorMode = num;
}

//----- (0046C0F0) --------------------------------------------------------

void SOptions::SetDisplayMode(int num)
{
  this->DisplayMode = num;
}

//----- (0046C100) --------------------------------------------------------

void SOptions::SetDominationEnabled(bool enabled)
{
  this->domination = enabled;
}

//----- (0046C110) --------------------------------------------------------

void SOptions::SetEquipmentEnabled(bool enabled)
{
  this->equipment = enabled;
}

//----- (0046C120) --------------------------------------------------------

void SOptions::SetFogOfWarView(int num)
{
  this->FogOfWarView = num;
}

//----- (0046C130) --------------------------------------------------------

void SOptions::SetGameType(unsigned char gameType)
{
  this->GameType = gameType;
}

//----- (0046C140) --------------------------------------------------------

void SOptions::SetKeyboardMode(bool on)
{
  this->keyboardmode = on;
}

//----- (0046C150) --------------------------------------------------------

void SOptions::SetKeyboardScrollSpeed(int num)
{
  this->KeyboardScrollSpeed = (float)((double)(num - 5) * 0.002f + 0.02f);

}

//----- (0046C180) --------------------------------------------------------

void SOptions::SetLastHostIP(const char *str)
{
  this->LastHostIP = str;
}

//----- (0046C190) --------------------------------------------------------

void SOptions::SetLimitedammoEnabled(bool enabled)
{
  this->limitedammo = enabled;
}

//----- (0046C1A0) --------------------------------------------------------

void SOptions::SetLimitedfuelEnabled(bool enabled)
{
  this->limitedfuel = enabled;
}

//----- (0046C1B0) --------------------------------------------------------

void SOptions::SetMonitor(int num)
{
  this->Monitor = num;
}

//----- (0046C1C0) --------------------------------------------------------

void SOptions::SetMouseRestriction(int num)
{
  this->MouseRestriction = num;
}

//----- (0046C1D0) --------------------------------------------------------

void SOptions::SetMouseScrollSpeed(int num)
{
  this->MouseScrollSpeed = (float)((double)(num - 5) * 0.002f + 0.025f);

}

//----- (0046C200) --------------------------------------------------------

void SOptions::SetMovingforceEnabled(bool enabled)
{
  this->movingforce = enabled;
}

//----- (0046C210) --------------------------------------------------------

void SOptions::SetMusicVolume(int num)
{
  this->MusicVolume = num;
}

//----- (0046C220) --------------------------------------------------------

void SOptions::SetOtherUnitVoice(int num)
{
  this->OtherUnitVoice = num;
}

//----- (0046C230) --------------------------------------------------------

void SOptions::SetPeriodicSp(int periodicSp)
{
  this->PeriodicSp = periodicSp;
}

//----- (0046C240) --------------------------------------------------------

void SOptions::SetPlaySoundInBackground(int num)
{
  this->PlaySoundInBackGround = num;
}

//----- (0046C250) --------------------------------------------------------

void SOptions::SetPlayerName(const char *str)
{
  this->PlayerName = str;
}

//----- (0046C260) --------------------------------------------------------

void SOptions::SetRandomStartPositionEnabled(bool enabled)
{
  this->randomstartposition = enabled;
}

//----- (0046C270) --------------------------------------------------------

void SOptions::SetResolutionScale(float num)
{
  this->ResolutionScale = (int)(float)(num * 1000.0f);
}

//----- (0046C290) --------------------------------------------------------

void SOptions::SetReverseChannels(int num)
{
  this->ReverseChannels = num;
}

//----- (0046C2A0) --------------------------------------------------------

void SOptions::SetScreenResolution(int num)
{
  this->ScreenResolution = num;
}

//----- (0046C2B0) --------------------------------------------------------

void SOptions::SetShadows(int num)
{
  this->Shadows = num;
}

//----- (0046C2C0) --------------------------------------------------------

void SOptions::SetShowTipsAtStartup(int num)
{
  this->ShowTipsAtStartup = num;
}

//----- (0046C2D0) --------------------------------------------------------

void SOptions::SetSoundEffectVolume(int num)
{
  this->SoundEffectVolume = num;
}

//----- (0046C2E0) --------------------------------------------------------

void SOptions::SetStartSp(int startSp)
{
  this->StartSp = startSp;
}

//----- (0046C2F0) --------------------------------------------------------

void SOptions::SetSubtitles(int num)
{
  this->Subtitles = num;
}

//----- (0046C300) --------------------------------------------------------

void SOptions::SetTexturesDetail(int num)
{
  this->TexturesDetail = num;
}

//----- (0046C310) --------------------------------------------------------

void SOptions::SetUnitAcknowledgement(int num)
{
  this->UnitAcknowledgement = num;
}

//----- (0046C320) --------------------------------------------------------

void SOptions::SetVSync(int num)
{
  this->VSync = num;
}

//----- (0046C330) --------------------------------------------------------

void SOptions::SetVoiceVolume(int num)
{
  this->VoiceVolume = num;
}

//----- (0046C350) --------------------------------------------------------

int SOptions::WriteOptionsIni()
{
  char buf[260];

  char* AppDataPath = (char*)FileSystem.MakeAppDataPath("options.ini");
  strcpy(buf, AppDataPath);

  SStream *is = FileSystem.OpenWrite(buf, 0);
  if ( !is )
    return 0;

  strcpy(buf, "[Language settings]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 - English\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 1 - German\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 2 - French\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 3 - Hungarian\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 4 - Czech\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 5 - Spanish\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 6 - Italian\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 7 - Russian\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 8 - Chinese\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Language = %d\r\n\r\n", CurrentLanguage);
  is->Write(buf, (int)strlen(buf));
#ifdef HDB_MODLOADER_SYSTEM
  strcpy(buf, "; Fan-made langpack folder code under langpack/ (empty = use Language above)\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "LangPack = %s\r\n\r\n",
          this->LanguageLangPack.buf ? this->LanguageLangPack.buf : "");
  is->Write(buf, (int)strlen(buf));
#endif
  strcpy(buf, "\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "[Game settings]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0-10\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Mouse scroll speed = %d\r\n\r\n", (int)((this->MouseScrollSpeed - 0.025) / 0.002 + 5.0 + 0.5));
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0-10\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Keyboard scroll speed = %d\r\n\r\n", (int)((this->KeyboardScrollSpeed - 0.02) / 0.002 + 5.0 + 0.5));
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = disabled, 1 = enabled\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Mouse restriction = %d\r\n\r\n", this->MouseRestriction);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Show tips at startup = %d\r\n\r\n", this->ShowTipsAtStartup);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = no subtitles, 1 = only in dialogs, 2 = all subtitles\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Subtitles = %d\r\n\r\n", this->Subtitles);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = normal, 2 = green\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Fog of war view = %d\r\n\r\n", this->FogOfWarView);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = no, 1 = only voice, 2 = video and voice\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Unit acknowledgement = %d\r\n\r\n", this->UnitAcknowledgement);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = no, 1 = only voice, 2 = video and voice\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Other unit voice = %d\r\n\r\n\r\n\r\n", this->OtherUnitVoice);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "[Graphics settings]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; value represents display index\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Monitor = %d\r\n\r\n", this->Monitor);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = 800x600, 1 = 1024x768, 2 = 1280x1024, 3 = 1600x1200\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Screen resolution = %d\r\n\r\n", this->ScreenResolution);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 500 = 0.5x, 1000 = 1.0x, 2000 = 2.0x, etc clamped to the range 0.5-2.0\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Resolution scale = %d\r\n\r\n", this->ResolutionScale);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = windowed, 1 = borderless windowed, 2 = fullscreen\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Display mode = %d\r\n\r\n", this->DisplayMode);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "VSync = %d\r\n\r\n", this->VSync);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = MSAA 2x, 2 = MSAA 4x, 3 = MSAA 8x\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Antialiasing = %d\r\n\r\n", this->Antialiasing);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = disabled, 1 = enabled\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Anisotropic filtering = %d\r\n\r\n", this->AnisotropicFiltering);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = terrain only, 2 = object self shadows 2048, 3 = object self shadows 4096\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Shadows = %d\r\n\r\n", this->Shadows);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = low, 1 = high\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Textures detail = %d\r\n\r\n\r\n\r\n", this->TexturesDetail);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "[Audio settings]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0-10\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Music volume = %d\r\n\r\n", this->MusicVolume);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0-10\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Sound effect volume = %d\r\n\r\n", this->SoundEffectVolume);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0-10\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Voice volume = %d\r\n\r\n", this->VoiceVolume);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Reverse channels = %d\r\n\r\n", this->ReverseChannels);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Play sound in background = %d\r\n\r\n\r\n\r\n", this->PlaySoundInBackGround);
  is->Write(buf, (int)strlen(buf));
  strcpy(buf, "[Multiplayer settings]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0-2\r\nStart SP limit = %d\r\n\r\n", this->StartSp);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0-2\r\nPeriodic SP limit = %d\r\n\r\n", this->PeriodicSp);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0-%d\r\nCountdown minute = %u\r\n\r\n", 5, this->CountMinute);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Equipment = %d\r\n\r\n", this->equipment);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Limited ammo = %d\r\n\r\n", this->limitedammo);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Limited fuel = %d\r\n\r\n", this->limitedfuel);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Moving Fortress = %d\r\n\r\n", this->movingforce);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Bomber = %d\r\n\r\n", this->bomber);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Buying in game = %d\r\n\r\n", this->buyingingame);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Random start position = %d\r\n\r\n", this->randomstartposition);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Domination = %d\r\n\r\n", this->domination);
  is->Write(buf, (int)strlen(buf));
  const char* playerName = this->PlayerName.buf ? this->PlayerName.buf : "";
  sprintf(buf, "Player name = %s\r\n\r\n", playerName);
  is->Write(buf, (int)strlen(buf));
  const char* lastHostIP = this->LastHostIP.buf ? this->LastHostIP.buf : "";
  sprintf(buf, "Last host IP = %s\r\n\r\n\r\n\r\n", lastHostIP);
  is->Write(buf, (int)strlen(buf));

#ifdef HD_HDBEEFUP_SETTINGS
  strcpy(buf, "[HDBeefup settings]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Camera rotation = %d\r\n\r\n", this->CameraRotation ? 1 : 0);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = on\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Enable cheats = %d\r\n\r\n", this->EnableCheats ? 1 : 0);
  is->Write(buf, (int)strlen(buf));
#ifdef HDB_MODLOADER_SYSTEM
  sprintf(buf, "; Active mods, semicolon-separated, leftmost = highest priority (empty = Base Game)\r\n");
  is->Write(buf, (int)strlen(buf));
  // Build the joined string in a heap buffer to avoid overflowing `buf`
  // when many mods are active. Size accounts for: "Active mods = " (14) +
  // each name's strlen + (N-1) ';' separators + "\r\n\r\n" (4) + the
  // null sprintf writes when emitting the suffix (1).
  {
    int needed = 14 + 4 + 1;
    for (int i = 0; i < this->ActiveModCount; ++i) {
      const char* n = this->ActiveMods[i].buf ? this->ActiveMods[i].buf : "";
      needed += (int)strlen(n);
      if (i > 0) ++needed;          // ';' separator only between names
    }
    char* line = (char*)malloc(needed);
    int pos = sprintf(line, "Active mods = ");
    for (int i = 0; i < this->ActiveModCount; ++i) {
      const char* n = this->ActiveMods[i].buf ? this->ActiveMods[i].buf : "";
      if (i > 0) line[pos++] = ';';
      int ln = (int)strlen(n);
      memcpy(line + pos, n, ln);
      pos += ln;
    }
    pos += sprintf(line + pos, "\r\n\r\n");
    is->Write(line, pos);
    free(line);
  }
#endif
  sprintf(buf, "; 0 = off (2001-style), 1 = on (HD map border texture)\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Map border = %d\r\n\r\n", this->MapBorder ? 1 : 0);
  is->Write(buf, (int)strlen(buf));
#endif

#ifdef HD_KEYBINDS
  sprintf(buf, "; 0 = Classic, 1 = WASD, 2 = Custom\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "Control scheme = %d\r\n\r\n", this->ControlScheme);
  is->Write(buf, (int)strlen(buf));
  this->CustomBindings.WriteToStream(is);
#endif

#ifdef HD_HDBEEFUP_SETTINGS
  strcpy(buf, "[Developer]\r\n\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = skip disclaimer + logo video + assemble/splash screens\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "SkipIntro = %d\r\n\r\n", this->SkipIntro ? 1 : 0);
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "; 0 = off, 1 = skip 3D menu_background.scene\r\n");
  is->Write(buf, (int)strlen(buf));
  sprintf(buf, "FastMenu = %d\r\n\r\n", this->FastMenu ? 1 : 0);
  is->Write(buf, (int)strlen(buf));
#endif

  is->Release();
  return 1;
}

#ifdef HD_HDBEEFUP_SETTINGS
bool SOptions::GetCameraRotation() { return this->CameraRotation; }
bool SOptions::GetEnableCheats() { return this->EnableCheats; }
bool SOptions::GetMapBorder() { return this->MapBorder; }
void SOptions::SetCameraRotation(bool enabled) { this->CameraRotation = enabled; }
void SOptions::SetEnableCheats(bool enabled) { this->EnableCheats = enabled; }
void SOptions::SetMapBorder(bool enabled) { this->MapBorder = enabled; }
#endif

#ifdef HDB_MODLOADER_SYSTEM
int SOptions::GetActiveModCount() const
{
    return this->ActiveModCount;
}

const char* SOptions::GetActiveMod(int index) const
{
    if (index < 0 || index >= this->ActiveModCount) return "";
    return this->ActiveMods[index].buf ? this->ActiveMods[index].buf : "";
}

void SOptions::SetActiveMods(const char* const* names, int count)
{
    // Free old entries (full reset, not in-place).
    if (this->ActiveMods) {
        for (int i = 0; i < this->ActiveModCount; ++i) {
            if (this->ActiveMods[i].buf) {
                delete[] this->ActiveMods[i].buf;
                this->ActiveMods[i].buf = 0;
            }
        }
        free(this->ActiveMods);
        this->ActiveMods = 0;
        this->ActiveModCapacity = 0;
    }
    this->ActiveModCount = 0;
    if (count > 0 && names) {
        this->ActiveMods = (SString*)calloc(count, sizeof(SString));
        this->ActiveModCapacity = count;
        for (int i = 0; i < count; ++i) {
            if (!names[i] || !names[i][0]) continue;
            this->ActiveMods[this->ActiveModCount] = names[i];
            ++this->ActiveModCount;
        }
    }
    // Push to ModManager so it can update its own active stack + .unit list.
    const char* tmp[64];
    int n = this->ActiveModCount > 64 ? 64 : this->ActiveModCount;
    for (int i = 0; i < n; ++i)
        tmp[i] = this->ActiveMods[i].buf ? this->ActiveMods[i].buf : "";
    ModManager.SetActiveMods(tmp, n);
}
#endif

#ifdef HD_KEYBINDS
int  SOptions::GetControlScheme()         { return this->ControlScheme; }
void SOptions::SetControlScheme(int s)    { this->ControlScheme = s; RefreshActiveBindings(); }

void SOptions::RefreshActiveBindings()
{
    if (this->ControlScheme == CS_CUSTOM)
        this->ActiveBindings = this->CustomBindings;
    else
        this->ActiveBindings.LoadPreset(this->ControlScheme);
}
#endif

//----- (0046D8E0) --------------------------------------------------------

bool SOptions::isBomberEnabled()
{
  return this->bomber;
}

//----- (0046D8F0) --------------------------------------------------------

bool SOptions::isBuyingingameEnabled()
{
  return this->buyingingame;
}

//----- (0046D900) --------------------------------------------------------

bool SOptions::isDominationEnabled()
{
  return this->domination;
}

//----- (0046D910) --------------------------------------------------------

bool SOptions::isEquipmentEnabled()
{
  return this->equipment;
}

//----- (0046D920) --------------------------------------------------------

bool SOptions::isLimitedFuelEnabled()
{
  return this->limitedfuel;
}

//----- (0046D930) --------------------------------------------------------

bool SOptions::isLimitedammoEnabled()
{
  return this->limitedammo;
}

//----- (0046D940) --------------------------------------------------------

bool SOptions::isMovingforceEnabled()
{
  return this->movingforce;
}

//----- (0046D950) --------------------------------------------------------

bool SOptions::isRandomStartPositionEnabled()
{
  return this->randomstartposition;
}
