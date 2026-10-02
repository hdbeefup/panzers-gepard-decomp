// core/logger.cpp
// Logging system
// Decompiled from: gameSplit/globals.c

#include "logger.h"
#include "stream.h"
#include <stdarg.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

LoggerGlobal Logger;
bool g_HeadlessMode = false;
bool g_BotVerbose = false;

static const char fullpath[] = "------------------------------------------------------------\r\n";

//----- (004FB410) --------------------------------------------------------

SLogger::SLogger(char* filename, char* appname, bool logging, int log_level)
{
    this->LogBufferUsed = 0;
    for (int i = 0; i < 512; ++i)
        this->LogBufferContent[i] = (char*)operator new[](0x20Au);
    this->Logging = logging;
    this->LogLevel = log_level;
    this->hWnd = nullptr;
    this->LogToFile = false;
    this->LogFileName = nullptr;
    if (logging) {
        const char* adp = FileSystem.MakeAppDataPath("log");
        SString logDir;
        if (adp) {
            logDir.size = (int)strlen(adp);
            logDir.buf = (char*)operator new[](logDir.size + 1);
            memcpy(logDir.buf, adp, logDir.size + 1);
        } else { logDir.buf = nullptr; logDir.size = 0; }
        FileSystem.MakePath(logDir);
        int timestamp = (int)_time64(nullptr);
        char buf[260];
        sprintf(buf, "log\\%s %d.txt", filename, timestamp);
        const char* logPath = FileSystem.MakeAppDataPath(buf);
        this->LogFileName = _strdup(logPath);
        int fd;
        errno_t err = _sopen_s(&fd, this->LogFileName, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _SH_DENYNO, _S_IWRITE);
        if (err || fd < 0) {
            this->LogToFile = false;
            Warning("SLogger::SLogger: Open failed: %s", this->LogFileName);
        } else {
            this->LogToFile = true;
            _close(fd);
            Log(-1, "Log system initialized (log level: %d)", this->LogLevel);
            Log(-1, "Application name: %s", appname);
            Log(-1, fullpath);
        }
    }
}

//----- (004FB5E0) --------------------------------------------------------

SLogger::~SLogger()
{
    Log(-1, fullpath);
    Log(-1, "Log system destroyed");
    for (int i = 0; i < 512; ++i)
        operator delete[](this->LogBufferContent[i]);
    if (this->LogFileName) {
        operator delete(this->LogFileName);
        this->LogFileName = nullptr;
    }
}

//----- (004FB640) --------------------------------------------------------

void SLogger::Attach(HWND hwnd) { this->hWnd = hwnd; }

//----- (004FB650) --------------------------------------------------------

void SLogger::Log(int loglev, const char* message, ...)
{
    va_list ArgList;
    va_start(ArgList, message);
    if (this->Logging && loglev <= this->LogLevel) {
        char buf[512];
        char LogTextBuffer[524];
        vsnprintf(buf, 0x200u, message, ArgList);
        if (loglev < 0) _snprintf(LogTextBuffer, 0x20Au, "%s\r\n", buf);
        else _snprintf(LogTextBuffer, 0x20Au, "<%d> %s\r\n", loglev, buf);
        int used = this->LogBufferUsed;
        bool flush = (used == 512);
        if (used < 512) { strcpy(this->LogBufferContent[used], LogTextBuffer); flush = (++this->LogBufferUsed == 512); }
        if (flush || loglev <= 0) WirteBufferToLog();
        OutputDebugStringA(LogTextBuffer);
    }
    va_end(ArgList);
}

//----- (004FB750) --------------------------------------------------------

