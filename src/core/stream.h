// core/stream.h
// SStream, SStreamBuffer, SArchiveStream, SFileStream, SFileSystem
// Imported from the S.W.I.N.E. HD decompilation; the file system part was
// re-checked against the Codename Panzers HD exe (see docs/FORMATS.md).

#ifndef CORE_STREAM_H
#define CORE_STREAM_H

#include "core_common.h"
#include "darray.h"
#include "string2.h"
#include <stddef.h>

struct SStream;
struct SStreamBuffer;
struct SArchiveStream;
struct SFileStream;
struct SFileSystem;

// The 8 bytes after a TOC node's name suffix, as LookupArchive returns them.
// The member's data starts at 16 + TocSize + Pos in the pak.
struct SArchiveHeaderEntry {
    int Pos;
    int Size;
};

// PANZERS: one [Paths] Search element (HD 0x65df90 / 0x65f1e0; SDArray
// element size 0x14). This replaces SWINE's 16-byte SSearchPath/SArchiveInfo,
// which kept the archive file name and the directory path in separate fields.
enum {
    SEARCHPATH_DIRECTORY = 0,
    SEARCHPATH_ARCHIVE = 1,
};

struct SSearchPathElement {
    int Type;               // 0x00: SEARCHPATH_DIRECTORY or SEARCHPATH_ARCHIVE
    SString Name;           // 0x04: _fullpath() result; directories end in '/'
    int TocSize;            // 0x0C: archive TOC byte count (pak header +0x0C)
    unsigned char* Toc;     // 0x10: archive TOC, read whole
};

// SStream — base stream class with virtual I/O methods
// Virtual function order matches original vtable: AddRef, Release, Read, ReadMax, Write, Seek
// (The Panzers HD vtables are {scalar deleting dtor, Read, ReadMax, Write, Seek};
// the SWINE AddRef/Release interface is kept because the engine calls it.)
struct SStream {
    virtual void AddRef();
    virtual void Release();
    virtual void Read(void* buf, int size);
    virtual int ReadMax(void* buf, int size);
    virtual void Write(const void* buf, int size);
    virtual int Seek(int offset, int origin);

    SStack<int> Chunks;
    bool NewTypeStrings;

    SStream();
    ~SStream();
    unsigned char ReadByte();
    int ReadChunkHeader();
    bool ReadChunkIsEnd();
    int ReadChunkRemain();
    void ReadChunkSkip();
    void ReadChunkValidate(int a2);
    float ReadFloat();
    int ReadInt();
    void ReadSignature();
    char* ReadString();
    unsigned short ReadWord();
    void WriteByte(unsigned char i);
    void WriteChunkEnd();
    void WriteChunkStart(int chunk_id);
    void WriteFloat(float f);
    void WriteInt(int i);
    void WriteSignature();
    void WriteString(const char* str);
    void WriteWord(unsigned short i);
};

struct SStreamBuffer : SStream {
    unsigned int RefCount;
    SDArray<unsigned char*> Blocks;
    int Size;
    int Position;

    SStreamBuffer();
    ~SStreamBuffer();
    void AddRef() override;
    void Release() override;
    void Read(void* buf, int size) override;
    int ReadMax(void* buf, int size) override;
    void Write(const void* buf, int size) override;
    int Seek(int offset, int origin) override;
    int getSize();
    int canRead(int size);
    int getSeek();
    int GenerateCRC();
    void Reset();
};

// PANZERS: archive members are read through a stdio FILE* (HD ctor 0x65cba0,
// object size 0x2c). SWINE used an _sopen_s file descriptor.
struct SArchiveStream : SStream {
    unsigned int RefCount;  // 0x18: SWINE refcount (the HD ctor leaves +0x18 alone)
    FILE* File;             // 0x1C
    int Pos;                // 0x20: absolute file offset of the member's data
    int Size;               // 0x24
    int Offset;             // 0x28: current offset inside the member

    SArchiveStream(FILE* file, int pos, int size);
    ~SArchiveStream();
    void AddRef() override;
    void Release() override;
    void Read(void* buf, int size) override;
    int ReadMax(void* buf, int size) override;
    void Write(const void* buf, int size) override;
    int Seek(int offset, int origin) override;
};

struct SFileStream : SStream {
    unsigned int RefCount;
    int FileDes;
    bool Readable;
    bool Writeable;

    SFileStream(int fd, bool readable, bool writeable);
    ~SFileStream();
    void AddRef() override;
    void Release() override;
    void Read(void* buf, int size) override;
    int ReadMax(void* buf, int size) override;
    void Write(const void* buf, int size) override;
    int Seek(int offset, int origin) override;
};

