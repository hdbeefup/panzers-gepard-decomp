// common/swineversion.cpp
// Version info
// Decompiled from: gameSplit/sversion.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "swineversion.h"
#include "logger.h"

// Classes: SVersion
// Function count: 12

//----- (0046D960) --------------------------------------------------------

SVersion::SVersion(int versionInt)
{
  this->VersionType = (SwineVersionType)BYTE3(versionInt);
  this->MajorVersion = BYTE2(versionInt);
  this->MinorVersion = (unsigned char)versionInt;
}

//----- (0046D990) --------------------------------------------------------

SVersion::SVersion(SString networkString)
{
  const char* str = networkString.buf ? networkString.buf : "";
  sscanf(str, "%d_%d_%d", (int*)&this->VersionType, &this->MajorVersion, &this->MinorVersion);
  if ( networkString.buf )
    delete[] networkString.buf;
}

//----- (0046D9D0) --------------------------------------------------------

SVersion::SVersion(SwineVersionType versionType, int majorVersion, int minorVersion)
{
  this->MajorVersion = majorVersion;
  this->MinorVersion = minorVersion;
  this->VersionType = versionType;
}

//----- (0046D9F0) --------------------------------------------------------

SVersion::SVersion()
{
  this->MajorVersion = 1;
  this->MinorVersion = 7;
  this->VersionType = Gold;
}

//----- (0046DA10) --------------------------------------------------------

double SVersion::GetFloat()
{
  SString result;
  result.buf = 0;
  result.size = 0;
  setlocale(LC_NUMERIC, "C");
  Format(&result, "%d.%d", this->MajorVersion, this->MinorVersion);
  const char* str = result.buf ? result.buf : "";
  float v7 = (float)atof(str);
  if ( result.buf )
    delete[] result.buf;
  return v7;
}

//----- (0046DA70) --------------------------------------------------------

int SVersion::GetInt()
{
  return this->MinorVersion | ((this->MajorVersion | (this->VersionType << 8)) << 16);
}

//----- (0046DA80) --------------------------------------------------------

SString SVersion::GetLongString()
{
  SString result;
  result.buf = 0;
  result.size = 0;

  char *v4 = 0;
  const char *v5;

  SwineVersionType VersionType = this->VersionType;
  if ( VersionType == Gold )
  {
    v4 = new char[12];
    strcpy(v4, "HD Remaster");
  }
  else if ( VersionType == Developer )
  {
    v4 = new char[10];
    strcpy(v4, "Developer");
  }

  v5 = v4 ? v4 : "";

  Format(&result, "S.W.I.N.E. %s v%d.%d.%d", v5, this->MajorVersion, this->MinorVersion, 1846);
  if ( v4 )
    delete[] v4;
  return result;
}

//----- (0046DB80) --------------------------------------------------------

SString SVersion::GetNetworkString()
{
  SString result;
  result.buf = 0;
  result.size = 0;
  Format(&result, "%d_%d_%d", this->VersionType, this->MajorVersion, this->MinorVersion);
  return result;
}

//----- (0046DBB0) --------------------------------------------------------

SString SVersion::GetShortString()
{
  SString result;
  result.buf = 0;
  result.size = 0;
  Format(&result, "%d.%d", this->MajorVersion, this->MinorVersion);
  return result;
}

//----- (0046DBD0) --------------------------------------------------------

bool SVersion::GetVersionTypeFromString(char *strVersionType)
{
  return _stricmp(strVersionType, "Developer") && !_stricmp(strVersionType, "HD Remaster");
}

//----- (0046DC10) --------------------------------------------------------

SString SVersion::GetVersionTypeString()
{
  SString result;
  result.buf = 0;
  result.size = 0;
  SwineVersionType VersionType = this->VersionType;
  if ( VersionType == Developer )
  {
    result.size = 9;
    char* v3 = new char[10];
    result.buf = v3;
    memcpy(v3, "Developer", result.size + 1);
  }
  else if ( VersionType == Gold )
  {
    result.size = 11;
    char* v3 = new char[12];
    result.buf = v3;
    memcpy(v3, "HD Remaster", result.size + 1);
  }
  return result;
}

//----- (0046DCC0) --------------------------------------------------------

bool SVersion::IsGoodVersion(SVersion otherVersion)
{
  bool result = this->VersionType == otherVersion.VersionType
      && this->MajorVersion == otherVersion.MajorVersion
      && this->MinorVersion == otherVersion.MinorVersion;
  Logger.g->Log(0, "IsGoodVersion: this={%d,%d,%d} other={%d,%d,%d} => %d",
      this->VersionType, this->MajorVersion, this->MinorVersion,
      otherVersion.VersionType, otherVersion.MajorVersion, otherVersion.MinorVersion,
      result);
  return result;
}
