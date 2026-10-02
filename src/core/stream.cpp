// core/stream.cpp
// File I/O, streams, filesystem
// Decompiled from: gameSplit/sfilesystem.c

#include "stream.h"
#include "logger.h"
#include "hdbeefup.h"
#include <shlobj.h>
#include <direct.h>
#include <errno.h>

#pragma comment(lib, "shell32.lib")

// ============================================================
// Static data
// ============================================================

// Per-function path buffers (avoid aliasing between nested calls)
static char fullname[260];     // OpenRead, AddSearchPath
static char fullname_0[260];   // FindFileInSearchPath
static char fullname_1[260];   // FindFiles
static char fullname_2[260];   // MakeFullPath
static char fullname_3[260];   // MakeAppDataPath
static char fullname_4[260];   // Lookup
static char fullname_5[260];   // FindFiles (archive)
static char fullname_6[260];   // Stat
static char err[260];          // SFileStream errors
static char err_0[260];        // SFileStream::ReadMax errors
static char err_1[260];        // SArchiveStream::Read errors
static char err_2[260];        // SArchiveStream::ReadMax errors
static char fileName[264];    // FindFilesIterate extracted filename
static char buf_0[0x4000];    // Iterate copy buffer (16KB)

// CRC32 lookup table (polynomial 0xEDB88320)
static unsigned int crctable[256];
static struct CRCTableInit {
    CRCTableInit() {
        for (unsigned int i = 0; i < 256; i++) {
            unsigned int crc = i;
            for (int j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (0xEDB88320 & -(int)(crc & 1));
            crctable[i] = crc;
        }
    }
} _crctable_init;

// Global FileSystem instance (adjacent to Logger in original binary)
SFileSystem FileSystem;

// ============================================================
// Free functions
// ============================================================

//----- (0043A670) --------------------------------------------------------

int __cdecl ReadSign(SStream* is)
{
    if (is->ReadInt() != 123456)
        Logger.g->Panic("SGepard::ReadSign: Attempt to read sign, but no sign here!");
    return is->ReadInt();
}

//----- (00446440) --------------------------------------------------------

void __cdecl WriteSign(SStream* is, int sign)
{
    is->WriteInt(123456);
    is->WriteInt(sign);
}

// ============================================================
// SStream
// ============================================================

//----- (004FBF20) --------------------------------------------------------

SStream::SStream()
{
    Chunks.array = 0;
    Chunks.size = 0;
    Chunks.maxsize = 0;
    Chunks.userdata = 0.0f;
    NewTypeStrings = false;
}

//----- (004FC1A0) --------------------------------------------------------

SStream::~SStream()
{
    // Idempotent: IDA-style ports call this explicitly AND the compiler
    // emits another call at scope exit, so clear the pointer to neutralise
    // the second dispatch (Chunks.array would otherwise be a dangling free).
    if (Chunks.array)
    {
        free(Chunks.array);
        Chunks.array = 0;
    }
    Chunks.size = 0;
    Chunks.maxsize = 0;
}

// Base class virtual methods — SStream is never instantiated directly,
// these provide default no-op/error behavior for the vtable slots.
// All real I/O goes through SFileStream, SArchiveStream, or SStreamBuffer.

void SStream::AddRef() { }
void SStream::Release() { operator delete(this); }
void SStream::Read(void* buf, int size) { throw "Read failed"; }
int SStream::ReadMax(void* buf, int size) { return 0; }
void SStream::Write(const void* buf, int size) { throw "Write failed"; }
int SStream::Seek(int offset, int origin) { return 0; }

//----- (004FE370) --------------------------------------------------------

unsigned char SStream::ReadByte()
{
    unsigned char result;
    Read(&result, 1);
    return result;
}

//----- (004FE630) --------------------------------------------------------

int SStream::ReadInt()
{
    int result;
    Read(&result, 4);
    return result;
}

//----- (004FE600) --------------------------------------------------------

float SStream::ReadFloat()
{
    float result;
    Read(&result, 4);
    return result;
}

//----- (004FE8F0) --------------------------------------------------------

unsigned short SStream::ReadWord()
{
    unsigned short result;
    Read(&result, 2);
    return result;
}

//----- (004FE870) --------------------------------------------------------

char* SStream::ReadString()
{
    int len = 0;
    Read(&len, 2);
    char* buf = (char*)operator new[](len + 1);
    if (!buf)
        throw "Out of memory";
    Read(buf, len);
    buf[len] = 0;
    return buf;
}

// PANZERS 0x65d6a0
// Panzers accepts only the 0x1B1A7253 magic. SWINE also took the older
// 0x1A1A7253 (old-style strings, NewTypeStrings=false); Panzers throws
// "Not a Stormregion file" for it. The HD function does not touch the
// string-format flag (the separate setter 0x65d8f0 does). It is set to true
// here, which is what SWINE did for this magic, so SString::Load callers
// see the same format as before.
void SStream::ReadSignature()
{
    int sig = 0;
    Read(&sig, 4);
    if (sig == 0x1B1A7253) {
        Read(&sig, 4);
        if (sig == 0x0A870A0D) {
            NewTypeStrings = true;
            return;
        }
    }
    throw "Not a Stormregion file";
}

//----- (004FE3A0) --------------------------------------------------------

int SStream::ReadChunkHeader()
{
    int chunkId, chunkSize;
    Read(&chunkId, 4);
    Read(&chunkSize, 4);
    int pos = Seek(0, 1);
    Chunks.Push(chunkSize + pos);
    return chunkId;
}

//----- (004FE400) --------------------------------------------------------

bool SStream::ReadChunkIsEnd()
{
    if (!Chunks.size)
        Logger.g->Panic("SStack::Peek: stack is empty");
    return Chunks.array[Chunks.size - 1] == Seek(0, 1);
}

//----- (004FE430) --------------------------------------------------------

int SStream::ReadChunkRemain()
{
    if (!Chunks.size)
        Logger.g->Panic("SStack::Peek: stack is empty");
    return Chunks.array[Chunks.size - 1] - Seek(0, 1);
}

//----- (004FE470) --------------------------------------------------------

void SStream::ReadChunkSkip()
{
    if (!Chunks.size)
        Logger.g->Panic("SStack::Peek: stack is empty");
    Seek(Chunks.array[Chunks.size - 1], 0);
}

//----- (004FE4A0) --------------------------------------------------------

void SStream::ReadChunkValidate(int a2)
{
    if (!Chunks.size)
        Logger.g->Panic("SStack::Pop: stack is empty");
    int expected = Chunks.array[--Chunks.size];
    int actual = Seek(0, 1);
    if (expected != actual)
        throw "Read chunk size mismatch";
}

//----- (004FF050) --------------------------------------------------------

void SStream::WriteByte(unsigned char i)
{
    Write(&i, 1);
}

//----- (004FF170) --------------------------------------------------------

void SStream::WriteInt(int i)
{
    Write(&i, 4);
}

//----- (004FF150) --------------------------------------------------------

void SStream::WriteFloat(float f)
{
    Write(&f, 4);
}

//----- (004FF260) --------------------------------------------------------

void SStream::WriteWord(unsigned short i)
{
    Write(&i, 2);
}

//----- (004FF1E0) --------------------------------------------------------

void SStream::WriteString(const char* str)
{
    int len = (int)strlen(str);
    if (len > 0xFFFF)
        throw "String is too long";
    Write(&len, 2);
    Write(str, len);
}

//----- (004FF190) --------------------------------------------------------

void SStream::WriteSignature()
{
    int sig = 454718035;
    Write(&sig, 4);
    sig = 176622093;
    Write(&sig, 4);
}

//----- (004FF0F0) --------------------------------------------------------

void SStream::WriteChunkStart(int chunk_id)
{
    int tmp = chunk_id;
    Write(&tmp, 4);
    tmp = 0;
    Write(&tmp, 4);
    int pos = Seek(0, 1);
    Chunks.Push(pos);
}

//----- (004FF070) --------------------------------------------------------

void SStream::WriteChunkEnd()
{
    if (!Chunks.size)
        Logger.g->Panic("SStack::Pop: stack is empty");
    int startPos = Chunks.array[--Chunks.size];
    int endPos = Seek(0, 1);
    Seek(startPos - 4, 0);
    int chunkSize = endPos - startPos;
    Write(&chunkSize, 4);
    Seek(endPos, 0);
}

// ============================================================
// SStreamBuffer
// ============================================================

//----- (004FBF60) --------------------------------------------------------

SStreamBuffer::SStreamBuffer()
{
    RefCount = 1;
    Blocks.size = 0;
    Blocks.maxsize = 0;
    Blocks.array = 0;
    Size = 0;
    Position = 0;
    NewTypeStrings = true;
}

//----- (004FC1B0) --------------------------------------------------------

SStreamBuffer::~SStreamBuffer()
{
    // Idempotent: see SStream::~SStream above. Clearing Blocks.size
    // ensures a double-destruction (explicit + scope-exit) is safe even
    // if surrounding code has since scribbled over Blocks.array.
    for (int i = 0; i < Blocks.size; i++)
        operator delete(Blocks.array[i]);
    if (Blocks.array) {
        free(Blocks.array);
        Blocks.array = 0;
    }
    Blocks.size = 0;
    Blocks.maxsize = 0;
}

//----- (004FC350) --------------------------------------------------------

void SStreamBuffer::AddRef()
{
    ++RefCount;
}

//----- (004FE9A0) --------------------------------------------------------

void SStreamBuffer::Release()
{
    if (RefCount-- == 1) {
        for (int i = 0; i < Blocks.size; i++)
            operator delete(Blocks.array[i]);
        if (Blocks.array) {
            free(Blocks.array);
            Blocks.array = 0;
        }
        if (Chunks.array)
            free(Chunks.array);
        operator delete(this);
    }
}

//----- (004FE2D0) --------------------------------------------------------

void SStreamBuffer::Read(void* buf, int size)
{
    if (size > Size - Position)
        throw "Read failed";
    char* dst = (char*)buf;
    int remaining = size;
    int pos = Position;
    while (remaining > 0) {
        int blockOffset = pos % 4096;
        int copySize = remaining;
        if (copySize > 4096 - blockOffset)
            copySize = 4096 - blockOffset;
        memcpy(dst, &Blocks.array[pos / 4096][blockOffset], copySize);
        Position += copySize;
        dst += copySize;
        pos = Position;
        remaining -= copySize;
    }
}

//----- (004FE750) --------------------------------------------------------

int SStreamBuffer::ReadMax(void* buf, int size)
{
    int available = Size - Position;
    int toRead = size;
    if (toRead > available)
        toRead = (available >= 0) ? available : 0;
    int result = toRead;
    char* dst = (char*)buf;
    int pos = Position;
    while (size > 0) {
        int blockOffset = pos % 4096;
        int copySize = size;
        if (copySize > 4096 - blockOffset)
            copySize = 4096 - blockOffset;
        memcpy(dst, &Blocks.array[Position / 4096][blockOffset], copySize);
        Position += copySize;
        dst += copySize;
        pos = Position;
        size -= copySize;
    }
    return result;
}

//----- (004FEF00) --------------------------------------------------------

void SStreamBuffer::Write(const void* buf, int size)
{
    const char* src = (const char*)buf;
    int remaining = size;
    if (remaining) {
        int pos = Position;
        do {
            if (pos / 4096 >= Blocks.size) {
                unsigned char* block = (unsigned char*)operator new[](0x1000u);
                if (!block)
                    throw "Out of memory";
                int idx = Blocks.Add();
                Blocks.array[idx] = block;
                pos = Position;
                remaining = size;
            }
            int blockOffset = pos % 4096;
            int copySize = remaining;
            if (copySize > 4096 - blockOffset)
                copySize = 4096 - blockOffset;
            memcpy(&Blocks.array[pos / 4096][blockOffset], src, copySize);
            Position += copySize;
            remaining -= copySize;
            src += copySize;
            pos = Position;
            size = remaining;
        } while (remaining);
    }
    if (Size < Position)
        Size = Position;
}

//----- (004FEC10) --------------------------------------------------------

int SStreamBuffer::Seek(int offset, int origin)
{
    int result;
    if (origin == 0)
        result = offset;
    else if (origin == 1)
        result = Position + offset;
    else if (origin == 2)
        result = Size;
    else
        result = 0;
    if (result < 0 || result > Size)
        throw "Seek failed";
    Position = result;
    return result;
}

//----- (004FF2B0) --------------------------------------------------------

int SStreamBuffer::getSize()
{
    return Size;
}

//----- (004FF2A0) --------------------------------------------------------

int SStreamBuffer::getSeek()
{
    return Position;
}

//----- (004FF280) --------------------------------------------------------

int SStreamBuffer::canRead(int size)
{
    return size <= Size - Position;
}

//----- (004FD0A0) --------------------------------------------------------

int SStreamBuffer::GenerateCRC()
{
    unsigned int crc = 0xFFFFFFFF;
    for (int i = 0; i < Size; i++) {
        unsigned char byte = Blocks.array[i / 4096][i % 4096];
        crc = crctable[(unsigned char)(crc ^ byte)] ^ (crc >> 8);
    }
    return ~crc;
}

//----- (004FEB10) --------------------------------------------------------

void SStreamBuffer::Reset()
{
    Position = 0;
}

// ============================================================
// SFileStream
// ============================================================

//----- (004FBE00) --------------------------------------------------------

SFileStream::SFileStream(int fd, bool readable, bool writeable)
{
    RefCount = 1;
    FileDes = fd;
    Readable = readable;
    Writeable = writeable;
}

//----- (004FC010) --------------------------------------------------------

SFileStream::~SFileStream()
{
    _close(FileDes);
    FileDes = -1;
}

//----- (004FC340) --------------------------------------------------------

void SFileStream::AddRef()
{
    ++RefCount;
}

//----- (004FE960) --------------------------------------------------------

void SFileStream::Release()
{
    if (RefCount-- == 1) {
        _close(FileDes);
        FileDes = -1;
        if (Chunks.array)
            free(Chunks.array);
        operator delete(this);
    }
}

//----- (004FE240) --------------------------------------------------------

void SFileStream::Read(void* buf, int size)
{
    if (!Readable)
        throw "Read failed: File is not open for reading.";
    int bytesRead = _read(FileDes, buf, size);
    if (bytesRead != size) {
        if (bytesRead < 0) {
            char* errstr = strerror(errno);
            sprintf(err, "Read failed(%d): %s.", FileDes, errstr);
        } else {
            sprintf(err, "Read failed(%d): Expected %d bytes, read %d.", FileDes, size, bytesRead);
        }
        throw (const char*)err;
    }
}

//----- (004FE6D0) --------------------------------------------------------

int SFileStream::ReadMax(void* buf, int size)
{
    if (!Readable)
        throw "Read failed: File is not open for reading.";
    int result = _read(FileDes, buf, size);
    if (result < 0) {
        char* errstr = strerror(errno);
        sprintf(err_0, "Read failed(%d): %s.", FileDes, errstr);
        throw (const char*)err_0;
    }
    return result;
}

//----- (004FEEC0) --------------------------------------------------------

void SFileStream::Write(const void* buf, int size)
{
    if (!Writeable || _write(FileDes, buf, size) != size)
        throw "Write failed";
}

//----- (004FEBD0) --------------------------------------------------------

int SFileStream::Seek(int offset, int origin)
{
    if (!Readable && !Writeable)
        throw "Seek failed";
    int result = _lseek(FileDes, offset, origin);
    if (result < 0)
        throw "Seek failed";
    return result;
}

// ============================================================
// SArchiveStream
// ============================================================

// PANZERS 0x65cba0
// Panzers reads archive members through stdio (fopen/fseek/ftell/fread)
// instead of SWINE's _sopen_s file descriptor.
SArchiveStream::SArchiveStream(FILE* file, int pos, int size)
{
    RefCount = 1;
    File = file;
    Pos = pos;
    Size = size;
    Offset = 0;
    int res = fseek(file, pos, SEEK_SET);
    if (res == 0) {
        Offset = ftell(File) - Pos;
        if (Offset >= 0 && Offset <= Size)
            return;
    }
    Logger.g->Log(0, "Seek error (origin=%d, offset=%d, res=%d,  Pos=%d Size=%d Offset=%d)",
        0, 0, res, Pos, Size, Offset);
    throw "Seek failed";
}

// PANZERS 0x65cd80
SArchiveStream::~SArchiveStream()
{
    if (File) {
        fclose(File);
        File = nullptr;
    }
}

//----- (004FC330) --------------------------------------------------------

void SArchiveStream::AddRef()
{
    ++RefCount;
}

//----- (004FE920) --------------------------------------------------------

void SArchiveStream::Release()
{
    // The HD vtable has a scalar deleting destructor (0x65cef0) here: fclose,
    // free the chunk stack, delete. The SWINE refcount is kept.
    if (RefCount-- == 1)
        delete this;
}

// PANZERS 0x65d150
void SArchiveStream::Read(void* buf, int size)
{
    int bytesRead = ReadMax(buf, size);
    if (bytesRead != size) {
        sprintf(err_1, "Read failed: Expected %d bytes, read %d.", size, bytesRead);
        throw (const char*)err_1;
    }
}

// PANZERS 0x65d4f0
int SArchiveStream::ReadMax(void* buf, int size)
{
    int available = Size - Offset;
    if (size > available)
        size = available;
    if (size <= 0)
        return 0;
    int result = (int)fread(buf, 1, (size_t)size, File);
    if (result < 0) {
        sprintf(err_2, "Read failed: %s.", strerror(errno));
        throw (const char*)err_2;
    }
    Offset += result;
    return result;
}

// PANZERS 0x65d900
void SArchiveStream::Write(const void* buf, int size)
{
    throw "Write failed";
}

// PANZERS 0x65d7a0
int SArchiveStream::Seek(int offset, int origin)
{
    int target;
    int whence;
    if (origin == 0) {
        target = Pos + offset;
        whence = SEEK_SET;
    } else if (origin == 1) {
        target = offset;
        whence = SEEK_CUR;
    } else if (origin == 2) {
        target = Pos + Size + offset;
        whence = SEEK_SET;
    } else {
        throw "Seek called with bad origin";
    }
    int res = fseek(File, target, whence);
    if (res == 0) {
        Offset = ftell(File) - Pos;
        if (Offset >= 0 && Offset <= Size)
            return Offset;
    }
    Logger.g->Log(0, "Seek error (origin=%d, offset=%d, res=%d,  Pos=%d Size=%d Offset=%d)",
        origin, offset, res, Pos, Size, Offset);
    throw "Seek failed";
}

// ============================================================
// Archive TOC helpers
// ============================================================
//
// The TOC is a binary search tree serialised in pre-order. Node:
//   u8 prefix   chars taken from the parent's full name
//   u8 len      suffix length
//   len bytes   suffix (no NUL)
//   i32 pos, i32 size        (SArchiveHeaderEntry)
//   u8 hasLeft  left child follows at node + 15 + len
//   i32 right   TOC offset of the right child, 0 = none
// See docs/FORMATS.md.

// Build "parent[0:prefix] + suffix" for a node into an SString.
static void ArchiveNodeName(const unsigned char* node, const SString& parent, SString* out)
{
    unsigned char prefixStart = node[0];
    unsigned char prefixLen = node[1];
    int take = 0;
    if (parent.size > 0 && prefixStart)
        take = (prefixStart >= (unsigned int)parent.size) ? parent.size : prefixStart;
    char* combined = new char[take + prefixLen + 1];
    if (take)
        memcpy(combined, parent.buf, take);
    memcpy(combined + take, node + 2, prefixLen);
    combined[take + prefixLen] = 0;
    *out = combined;
    delete[] combined;
}

static void CopyString(SString* dst, const SString& src)
{
    dst->buf = nullptr;
    dst->size = 0;
    if (src.size) {
        dst->size = src.size;
        dst->buf = new char[src.size + 1];
        memcpy(dst->buf, src.buf, src.size + 1);
    }
}

//----- (004FC7D0) --------------------------------------------------------

static void ArchiveFindFilesIterate(const unsigned char* toc, const SString* path, SString* postfix,
                                    SDArray<SString>* result, const unsigned char* lookup_ptr, SString name)
{
    SString full;
    ArchiveNodeName(lookup_ptr, name, &full);
    if (name.buf)
        delete[] name.buf;
    name.buf = full.buf;    // take ownership
    name.size = full.size;

    const unsigned char* after = lookup_ptr + 2 + lookup_ptr[1];

    if (after[8]) {
        SString leftName;
        CopyString(&leftName, name);
        ArchiveFindFilesIterate(toc, path, postfix, result, after + 13, leftName);
    }
    int rightOffset = *(const int*)(after + 9);
    if (rightOffset) {
        SString rightName;
        CopyString(&rightName, name);
        ArchiveFindFilesIterate(toc, path, postfix, result, toc + rightOffset, rightName);
    }

    const char* pathBuf = path->buf ? path->buf : "";
    const char* nameBuf = name.buf ? name.buf : "";
    if (strncmp(nameBuf, pathBuf, path->size) != 0 || name.size == path->size)
        goto cleanup;

    {
        const char* src = nameBuf + path->size;
        strncpy(fileName, src, sizeof(fileName) - 1);
        fileName[sizeof(fileName) - 1] = 0;
    }

    if (postfix->size == 0) {
        // Directory listing mode: name must end with '/'
        if (nameBuf[name.size - 1] != '/')
            goto cleanup;
        unsigned int fnLen = name.size - path->size - 1;
        if (fnLen < 0x104)
            fileName[fnLen] = 0;
    }

    if (strchr(fileName, '/'))
        goto cleanup;

    if (postfix->size) {
        const char* postBuf = postfix->buf ? postfix->buf : "";
        if (name.size < postfix->size || strcmp(&nameBuf[name.size - postfix->size], postBuf) != 0)
            goto cleanup;
    }

    for (int i = 0; i < result->size; i++) {
        const char* existing = result->array[i].buf ? result->array[i].buf : "";
        if (_stricmp(existing, fileName) == 0)
            goto cleanup;
    }
    {
        SString item;
        item.size = (int)strlen(fileName);
        item.buf = new char[item.size + 1];
        memcpy(item.buf, fileName, item.size + 1);
        result->Add(&item);
    }

cleanup:
    if (name.buf)
        delete[] name.buf;
}

//----- (004FC570) --------------------------------------------------------

static void ArchiveFindFiles(const unsigned char* toc, const char* path, const char* filter, SDArray<SString>* result)
{
    strncpy(fullname_5, path, sizeof(fullname_5) - 1);
    fullname_5[sizeof(fullname_5) - 1] = 0;
    _strlwr(fullname_5);
    for (char* p = fullname_5; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    SString postfix;
    if (filter == (const char*)-1 || !filter || !filter[0]) {
        postfix.size = 0;
        postfix.buf = nullptr;
    } else {
        postfix.size = (int)strlen(filter + 1);
        postfix.buf = new char[postfix.size + 1];
        memcpy(postfix.buf, filter + 1, postfix.size + 1);
    }

    SString pathStr;
    pathStr.size = (int)strlen(fullname_5);
    pathStr.buf = new char[pathStr.size + 1];
    memcpy(pathStr.buf, fullname_5, pathStr.size + 1);

    SString emptyName;
    ArchiveFindFilesIterate(toc, &pathStr, &postfix, result, toc, emptyName);

    delete[] pathStr.buf;
    if (postfix.buf)
        delete[] postfix.buf;
}

//----- (004FD290) --------------------------------------------------------

// SWINE "EnumArchive": extracts every member of an archive below Home.
static void ArchiveIterate(SFileSystem* fs, const SSearchPathElement* e, const unsigned char* node, SString name)
{
    SString full;
    ArchiveNodeName(node, name, &full);
    if (name.buf)
        delete[] name.buf;
    name.buf = full.buf;    // take ownership
    name.size = full.size;

    const unsigned char* after = node + 2 + node[1];
    if (after[8]) {
        SString leftName;
        CopyString(&leftName, name);
        ArchiveIterate(fs, e, after + 13, leftName);
    }
    int rightOffset = *(const int*)(after + 9);
    if (rightOffset) {
        SString rightName;
        CopyString(&rightName, name);
        ArchiveIterate(fs, e, e->Toc + rightOffset, rightName);
    }

    const char* logName = name.buf ? name.buf : "";
    int pos = *(const int*)after;
    int size = *(const int*)(after + 4);
    Logger.g->Log(0, "EnumArchive: %s %d %d", logName, pos, size);

    if (size > 0) {
        FILE* f = fopen(e->Name.buf ? e->Name.buf : "", "rb");
        if (f) {
            SArchiveStream as(f, 16 + e->TocSize + pos, size);
            SStream* outStream = fs->OpenWrite(logName, "SFileSystem::EnumArchive");
            int remaining = size;
            while (remaining) {
                int chunk = remaining > 0x4000 ? 0x4000 : remaining;
                as.Read(buf_0, chunk);
                outStream->Write(buf_0, chunk);
                remaining -= chunk;
            }
            outStream->Release();
        }
    }
    if (name.buf)
        delete[] name.buf;
}

// ============================================================
// SFileSystem
// ============================================================

// SString + const char* (HD 0x52c580), into a fresh SString.
static void ConcatString(SString* out, const SString& a, const char* b)
{
    out->buf = nullptr;
    out->size = 0;
    int blen = b ? (int)strlen(b) : 0;
    int len = a.size + blen;
    if (!a.size && !b)
        return;
    out->buf = new char[len + 1];
    if (a.size)
        memcpy(out->buf, a.buf, a.size);
    if (blen)
        memcpy(out->buf + a.size, b, blen);
    out->buf[len] = 0;
    out->size = len;
}

static void FreeString(SString* s)
{
    if (s->buf) {
        delete[] s->buf;
        s->buf = nullptr;
    }
    s->size = 0;
}

static void FreeSearchPathElement(SSearchPathElement* e)
{
    FreeString(&e->Name);
    if (e->Toc) {
        delete[] e->Toc;
        e->Toc = nullptr;
    }
    e->TocSize = 0;
    e->Type = 0;
}

//----- (004FBE50) --------------------------------------------------------

SFileSystem::SFileSystem()
{
    SearchPath = nullptr;
    SearchPathCount = 0;
    SearchPathMax = 0;
    Home.buf = nullptr;
    Home.size = 0;
}

//----- (004FC040) --------------------------------------------------------

SFileSystem::~SFileSystem()
{
    for (int i = 0; i < SearchPathCount; i++)
        FreeSearchPathElement(&SearchPath[i]);
    if (SearchPath) {
        free(SearchPath);
        SearchPath = nullptr;
    }
    SearchPathCount = 0;
    SearchPathMax = 0;
    FreeString(&Home);
}

// PANZERS 0x65de60
SSearchPathElement* SFileSystem::GetSearchPathElement(int index)
{
    if (index < 0 || index >= SearchPathCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SSearchPathElement", index);
    return &SearchPath[index];
}

// PANZERS 0x65df20
int SFileSystem::AddSearchPathElement()
{
    if (SearchPathCount == SearchPathMax) {
        int newmax = (SearchPathMax < 16) ? 16 : (SearchPathMax * 6) / 5;
        SearchPath = (SSearchPathElement*)realloc(SearchPath, newmax * sizeof(SSearchPathElement));
        memset(&SearchPath[SearchPathMax], 0, (newmax - SearchPathMax) * sizeof(SSearchPathElement));
        SearchPathMax = newmax;
    }
    return SearchPathCount++;
}

// PANZERS 0x65ed80
int SFileSystem::InsertSearchPathElement(int index)
{
    if (index < 0 || index > SearchPathCount)
        Logger.g->Panic("SDArray<%s>::Insert: invalid index (%d)", "struct SSearchPathElement", index);
    if (SearchPathCount == SearchPathMax) {
        int newmax = (SearchPathMax < 16) ? 16 : (SearchPathMax * 6) / 5;
        SearchPath = (SSearchPathElement*)realloc(SearchPath, newmax * sizeof(SSearchPathElement));
        memset(&SearchPath[SearchPathMax], 0, (newmax - SearchPathMax) * sizeof(SSearchPathElement));
        SearchPathMax = newmax;
    }
    if (index < SearchPathCount)
        memmove(&SearchPath[index + 1], &SearchPath[index], (SearchPathCount - index) * sizeof(SSearchPathElement));
    memset(&SearchPath[index], 0, sizeof(SSearchPathElement));
    SearchPathCount++;
    return index;
}

// PANZERS 0x65df90
// Splits [Paths] Search on ';'. Every element goes through _fullpath().
// Names ending in ".pak" (case-insensitive) become archives and are opened
// at once (OpenArchive, which panics on failure). Everything else is a
// directory with a '/' appended. With prepend set, each element is inserted
// at index 0, so the list ends up reversed. Lookup walks the array in order,
// so the first element listed in Search wins.
void SFileSystem::SetSearchPath(const char* paths, bool prepend)
{
    char element[260];
    char full[264];
    while (paths && *paths) {
        const char* semi = strchr(paths, ';');
        if (!semi) {
            strncpy(element, paths, sizeof(element) - 1);
            element[sizeof(element) - 1] = 0;
            paths = nullptr;
        } else {
            memset(element, 0, sizeof(element));
            int n = (int)(semi - paths);
            if (n > (int)sizeof(element) - 1)
                n = (int)sizeof(element) - 1;
            strncpy(element, paths, n);
            paths = semi + 1;
        }
        if (!_fullpath(full, element, 260))
            strcpy(full, element);
        int len = (int)strlen(full);
        if (len > 4 && _stricmp(&full[len - 4], ".pak") == 0) {
            int idx = prepend ? InsertSearchPathElement(0) : AddSearchPathElement();
            SSearchPathElement* e = GetSearchPathElement(idx);
            e->Type = SEARCHPATH_ARCHIVE;
            e->Name = full;
            OpenArchive(idx);
            continue;
        }
        if (len == 0 || (full[len - 1] != '/' && full[len - 1] != '\\'))
            strcat(full, "/");
        int idx = prepend ? InsertSearchPathElement(0) : AddSearchPathElement();
        SSearchPathElement* e = GetSearchPathElement(idx);
        e->Type = SEARCHPATH_DIRECTORY;
        e->Name = full;
    }
}

// PANZERS 0x65fa30
void SFileSystem::SetHomePath(const char* path)
{
    FreeString(&Home);
    if (path) {
        char full[264];
        if (!_fullpath(full, path, 260))
            strcpy(full, path);
        int len = (int)strlen(full);
        if (len == 0 || (full[len - 1] != '/' && full[len - 1] != '\\'))
            strcat(full, "/");
        Home = full;
    }
}

// PANZERS 0x65ed70
const char* SFileSystem::GetHomePath()
{
    return Home.buf ? Home.buf : "";
}

// PANZERS 0x65f020
// Loose files are resolved against Home. Absolute names pass through.
void SFileSystem::FileNameProcess(SString* result, const char* filename)
{
    if (!IsFullPath(filename)) {
        ConcatString(result, Home, filename);
        return;
    }
    result->size = (int)strlen(filename);
    result->buf = new char[result->size + 1];
    memcpy(result->buf, filename, result->size + 1);
}

// PANZERS 0x65f1e0
// Panzers accepts only the "Sr\x1a\x1b" magic, then "\r\n\x87\n", "PACK" and
// the TOC size. The TOC is read whole. Any failure is fatal.
void SFileSystem::OpenArchive(int index)
{
    SSearchPathElement* e = GetSearchPathElement(index);
    const char* name = e->Name.buf ? e->Name.buf : "";
    int fd;
    if (_sopen_s(&fd, name, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0) != 0)
        fd = -1;
    if (fd < 0)
        Logger.g->Panic("SFileSystem::OpenArchive: Couldn't open (%s)", name);
    int v;
    if (_read(fd, &v, 4) != 4 || v != 0x1B1A7253)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Not a Stormregion file", name);
    if (_read(fd, &v, 4) != 4 || v != 0x0A870A0D)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Not a Stormregion file", name);
    if (_read(fd, &v, 4) != 4 || v != 0x4B434150)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Not a 'PACK' file", name);
    if (_read(fd, &v, 4) != 4)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Read error", name);
    e->TocSize = v;
    e->Toc = new unsigned char[v];
    if (_read(fd, e->Toc, e->TocSize) != e->TocSize)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Read error", name);
    _close(fd);
}

// PANZERS 0x65ee70
// The name is lower-cased and '\' becomes '/'. The node walk is the same as
// SWINE's SArchiveInfo::Lookup. A directory node (size 0) matches its own
// name.
SArchiveHeaderEntry* SFileSystem::LookupArchive(int index, const char* filename)
{
    if (index < 0 || index >= SearchPathCount || SearchPath[index].Type != SEARCHPATH_ARCHIVE
        || IsFullPath(filename))
        Logger.g->Panic("SFileSystem::LookupArchive: Invalid parameters");

    // Zero-filled so that a prefix longer than the name compares as "".
    char name[260];
    memset(name, 0, sizeof(name));
    strncpy(name, filename, sizeof(name) - 1);
    _strlwr(name);
    for (char* p = name; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    int nameLen = (int)strlen(name);

    unsigned char* toc = GetSearchPathElement(index)->Toc;
    unsigned char* node = toc;
    while (true) {
        unsigned char prefixStart = node[0];
        unsigned char prefixLen = node[1];
        const char* suffix = (const char*)(node + 2);
        int cmp = strncmp(&name[prefixStart], suffix, prefixLen);
        unsigned char* after = (unsigned char*)suffix + prefixLen;
        if (cmp < 0) {
            if (!after[8])
                return nullptr;
            node = after + 13;
            continue;
        }
        if (cmp == 0 && nameLen <= prefixStart + prefixLen)
            return (SArchiveHeaderEntry*)after;
        int rightOffset = *(int*)(after + 9);
        if (!rightOffset)
            return nullptr;
        node = toc + rightOffset;
    }
}

// SWINE API: add one search path element (directory or .pak).
void SFileSystem::AddSearchPath(const char* path)
{
    SetSearchPath(path, false);
}

//----- (004FC460) --------------------------------------------------------

char* SFileSystem::FindFileInSearchPath(const char* filename)
{
    if (!IsFullPath(filename)) {
        for (int i = 0; i < SearchPathCount; i++) {
            const SSearchPathElement* e = &SearchPath[i];
            if (e->Type == SEARCHPATH_DIRECTORY && e->Name.buf) {
                strcpy(fullname_0, e->Name.buf);
                strcat(fullname_0, filename);
                struct _stat tmpbuf;
                if (_stat(fullname_0, &tmpbuf) == 0)
                    return fullname_0;
            }
        }
    }
    strcpy(fullname_0, MakeFullPath(filename));
    return fullname_0;
}

//----- (004FD250) --------------------------------------------------------

char* SFileSystem::GetGameDataPath()
{
    return (char*)GetHomePath();
}

//----- (004FEC60) --------------------------------------------------------

void SFileSystem::SetGameDataPath(const char* pathname)
{
    SetHomePath(pathname ? pathname : ".");
    if (Logger.g)
        Logger.g->Log(1, "Working Directory = %s", GetHomePath());
}

//----- (004FD260) --------------------------------------------------------

bool SFileSystem::IsFullPath(const char* filename)
{
    char c = *filename;
    return c == '/' || c == '\\' || (c && filename[1] == ':');
}

//----- (004FD770) --------------------------------------------------------

// Panzers has no per-user data folder: logs, options.ini and saves live
// under Home. This is the SWINE entry point, resolved like MakeFullPath.
const char* SFileSystem::MakeAppDataPath(const char* filename)
{
    strcpy(fullname_3, MakeFullPath(filename));
    return fullname_3;
}

//----- (004FD810) --------------------------------------------------------

char* SFileSystem::MakeFullPath(const char* filename)
{
    SString full;
    FileNameProcess(&full, filename);
    strncpy(fullname_2, full.buf ? full.buf : "", sizeof(fullname_2) - 1);
    fullname_2[sizeof(fullname_2) - 1] = 0;
    FreeString(&full);
    return fullname_2;
}

//----- (004FD8A0) --------------------------------------------------------

void SFileSystem::MakePath(SString pathname)
{
    const char* path = pathname.buf ? pathname.buf : "";
    if (_mkdir(path) < 0 && errno == ENOENT) {
        SString dirname = {0, 0};
        SString::Dirname(&pathname, &dirname);
        char* dirBuf = dirname.buf;
        bool shouldRecurse = true;
        if (dirBuf) {
            if (strcmp(dirBuf, ".") == 0)
                shouldRecurse = false;
        }
        if (shouldRecurse) {
            SString dirCopy;
            if (dirname.buf) {
                dirCopy.size = dirname.size;
                dirCopy.buf = (char*)operator new[](dirname.size + 1);
                memcpy(dirCopy.buf, dirname.buf, dirname.size + 1);
            } else {
                dirCopy.buf = 0;
                dirCopy.size = 0;
            }
            MakePath(dirCopy);
            _mkdir(pathname.buf ? pathname.buf : "");
        }
        if (dirBuf)
            operator delete(dirBuf);
    }
    if (pathname.buf)
        delete[] pathname.buf;
}

// PANZERS 0x65f420
// Order: every Search element in list order (directories: Name + filename
// via _sopen_s; archives: LookupArchive, then fopen(pak, "rb")), then the
// loose file relative to Home. A directory open that fails with anything
// other than ENOENT is fatal when panicstr is set. SWINE's "FILE OPEN
// READ"/"ARCHIVED FILE" log lines do not exist in Panzers.
SStream* SFileSystem::OpenRead(const char* filename, const char* panicstr)
{
    if (!IsFullPath(filename)) {
        for (int i = 0; i < SearchPathCount; i++) {
            SSearchPathElement* e = &SearchPath[i];
            if (e->Type == SEARCHPATH_DIRECTORY) {
                SString full;
                ConcatString(&full, e->Name, filename);
                const char* fn = full.buf ? full.buf : "";
                int fd;
                if (_sopen_s(&fd, fn, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0) != 0)
                    fd = -1;
                if (fd >= 0) {
                    FreeString(&full);
                    return new SFileStream(fd, true, false);
                }
                if (errno != ENOENT) {
                    if (panicstr)
                        Logger.g->Panic("%s: Couldn't open (%s)", panicstr, fn);
                    FreeString(&full);
                    return nullptr;
                }
                FreeString(&full);
            } else if (e->Type == SEARCHPATH_ARCHIVE) {
                SArchiveHeaderEntry* entry = LookupArchive(i, filename);
                if (!entry)
                    continue;
                const char* pak = e->Name.buf ? e->Name.buf : "";
                FILE* f = fopen(pak, "rb");
                if (f)
                    return new SArchiveStream(f, entry->Pos + 16 + e->TocSize, entry->Size);
                if (!panicstr)
                    return nullptr;
                Logger.g->Panic("%s: Couldn't open (%s)", panicstr, pak);
            }
        }
    }

    SString full;
    FileNameProcess(&full, filename);
    const char* fn = full.buf ? full.buf : "";
    int fd;
    if (_sopen_s(&fd, fn, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0) != 0)
        fd = -1;
    if (fd >= 0) {
        FreeString(&full);
        return new SFileStream(fd, true, false);
    }

#ifdef HDB_MISSING_ASSET_FALLBACK
    // SWINE HD remaster extension, not in PANZERS.exe: substitute
    // editor/missing.<ext> for known asset extensions instead of failing.
    {
        static const char *fallback_exts[] = { ".4d", ".dxt" };
        const char *dot = strrchr(filename, '.');
        bool already_fallback =
            _strnicmp(filename, "editor/missing.",  15) == 0 ||
            _strnicmp(filename, "editor\\missing.", 15) == 0;
        if (dot && !already_fallback) {
            for (size_t i = 0; i < sizeof(fallback_exts) / sizeof(fallback_exts[0]); i++) {
                if (_stricmp(dot, fallback_exts[i]) == 0) {
                    char fb_path[260];
                    _snprintf(fb_path, sizeof(fb_path), "editor/missing%s", fallback_exts[i]);
                    fb_path[sizeof(fb_path) - 1] = 0;
                    Logger.g->Log(0, "MISSING ASSET: %s -> falling back to %s", filename, fb_path);
                    SStream *fb = OpenRead(fb_path, nullptr);
                    if (fb) {
                        FreeString(&full);
                        return fb;
                    }
                    Logger.g->Log(0, "MISSING ASSET: %s -> fallback %s also missing", filename, fb_path);
                    break;
                }
            }
        }
    }
#endif

    if (panicstr)
        Logger.g->Panic("%s: Couldn't open (%s): %s", panicstr, fn, strerror(errno));
    FreeString(&full);
    return nullptr;
}

// PANZERS 0x65f7a0
// Relative to Home. Unlike SWINE, Panzers does not create missing parent
// directories, and logs the failure when panicstr is null.
SStream* SFileSystem::OpenWrite(const char* filename, const char* panicstr)
{
    SString full;
    FileNameProcess(&full, filename);
    const char* fn = full.buf ? full.buf : "";
    int fd;
    if (_sopen_s(&fd, fn, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0)
        fd = -1;
    if (fd >= 0) {
        FreeString(&full);
        return new SFileStream(fd, false, true);
    }
    if (panicstr)
        Logger.g->Panic("%s: Couldn't open (%s): %s", panicstr, fn, strerror(errno));
    Logger.g->Log(0, "SFileSystem::OpenWrite: Couldn't open (%s): %s", fn, strerror(errno));
    FreeString(&full);
    return nullptr;
}

// PANZERS 0x65f0a0
SStream* SFileSystem::OpenAppend(const char* filename, const char* panicstr)
{
    SString full;
    FileNameProcess(&full, filename);
    const char* fn = full.buf ? full.buf : "";
    int fd;
    if (_sopen_s(&fd, fn, _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0)
        fd = -1;
    if (fd >= 0) {
        FreeString(&full);
        return new SFileStream(fd, false, true);
    }
    if (panicstr)
        Logger.g->Panic("%s: Couldn't open (%s)", panicstr, fn);
    Logger.g->Log(0, "SFileSystem::OpenAppend: Couldn't open (%s): %s", fn, strerror(errno));
    FreeString(&full);
    return nullptr;
}

// PANZERS 0x65f8e0
// Same as SWINE. One deliberate change: the buffer gets one extra byte,
// set to 0. The HD SProperties parser (0x660840) writes a terminator one
// past the data when the file does not end in a newline, which overflows
// the HD's exact-size buffer.
void SFileSystem::ReadFile(const char* filename, char** buf, unsigned int* size, const char* caller)
{
    *buf = 0;
    *size = 0;
    SStream* is = OpenRead(filename, caller);
    if (is) {
        *size = is->Seek(0, 2);
        is->Seek(0, 0);
        *buf = (char*)operator new[](*size + 1);
        (*buf)[*size] = 0;
        is->Read(*buf, *size);
        is->Release();
    }
}

//----- (004FEA10) --------------------------------------------------------

void SFileSystem::RemoveAllSearchPaths()
{
    for (int i = 0; i < SearchPathCount; i++)
        FreeSearchPathElement(&SearchPath[i]);
    SearchPathCount = 0;
    if (SearchPath)
        memset(SearchPath, 0, sizeof(SSearchPathElement) * SearchPathMax);
}

//----- (004FED40) --------------------------------------------------------

// SWINE signature. The HD equivalent (0x65faf0) fills its own 0x30-byte
// struct, but uses the same order as OpenRead: directories by
// GetFileAttributesEx, archives by LookupArchive, then Home.
int SFileSystem::Stat(const char* filename, struct _stat* buffer)
{
    struct _stat tmpbuf;
    if (!buffer)
        buffer = &tmpbuf;
    if (!IsFullPath(filename)) {
        for (int i = 0; i < SearchPathCount; i++) {
            SSearchPathElement* e = &SearchPath[i];
            if (e->Type == SEARCHPATH_DIRECTORY) {
                SString full;
                ConcatString(&full, e->Name, filename);
                int result = _stat(full.buf ? full.buf : "", buffer);
                FreeString(&full);
                if (result == 0)
                    return 0;
            } else if (e->Type == SEARCHPATH_ARCHIVE) {
                SArchiveHeaderEntry* entry = LookupArchive(i, filename);
                if (entry) {
                    memset(buffer, 0, sizeof(struct _stat));
                    buffer->st_nlink = 1;
                    buffer->st_size = entry->Size;
                    return 0;
                }
            }
        }
    }
    return _stat(MakeFullPath(filename), buffer);
}

//----- (004FD0E0) --------------------------------------------------------

bool SFileSystem::GenerateFileCRC(const char* filename, unsigned int* result, const char* panicstr)
{
    unsigned int crc = 0xFFFFFFFF;
    SStream* is = OpenRead(filename, panicstr);
    if (!is)
        return false;
    char buf[4096];
    while (true) {
        int bytesRead = is->ReadMax(buf, 4096);
        if (bytesRead <= 0)
            break;
        for (int i = 0; i < bytesRead; i++)
            crc = crctable[(unsigned char)(crc ^ buf[i])] ^ (crc >> 8);
    }
    is->Release();
    *result = ~crc;
    return true;
}

//----- (004FD220) --------------------------------------------------------

int SFileSystem::GetCRC(const char* buffer, int length)
{
    unsigned int crc = 0xFFFFFFFF;
    for (int i = 0; i < length; i++)
        crc = crctable[(unsigned char)(crc ^ buffer[i])] ^ (crc >> 8);
    return ~crc;
}

//----- (004FC6D0) --------------------------------------------------------

static void FindFilesInDirectory(const char* pattern, SDArray<SString>* result)
{
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
        return;
    do {
        bool duplicate = false;
        for (int i = 0; i < result->size; i++) {
            const char* existing = result->array[i].buf ? result->array[i].buf : "";
            if (_stricmp(existing, findData.cFileName) == 0) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            SString item;
            item.size = (int)strlen(findData.cFileName);
            item.buf = new char[item.size + 1];
            memcpy(item.buf, findData.cFileName, item.size + 1);
            result->Add(&item);
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
}

void SFileSystem::FindFiles(const char* path, const char* filter, SDArray<SString>* result)
{
    if (IsFullPath(path)) {
        strcpy(fullname_1, path);
        strcat(fullname_1, filter);
        FindFilesInDirectory(fullname_1, result);
        return;
    }
    for (int i = 0; i < SearchPathCount; i++) {
        SSearchPathElement* e = &SearchPath[i];
        if (e->Type == SEARCHPATH_DIRECTORY && e->Name.buf) {
            strcpy(fullname_1, e->Name.buf);
            strcat(fullname_1, path);
            strcat(fullname_1, filter);
            FindFilesInDirectory(fullname_1, result);
        } else if (e->Type == SEARCHPATH_ARCHIVE && e->Toc) {
            ArchiveFindFiles(e->Toc, path, filter, result);
        }
    }
    // Panzers also lists the loose directory under Home (HD 0x65e720).
    SString full;
    FileNameProcess(&full, path);
    strncpy(fullname_1, full.buf ? full.buf : "", sizeof(fullname_1) - 1);
    fullname_1[sizeof(fullname_1) - 1] = 0;
    FreeString(&full);
    strncat(fullname_1, filter, sizeof(fullname_1) - strlen(fullname_1) - 1);
    FindFilesInDirectory(fullname_1, result);
}

// SWINE EnumArchive entry point (extracts one archive below Home).
void SFileSystem::EnumArchive(int index)
{
    SSearchPathElement* e = GetSearchPathElement(index);
    if (e->Type != SEARCHPATH_ARCHIVE || !e->Toc)
        return;
    SString empty;
    ArchiveIterate(this, e, e->Toc, empty);
}