// PANZERS: SFileSystem (HD global at 0x92e330, 0x14 bytes; the menu.ini
// string table follows it at 0x92e344). The search path array uses the HD
// SDArray field order {array, size, maxsize}. This tree's SDArray<T> is
// {size, maxsize, array}, so the fields are spelled out here.
// SWINE's BasePath and AppDataPath[260] are gone. Panzers resolves loose
// files against the [Paths] Home directory (FileNameProcess 0x65f020).
struct SFileSystem {
    SSearchPathElement* SearchPath; // 0x00
    int SearchPathCount;            // 0x04
    int SearchPathMax;              // 0x08
    SString Home;                   // 0x0C: [Paths] Home, _fullpath'd, ends in '/'

    SFileSystem();
    ~SFileSystem();

    // Panzers boot path
    void SetSearchPath(const char* paths, bool prepend);                 // 0x65df90
    void SetHomePath(const char* path);                                  // 0x65fa30
    const char* GetHomePath();                                           // 0x65ed70
    void OpenArchive(int index);                                         // 0x65f1e0
    SArchiveHeaderEntry* LookupArchive(int index, const char* filename); // 0x65ee70
    void FileNameProcess(SString* result, const char* filename);         // 0x65f020
    SSearchPathElement* GetSearchPathElement(int index);                 // 0x65de60
    int AddSearchPathElement();                                          // 0x65df20
    int InsertSearchPathElement(int index);                              // 0x65ed80

    // SWINE API, kept for the engine callers
    void AddSearchPath(const char* path);
    char* FindFileInSearchPath(const char* filename);
    void FindFiles(const char* path, const char* filter, SDArray<SString>* result);
    char* GetGameDataPath();
    bool GenerateFileCRC(const char* filename, unsigned int* result, const char* panicstr);
    static int GetCRC(const char* buffer, int length);
    static bool IsFullPath(const char* filename);
    const char* MakeAppDataPath(const char* filename);
    char* MakeFullPath(const char* filename);
    void MakePath(SString pathname);
    SStream* OpenAppend(const char* filename, const char* panicstr);
    SStream* OpenRead(const char* filename, const char* panicstr);
    SStream* OpenWrite(const char* filename, const char* panicstr);
    void ReadFile(const char* filename, char** buf, unsigned int* size, const char* caller);
    void RemoveAllSearchPaths();
    void SetGameDataPath(const char* pathname);
    int Stat(const char* filename, struct _stat* buffer);
    void EnumArchive(int index);
};

// x86 layouts, checked against the HD decompile (allocation sizes and the
// field offsets the HD functions use).
#if defined(_M_IX86)
static_assert(sizeof(SString) == 0x8, "SString is {buf,size}");
static_assert(sizeof(SSearchPathElement) == 0x14, "HD 0x65df20 grows by 0x14");
static_assert(offsetof(SSearchPathElement, Name) == 0x04, "HD 0x65f1e0 name at +4");
static_assert(offsetof(SSearchPathElement, TocSize) == 0x0C, "HD 0x65f1e0 tocSize at +0xC");
static_assert(offsetof(SSearchPathElement, Toc) == 0x10, "HD 0x65f1e0 toc at +0x10");
static_assert(sizeof(SFileSystem) == 0x14, "HD FileSystem 0x92e330..0x92e344");
static_assert(offsetof(SFileSystem, SearchPathCount) == 0x04, "HD 0x65f420 param_1[1]");
static_assert(offsetof(SFileSystem, Home) == 0x0C, "HD 0x65f020 ECX+0xC");
static_assert(sizeof(SArchiveStream) == 0x2C, "HD operator new(0x2c) in 0x65f420");
static_assert(offsetof(SArchiveStream, File) == 0x1C, "HD 0x65cba0 param_1[7]");
static_assert(offsetof(SArchiveStream, Pos) == 0x20, "HD 0x65cba0 param_1[8]");
static_assert(offsetof(SArchiveStream, Size) == 0x24, "HD 0x65cba0 param_1[9]");
static_assert(offsetof(SArchiveStream, Offset) == 0x28, "HD 0x65cba0 param_1[10]");
static_assert(sizeof(SFileStream) == 0x24, "HD operator new(0x24) in 0x65f420");
static_assert(offsetof(SFileStream, FileDes) == 0x1C, "HD 0x65cc90 param_1[7]");
static_assert(offsetof(SFileStream, Readable) == 0x20, "HD 0x65cc90 +0x20");
static_assert(offsetof(SFileStream, Writeable) == 0x21, "HD 0x65cc90 +0x21");
#endif

#endif // CORE_STREAM_H
