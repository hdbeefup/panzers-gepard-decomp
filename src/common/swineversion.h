// common/swineversion.h
// SVersion — game version info
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef COMMON_SWINEVERSION_H
#define COMMON_SWINEVERSION_H

#include "core_common.h"
#include "string2.h"

enum SwineVersionType {
    Developer = 0,
    Gold = 1
};

SString* Format(SString* result, const char* fmt, ...);

struct SVersion {
    int MajorVersion;
    int MinorVersion;
    SwineVersionType VersionType;

    SVersion();
    SVersion(int versionInt);
    SVersion(SString networkString);
    SVersion(SwineVersionType versionType, int majorVersion, int minorVersion);

    double GetFloat();
    int GetInt();
    SString GetLongString();
    SString GetNetworkString();
    SString GetShortString();
    bool GetVersionTypeFromString(char* strVersionType);
    SString GetVersionTypeString();
    bool IsGoodVersion(SVersion otherVersion);
};

#endif // COMMON_SWINEVERSION_H
