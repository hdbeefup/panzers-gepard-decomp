// core/stream.h
// SStream, SStreamBuffer, SArchiveStream, SFileStream, SFileSystem
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_STREAM_H
#define CORE_STREAM_H

#include "core_common.h"
#include "darray.h"
#include "string2.h"

struct SStream;
struct SStreamBuffer;
struct SArchiveStream;
struct SFileStream;
struct SFileSystem;

struct SArchiveHeaderEntry {
    int Pos;
    int Size;
};

struct SArchiveInfo {
    char* FileName;
    int HeaderSize;
    unsigned char* Header;

    SArchiveHeaderEntry* Lookup(const char* filename);
    void FindFiles(const char* path, const char* filter, SDArray<SString>* result);
    void FindFilesIterate(const SString* path, SString* postfix, SDArray<SString>* result, unsigned char* lookup_ptr, SString name);
    void Iterate(unsigned char* lookup_ptr, SString name);
};

struct SSearchPath {
    SArchiveInfo Archive;
    char* Path;
};

// SStream — base stream class with virtual I/O methods
// Virtual function order matches original vtable: AddRef, Release, Read, ReadMax, Write, Seek
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

struct SArchiveStream : SStream {
    unsigned int RefCount;
    int FileDes;
    int Pos;
    int Size;
    int Offset;

    SArchiveStream(int fd, int pos, int size);
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

struct SFileSystem {
    char* BasePath;
    char AppDataPath[260];
    SDArray<SSearchPath> SearchPaths;

    SFileSystem();
    ~SFileSystem();
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
    SSearchPath* OpenArchive(const char* filename);
    SStream* OpenRead(const char* filename, const char* panicstr);
    SStream* OpenWrite(const char* filename, const char* panicstr);
    void ReadFile(const char* filename, char** buf, unsigned int* size, const char* caller);
    void RemoveAllSearchPaths();
    void SetGameDataPath(const char* pathname);
    int Stat(const char* filename, struct _stat* buffer);
};

#endif // CORE_STREAM_H