void SLogger::LogCallStack(int filehandle)
{
    CONTEXT Context;
    memset(&Context, 0, sizeof(Context));
    Context.ContextFlags = CONTEXT_CONTROL;
    RtlCaptureContext(&Context);
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymInitialize(process, nullptr, TRUE);
    STACKFRAME64 stack;
    memset(&stack, 0, sizeof(stack));
#if defined(_M_X64)
    stack.AddrPC.Offset    = Context.Rip;
    stack.AddrStack.Offset = Context.Rsp;
    stack.AddrFrame.Offset = Context.Rbp;
    const DWORD stackWalkMachine = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
    stack.AddrPC.Offset    = Context.Eip;
    stack.AddrStack.Offset = Context.Esp;
    stack.AddrFrame.Offset = Context.Ebp;
    const DWORD stackWalkMachine = IMAGE_FILE_MACHINE_I386;
#else
#   error "SLogger::LogCallStack: unsupported architecture (port CONTEXT register names + StackWalk64 machine type)"
#endif
    stack.AddrPC.Mode    = AddrModeFlat;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Mode = AddrModeFlat;
    char symbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + 256];
    IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)symbolBuffer;
    IMAGEHLP_LINE64 line;
    DWORD64 displacement64;
    DWORD displacement;
    char buf[260];
    while (true) {
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symbol->MaxNameLength = 255;
        if (!SymGetSymFromAddr64(process, stack.AddrPC.Offset, &displacement64, symbol)) break;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &displacement, &line))
            _snprintf(buf, sizeof(buf), "  %s (%s:%d)\n", symbol->Name, line.FileName, line.LineNumber);
        else
            _snprintf(buf, sizeof(buf), "  %s\n", symbol->Name);
        _write(filehandle, buf, (unsigned int)strlen(buf));
        if (!StackWalk64(stackWalkMachine, process, thread, &stack, &Context,
                         nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
    }
}

//----- (004FB9F0) --------------------------------------------------------

__declspec(noreturn) void SLogger::Panic(const char* message, ...)
{
    va_list ArgList;
    va_start(ArgList, message);
    if (this->Logging) {
        char buf[256]; char buf2[280];
        vsnprintf(buf, sizeof(buf), message, ArgList);
        _snprintf(buf2, sizeof(buf2), "<Panic> %s\r\n", buf);
        if (this->LogToFile) {
            WirteBufferToLog();
            int fd;
            errno_t err = _sopen_s(&fd, this->LogFileName, _O_WRONLY | _O_APPEND | _O_BINARY, _SH_DENYNO, _S_IWRITE);
            if (!err && fd >= 0) { _write(fd, buf2, (unsigned int)strlen(buf2)); LogCallStack(fd); _close(fd); }
        }
        OutputDebugStringA(buf2);
        MessageBoxA(this->hWnd, buf, "Fatal error", MB_ICONERROR);
        PostQuitMessage(0);
    }
    va_end(ArgList);
    _exit(1);
}

//----- (004FBB10) --------------------------------------------------------

void SLogger::SetLogLevel(int log_level)
{
    if (this->LogLevel != log_level) {
        this->LogLevel = log_level;
        Log(-1, "Log level changed (%d)", log_level);
        Log(-1, fullpath);
    }
}

//----- (004FBB50) --------------------------------------------------------

void SLogger::Warning(const char* message, ...)
{
    va_list ArgList;
    va_start(ArgList, message);
    if (this->Logging) {
        char buf[256]; char buf2[280];
        vsnprintf(buf, sizeof(buf), message, ArgList);
        _snprintf(buf2, sizeof(buf2), "<Warning> %s\r\n", buf);
        if (this->LogToFile) {
            WirteBufferToLog();
            int fd;
            errno_t err = _sopen_s(&fd, this->LogFileName, _O_WRONLY | _O_APPEND | _O_BINARY, _SH_DENYNO, _S_IWRITE);
            if (!err && fd >= 0) { _write(fd, buf2, (unsigned int)strlen(buf2)); _close(fd); }
        }
        OutputDebugStringA(buf2);
        if (MessageBoxA(this->hWnd, buf, "Warning", MB_OKCANCEL) == IDCANCEL)
            PostQuitMessage(0);
    }
    va_end(ArgList);
}

//----- (004FBC70) --------------------------------------------------------

void SLogger::WirteBufferToLog()
{
    if (this->LogToFile) {
        int fd;
        errno_t err = _sopen_s(&fd, this->LogFileName, _O_WRONLY | _O_APPEND | _O_BINARY, _SH_DENYNO, _S_IWRITE);
        if (!err && fd >= 0) {
            for (int i = 0; i < this->LogBufferUsed; ++i) {
                char* text = this->LogBufferContent[i];
                _write(fd, text, (unsigned int)strlen(text));
            }
            this->LogBufferUsed = 0;
            _close(fd);
        }
    }
}
