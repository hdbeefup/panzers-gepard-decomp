// core/core_common.h
// Core common definitions — IDA decompiler compatibility layer
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_CORE_COMMON_H
#define CORE_CORE_COMMON_H

#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>
#include <time.h>
#include <locale.h>

// IDA type aliases
typedef uint8_t  _BYTE;
typedef uint16_t _WORD;
typedef uint32_t _DWORD;
typedef uint64_t _QWORD;

// IDA byte/word extraction macros
// LOBYTE, HIBYTE, LOWORD, HIWORD already defined in <minwindef.h>
#ifndef BYTE1
#define BYTE1(x)    (*(((_BYTE*)&(x)) + 1))
#define BYTE2(x)    (*(((_BYTE*)&(x)) + 2))
#define BYTE3(x)    (*(((_BYTE*)&(x)) + 3))
#endif
#ifndef LODWORD
#define LODWORD(x)  (*((_DWORD*)&(x)))
#define HIDWORD(x)  (*(((_DWORD*)&(x)) + 1))
#endif
#ifndef DWORD1
#define DWORD1(x)   (*(((_DWORD*)&(x)) + 1))
#endif
#ifndef DWORD2
#define DWORD2(x)   (*(((_DWORD*)&(x)) + 2))
#endif

// IDA signed LODWORD (treating float bits as signed int)
#ifndef SLODWORD
#define SLODWORD(x) (*((int*)&(x)))
#endif

// IDA 128-bit type (SSE register / XMM)
#ifndef _OWORD_DEFINED
#define _OWORD_DEFINED
typedef struct { unsigned long long lo, hi; } _OWORD;
#endif

// IDA pair macro — combines two 32-bit values into a 64-bit value
#ifndef __PAIR64__
#define __PAIR64__(hi, lo) (((unsigned long long)(unsigned int)(hi) << 32) | (unsigned int)(lo))
#endif

// IDA memory copy (equivalent to memcpy)
#define qmemcpy(dst, src, size) memcpy(dst, src, size)

// Strip IDA-specific qualifiers (these are no-ops in real C++)
#define __cppobj
#define __noreturn  __declspec(noreturn)

// Global logger and filesystem references (defined in game globals, externed here)
struct SLogger;
struct SFileSystem;

struct LoggerGlobal {
    SLogger* g;       // offset 0: the logger pointer
};

extern LoggerGlobal Logger;
extern SFileSystem FileSystem;

// Headless mode flag — set by swineBot to skip rendering and audio
extern bool g_HeadlessMode;
extern bool g_BotVerbose;

#endif // CORE_CORE_COMMON_H
