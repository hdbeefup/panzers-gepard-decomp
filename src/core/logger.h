// core/logger.h
// SLogger — logging system
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include "core_common.h"
#include "string2.h"

struct SLogger {
    char* LogBufferContent[512];
    int LogBufferUsed;
    bool LogToFile;
    bool Logging;
    char* LogFileName;
    HWND hWnd;
    int LogLevel;

    SLogger(char* filename, char* appname, bool logging, int log_level);
    ~SLogger();
    void Attach(HWND hwnd);
    void Log(int loglev, const char* message, ...);
    void LogCallStack(int filehandle);
    __declspec(noreturn) void Panic(const char* message, ...);
    void SetLogLevel(int log_level);
    void Warning(const char* message, ...);
    void WirteBufferToLog();
};

#endif // CORE_LOGGER_H
