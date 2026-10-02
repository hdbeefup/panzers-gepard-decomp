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

//----- (004FE7F0) --------------------------------------------------------

void SStream::ReadSignature()
{
    int sig = 0;
    Read(&sig, 4);
    if (sig == 437940819)
        NewTypeStrings = false;
    else if (sig == 454718035)
        NewTypeStrings = true;
    else
        throw "Not a Stormregion file";
    Read(&sig, 4);
    if (sig != 176622093)
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

//----- (004FBD20) --------------------------------------------------------

SArchiveStream::SArchiveStream(int fd, int pos, int size)
{
    RefCount = 1;
    FileDes = fd;
    Pos = pos;
    Size = size;
    int seekResult = _lseek(fd, pos, SEEK_SET);
    Offset = seekResult - pos;
    if (seekResult < 0 || Offset < 0 || Offset > size) {
        Logger.g->Log(0, "Seek error (origin=%d, offset=%d, res=%d,  Pos=%d Size=%d Offset=%d)",
            0, 0, seekResult, pos, size, Offset);
        throw "Seek failed";
    }
}

//----- (004FBFE0) --------------------------------------------------------

SArchiveStream::~SArchiveStream()
{
    _close(FileDes);
    FileDes = -1;
}

//----- (004FC330) --------------------------------------------------------

void SArchiveStream::AddRef()
{
    ++RefCount;
}

//----- (004FE920) --------------------------------------------------------

void SArchiveStream::Release()
{
    if (RefCount-- == 1) {
        _close(FileDes);
        FileDes = -1;
        if (Chunks.array)
            free(Chunks.array);
        operator delete(this);
    }
}

//----- (004FE1F0) --------------------------------------------------------

void SArchiveStream::Read(void* buf, int size)
{
    int bytesRead = ReadMax(buf, size);
    if (bytesRead != size) {
        sprintf(err_1, "Read failed(%d): Expected %d bytes, read %d.", FileDes, size, bytesRead);
        throw (const char*)err_1;
    }
}

//----- (004FE660) --------------------------------------------------------

int SArchiveStream::ReadMax(void* buf, int size)
{
    int available = Size - Offset;
    if (size > available)
        size = available;
    int result = _read(FileDes, buf, size);
    if (result < 0) {
        char* errstr = strerror(errno);
        sprintf(err_2, "Read failed(%d): %s.", FileDes, errstr);
        throw (const char*)err_2;
    }
    Offset += result;
    return result;
}

//----- (004FEEA0) --------------------------------------------------------

void SArchiveStream::Write(const void* buf, int size)
{
    throw "Write failed";
}

//----- (004FEB20) --------------------------------------------------------

int SArchiveStream::Seek(int offset, int origin)
{
    int seekPos;
    if (origin == 0)
        seekPos = _lseek(FileDes, offset + Pos, SEEK_SET);
    else if (origin == 1)
        seekPos = _lseek(FileDes, offset, SEEK_CUR);
    else if (origin == 2)
        seekPos = _lseek(FileDes, offset + Pos + Size, SEEK_SET);
    else
        throw "Seek called with bad origin";

    int newOffset = seekPos - Pos;
    Offset = newOffset;
    if (seekPos < 0 || newOffset < 0 || newOffset > Size) {
        Logger.g->Log(0, "Seek error (origin=%d, offset=%d, res=%d,  Pos=%d Size=%d Offset=%d)",
            origin, offset, seekPos, Pos, Size, newOffset);
        throw "Seek failed";
    }
    return newOffset;
}

// ============================================================
// SArchiveInfo
// ============================================================

//----- (004FD6A0) --------------------------------------------------------

SArchiveHeaderEntry* SArchiveInfo::Lookup(const char* filename)
{
    // Copy and normalize filename (lowercase, forward slashes)
    strcpy(fullname_4, filename);
    _strlwr(fullname_4);
    for (char* p = fullname_4; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    int nameLen = (int)strlen(fullname_4);
    unsigned char* node = Header;

    while (true) {
        while (true) {
            unsigned char prefixStart = node[0];
            unsigned char prefixLen = node[1];
            const char* nodeStr = (const char*)(node + 2);
            int cmp = strncmp(&fullname_4[prefixStart], nodeStr, prefixLen);
            const char* after = nodeStr + prefixLen;

            if (cmp >= 0) {
                // Match or greater — check for exact match
                if (cmp == 0 && (int)(prefixStart + prefixLen) >= nameLen)
                    return (SArchiveHeaderEntry*)after;
                // Try right subtree
                int rightOffset = *(int*)(after + 9);
                if (!rightOffset)
                    return nullptr;
                node = &Header[rightOffset];
                break;
            }
            // Less — try left subtree
            if (!after[8])
                return nullptr;
            node = (unsigned char*)(after + 13);
        }
    }
}

//----- (004FC570) --------------------------------------------------------

void SArchiveInfo::FindFiles(const char* path, const char* filter, SDArray<SString>* result)
{
    // Copy and normalize path (lowercase, forward slashes)
    const char* s = path;
    do {
        fullname_5[s - path] = *s;
    } while (*s++);
    _strlwr(fullname_5);
    for (char* p = fullname_5; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    SString postfix;
    if (filter == (const char*)-1) {
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
    emptyName.buf = nullptr;
    emptyName.size = 0;
    FindFilesIterate(&pathStr, &postfix, result, Header, emptyName);

    if (pathStr.buf) {
        delete[] pathStr.buf;
        pathStr.buf = nullptr;
    }
    if (postfix.buf)
        delete[] postfix.buf;
}

//----- (004FC7D0) --------------------------------------------------------

void SArchiveInfo::FindFilesIterate(const SString* path, SString* postfix, SDArray<SString>* result, unsigned char* lookup_ptr, SString name)
{
    unsigned char prefixStart = lookup_ptr[0];
    unsigned char prefixLen = lookup_ptr[1];
    unsigned char* nodeStr = lookup_ptr + 2;

    // Allocate copy of node prefix string
    void* nodeCopy;
    if (prefixLen) {
        nodeCopy = operator new[](prefixLen + 1);
        memcpy(nodeCopy, nodeStr, prefixLen);
        ((char*)nodeCopy)[prefixLen] = 0;
    } else {
        nodeCopy = nullptr;
    }

    // Extract prefix from accumulated name (name.buf[0..prefixStart])
    int prefixTake;
    void* namePrefixCopy;
    if (name.size > 0 && prefixStart) {
        prefixTake = (prefixStart >= (unsigned int)name.size) ? name.size : prefixStart;
        namePrefixCopy = operator new[](prefixTake + 1);
        memcpy(namePrefixCopy, name.buf, prefixTake);
        ((char*)namePrefixCopy)[prefixTake] = 0;
    } else {
        prefixTake = 0;
        namePrefixCopy = nullptr;
    }

    // Build new name = namePrefixCopy + nodeCopy
    const char* nodeStr2 = nodeCopy ? (const char*)nodeCopy : "";
    unsigned int nodeLen = (unsigned int)strlen(nodeStr2);
    char* combined = new char[nodeLen + prefixTake + 1];
    memcpy(combined, namePrefixCopy, prefixTake);
    memcpy(combined + prefixTake, nodeStr2, nodeLen + 1);

    const char* assignStr = combined ? combined : "";
    name = assignStr;

    if (combined)
        operator delete(combined);
    if (namePrefixCopy)
        operator delete(namePrefixCopy);
    if (nodeCopy)
        operator delete(nodeCopy);

    unsigned char* after = nodeStr + prefixLen;

    // Recurse left child
    if (after[8]) {
        SString leftName;
        if (name.size) {
            leftName.size = name.size;
            leftName.buf = new char[name.size + 1];
            memcpy(leftName.buf, name.buf, name.size + 1);
        } else {
            leftName.buf = nullptr;
            leftName.size = 0;
        }
        FindFilesIterate(path, postfix, result, after + 13, leftName);
    }

    // Recurse right child
    int rightOffset = *(int*)(after + 9);
    if (rightOffset) {
        SString rightName;
        if (name.size) {
            rightName.size = name.size;
            rightName.buf = new char[name.size + 1];
            memcpy(rightName.buf, name.buf, name.size + 1);
        } else {
            rightName.buf = nullptr;
            rightName.size = 0;
        }
        FindFilesIterate(path, postfix, result, &Header[rightOffset], rightName);
    }

    // Filter: check if this entry's name starts with path
    const char* pathBuf = path->buf ? path->buf : "";
    const char* nameBuf = name.buf ? name.buf : "";
    if (strncmp(nameBuf, pathBuf, path->size) != 0)
        goto cleanup;

    // Skip if name equals path exactly (directory entry itself)
    if (name.size == path->size)
        goto cleanup;

    // Extract fileName = name after path prefix
    {
        const char* src = (name.buf ? name.buf : "") + path->size;
        char* dst = fileName;
        do {
            *dst++ = *src;
        } while (*src++);
    }

    // Extension/directory filter
    if (postfix->size == 0) {
        // Directory listing mode: name must end with '/'
        const char* nb = name.buf ? name.buf : "";
        if (nb[name.size - 1] != '/') {
            if (name.buf) delete[] name.buf;
            return;
        }
        // Truncate trailing slash from fileName
        unsigned int fnLen = name.size - path->size - 1;
        if (fnLen < 0x104)
            fileName[fnLen] = 0;
    }

    // Skip subdirectory entries (fileName contains '/')
    if (strchr(fileName, '/'))
        goto cleanup;

    // Extension filter
    if (postfix->size) {
        const char* postBuf = postfix->buf ? postfix->buf : "";
        const char* nb2 = name.buf ? name.buf : "";
        if (strcmp(&nb2[name.size - postfix->size], postBuf) != 0)
            goto cleanup;
    }

    // Dedup check: skip if fileName already in result
    {
        int i;
        for (i = 0; i < result->size; i++) {
            const char* existing = result->array[i].buf ? result->array[i].buf : "";
            if (_stricmp(existing, fileName) == 0)
                goto cleanup;
        }
        // Add to result
        SString item;
        int fnLen2 = (int)strlen(fileName);
        item.size = fnLen2;
        item.buf = new char[fnLen2 + 1];
        memcpy(item.buf, fileName, fnLen2 + 1);
        result->Add(&item);
        if (item.buf)
            delete[] item.buf;
    }

cleanup:
    if (name.buf)
        delete[] name.buf;
}

//----- (004FD290) --------------------------------------------------------

void SArchiveInfo::Iterate(unsigned char* lookup_ptr, SString name)
{
    unsigned char* node = lookup_ptr;
    if (!node)
        node = Header;

    unsigned char prefixStart = node[0];
    unsigned char prefixLen = node[1];
    char* nodeStr = (char*)(node + 2);

    // Allocate copy of node prefix string
    void* nodeCopy;
    if (prefixLen) {
        nodeCopy = operator new[](prefixLen + 1);
        memcpy(nodeCopy, nodeStr, prefixLen);
        ((char*)nodeCopy)[prefixLen] = 0;
    } else {
        nodeCopy = nullptr;
    }

    // Extract prefix from accumulated name
    int prefixTake;
    void* namePrefixCopy;
    if (name.size > 0 && prefixStart) {
        prefixTake = (prefixStart >= (unsigned int)name.size) ? name.size : prefixStart;
        namePrefixCopy = operator new[](prefixTake + 1);
        memcpy(namePrefixCopy, name.buf, prefixTake);
        ((char*)namePrefixCopy)[prefixTake] = 0;
    } else {
        prefixTake = 0;
        namePrefixCopy = nullptr;
    }

    // Build new name = namePrefixCopy + nodeCopy
    const char* nodeStr2 = nodeCopy ? (const char*)nodeCopy : "";
    unsigned int nodeLen = (unsigned int)strlen(nodeStr2);
    char* combined = new char[nodeLen + prefixTake + 1];
    memcpy(combined, namePrefixCopy, prefixTake);
    memcpy(combined + prefixTake, nodeStr2, nodeLen + 1);

    const char* assignStr = combined ? combined : "";
    name = assignStr;

    if (combined)
        operator delete(combined);
    if (namePrefixCopy)
        operator delete(namePrefixCopy);
    if (nodeCopy)
        operator delete(nodeCopy);

    char* after = nodeStr + prefixLen;

    // Recurse left child
    if (after[8]) {
        SString leftName;
        if (name.size) {
            leftName.size = name.size;
            leftName.buf = new char[name.size + 1];
            memcpy(leftName.buf, name.buf, name.size + 1);
        } else {
            leftName.buf = nullptr;
            leftName.size = 0;
        }
        Iterate((unsigned char*)after + 13, leftName);
    }

    // Recurse right child
    int rightOffset = *(int*)(after + 9);
    if (rightOffset) {
        SString rightName;
        if (name.size) {
            rightName.size = name.size;
            rightName.buf = new char[name.size + 1];
            memcpy(rightName.buf, name.buf, name.size + 1);
        } else {
            rightName.buf = nullptr;
            rightName.size = 0;
        }
        Iterate(&Header[rightOffset], rightName);
    }

    // Log this entry
    const char* logName = name.buf ? name.buf : "";
    Logger.g->Log(0, "EnumArchive: %s %d %d", logName, *(int*)after, *(int*)(after + 4));

    // Open archive file
    int fd;
    errno_t e = _sopen_s(&fd, FileName, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
    if (e) fd = -1;

    // Get output path
    const char* outPath = FileSystem.MakeFullPath(name.buf ? name.buf : "");
    char* outPathCopy;
    if (outPath) {
        unsigned int pathLen = (unsigned int)strlen(outPath);
        outPathCopy = new char[pathLen + 1];
        memcpy(outPathCopy, outPath, pathLen + 1);
    } else {
        outPathCopy = nullptr;
    }

    // Create archive stream for this entry
    SArchiveStream as(fd, HeaderSize + 16 + *(int*)after, *(int*)(after + 4));

    // Open output file and copy data
    SStream* outStream = FileSystem.OpenWrite(outPathCopy ? outPathCopy : "", "SFileSystem::EnumArchive");
    int remaining = *(int*)(after + 4);
    while (remaining) {
        int chunk = remaining;
        if (chunk > 0x4000)
            chunk = 0x4000;
        int bytesRead = as.ReadMax(buf_0, chunk);
        if (chunk != bytesRead) {
            sprintf(err_1, "Read failed(%d): Expected %d bytes, read %d.", as.FileDes, chunk, bytesRead);
            throw (const char*)err_1;
        }
        outStream->Write(buf_0, chunk);
        remaining -= chunk;
    }
    outStream->Release();

    // Cleanup archive stream manually (matches decompile)
    _close(as.FileDes);
    as.FileDes = -1;
    if (as.Chunks.array)
        free(as.Chunks.array);
    as.Chunks.array = nullptr;

    if (outPathCopy)
        operator delete(outPathCopy);
    if (name.buf)
        delete[] name.buf;
}

// ============================================================
// SFileSystem
// ============================================================

//----- (004FBE50) --------------------------------------------------------

SFileSystem::SFileSystem()
{
    SearchPaths.size = 0;
    SearchPaths.maxsize = 0;
    SearchPaths.array = 0;
    BasePath = 0;
#ifdef HD_PORTABLE_PATHS
    strcpy(AppDataPath, ".\\");
#else
    if (SHGetFolderPathA(0, CSIDL_LOCAL_APPDATA, 0, 0, AppDataPath) < 0)
        Logger.g->Panic("SFileSystem::SFileSystem: Can't get appdata path");
    strcat(AppDataPath, "\\Kite Games\\Swine\\");
#endif
}

//----- (004FC040) --------------------------------------------------------

SFileSystem::~SFileSystem()
{
    for (int i = 0; i < SearchPaths.size; i++) {
        SSearchPath* sp = &SearchPaths.array[i];
        if (sp->Path) {
            operator delete(sp->Path);
            sp->Path = 0;
        }
        if (sp->Archive.FileName) {
            operator delete(sp->Archive.FileName);
            sp->Archive.FileName = 0;
        }
        if (sp->Archive.Header) {
            operator delete(sp->Archive.Header);
            sp->Archive.Header = 0;
        }
    }
    if (SearchPaths.size && !SearchPaths.array)
        Logger.g->Panic("SDArray::Clear: array is damaged");
    int maxsize = SearchPaths.maxsize;
    SearchPaths.size = 0;
    if (maxsize < 0) {
        SearchPaths.maxsize = 0;
        SearchPaths.array = (SSearchPath*)realloc(SearchPaths.array, 0);
        maxsize = SearchPaths.maxsize;
    }
    memset(SearchPaths.array, 0, sizeof(SSearchPath) * maxsize);
    if (BasePath) {
        operator delete(BasePath);
        BasePath = 0;
    }
    if (SearchPaths.array) {
        free(SearchPaths.array);
        SearchPaths.array = 0;
    }
}

//----- (004FC360) --------------------------------------------------------

void SFileSystem::AddSearchPath(const char* path)
{
    int idx = SearchPaths.Add();
    SearchPaths.array[idx].Path = _strdup(path);
}

//----- (004FC460) --------------------------------------------------------

char* SFileSystem::FindFileInSearchPath(const char* filename)
{
    if (!IsFullPath(filename)) {
        for (int i = 0; i < SearchPaths.size; i++) {
            const char* searchDir = SearchPaths.array[i].Path;
            if (searchDir) {
                strcpy(fullname_0, searchDir);
                strcat(fullname_0, filename);
                struct _stat tmpbuf;
                if (_stat(fullname_0, &tmpbuf) == 0)
                    return fullname_0;
            }
        }
    }
    strcpy(fullname_0, filename);
    return fullname_0;
}

//----- (004FD250) --------------------------------------------------------

char* SFileSystem::GetGameDataPath()
{
    return BasePath;
}

//----- (004FEC60) --------------------------------------------------------

void SFileSystem::SetGameDataPath(const char* pathname)
{
    char buf[260];
    if (pathname)
        strcpy(buf, pathname);
    else
        GetCurrentDirectoryA(260, buf);

    int len = (int)strlen(buf);
    if (len > 0) {
        char last = buf[len - 1];
        if (last != '/' && last != '\\')
            strcat(buf, "\\");
    }
    if (BasePath) {
        operator delete(BasePath);
        BasePath = 0;
    }
    BasePath = _strdup(buf);
    Logger.g->Log(1, "Working Directory = %s", buf);
}

//----- (004FD260) --------------------------------------------------------

bool SFileSystem::IsFullPath(const char* filename)
{
    char c = *filename;
    return c == '/' || c == '\\' || (c && filename[1] == ':');
}

//----- (004FD770) --------------------------------------------------------

const char* SFileSystem::MakeAppDataPath(const char* filename)
{
    if (!AppDataPath[0] || IsFullPath(filename)) {
        strcpy(fullname_3, filename);
        return fullname_3;
    }
    strcpy(fullname_3, AppDataPath);
    strcat(fullname_3, filename);
    return fullname_3;
}

//----- (004FD810) --------------------------------------------------------

char* SFileSystem::MakeFullPath(const char* filename)
{
    if (!BasePath || IsFullPath(filename)) {
        strcpy(fullname_2, filename);
        return fullname_2;
    }
    strcpy(fullname_2, BasePath);
    strcat(fullname_2, filename);
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

//----- (004FDC40) --------------------------------------------------------

SStream* SFileSystem::OpenRead(const char* filename, const char* panicstr)
{
    if (!IsFullPath(filename)) {
        for (int i = 0; i < SearchPaths.size; i++) {
            SSearchPath* sp = &SearchPaths.array[i];
            if (sp->Path) {
                // Filesystem search path
                strcpy(fullname, sp->Path);
                strcat(fullname, filename);
                int fd;
                errno_t e = _sopen_s(&fd, fullname, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
                if (e == 0 && fd >= 0) {
                    Logger.g->Log(2, "FILE OPEN READ(%s) = %d", filename, fd);
                    return new SFileStream(fd, true, false);
                }
            } else {
                // Archive search path
                SArchiveHeaderEntry* entry = sp->Archive.Lookup(filename);
                if (entry) {
                    int fd;
                    errno_t e = _sopen_s(&fd, sp->Archive.FileName, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
                    if (e == 0 && fd >= 0) {
                        Logger.g->Log(2, "ARCHIVED FILE(%s)", filename);
                        return new SArchiveStream(fd, sp->Archive.HeaderSize + 16 + entry->Pos, entry->Size);
                    }
                    if (panicstr)
                        Logger.g->Panic("%s: Couldn't open (%s)", panicstr, sp->Archive.FileName);
                    return nullptr;
                }
            }
        }
    }
    // Direct file open
    int fd;
    errno_t e = _sopen_s(&fd, filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
    if (e == 0 && fd >= 0) {
        Logger.g->Log(2, "FILE OPEN READ(%s) = %d", filename, fd);
        return new SFileStream(fd, true, false);
    }

#ifdef HDB_MISSING_ASSET_FALLBACK
    // Substitute editor/missing.<ext> for known asset extensions instead of
    // crashing. Mirrors newer Stormregion engines (Codename Panzers ships
    // missing.4d / missing.dxt for the same purpose). Procedural
    // placeholders for .png/.tga are handled inside SBitmap.
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
                    // Log BEFORE the fallback open attempt so a chained crash
                    // (e.g. parser blows up inside missing.4d) still names the
                    // original asset that triggered the substitution.
                    Logger.g->Log(0, "MISSING ASSET: %s -> falling back to %s", filename, fb_path);
                    SStream *fb = OpenRead(fb_path, nullptr);
                    if (fb)
                        return fb;
                    Logger.g->Log(0, "MISSING ASSET: %s -> fallback %s also missing", filename, fb_path);
                    break;
                }
            }
        }
    }
#endif

    if (panicstr)
        Logger.g->Panic("%s: Couldn't open (%s)", panicstr, filename);
    return nullptr;
}

//----- (004FDF70) --------------------------------------------------------

SStream* SFileSystem::OpenWrite(const char* filename, const char* panicstr)
{
    char* fullpath = MakeFullPath(filename);
    int fd;
    errno_t e = _sopen_s(&fd, fullpath, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (e == 0 && fd >= 0) {
        Logger.g->Log(2, "FILE OPEN WRITE(%s) = %d", filename, fd);
        return new SFileStream(fd, false, true);
    }
    if (errno == ENOENT) {
        // Try creating parent directory
        SString pathStr = {_strdup(filename), (int)strlen(filename)};
        SString dirname = {0, 0};
        SString::Dirname(&pathStr, &dirname);
        if (pathStr.buf) delete[] pathStr.buf;
        bool retry = false;
        if (!dirname.buf || strcmp(dirname.buf, ".") != 0) {
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
            e = _sopen_s(&fd, fullpath, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
            if (e == 0 && fd >= 0)
                retry = true;
        }
        if (dirname.buf) delete[] dirname.buf;
        if (retry) {
            Logger.g->Log(2, "FILE OPEN WRITE(%s) = %d", filename, fd);
            return new SFileStream(fd, false, true);
        }
    }
    if (panicstr)
        Logger.g->Panic("%s: Couldn't open (%s)", panicstr, fullpath);
    return nullptr;
}

//----- (004FD9B0) --------------------------------------------------------

SStream* SFileSystem::OpenAppend(const char* filename, const char* panicstr)
{
    const char* fullpath = MakeFullPath(filename);
    int fd;
    errno_t e = _sopen_s(&fd, fullpath, _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (e == 0 && fd >= 0) {
        Logger.g->Log(2, "FILE OPEN APPEND(%s) = %d", filename, fd);
        return new SFileStream(fd, false, true);
    }
    if (panicstr)
        Logger.g->Panic("%s: Couldn't open (%s)", panicstr, fullpath);
    return nullptr;
}

//----- (004FDAB0) --------------------------------------------------------

SSearchPath* SFileSystem::OpenArchive(const char* filename)
{
    int fd;
    errno_t e = _sopen_s(&fd, filename, _O_RDONLY | _O_BINARY, _SH_DENYNO, 0);
    if (e != 0 || fd < 0)
        Logger.g->Panic("SFileSystem::OpenArchive: Couldn't open (%s)", filename);
    int magic;
    if (_read(fd, &magic, 4) != 4 || magic != 454718035 ||
        _read(fd, &magic, 4) != 4 || magic != 176622093)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Not a Stormregion file", filename);
    if (_read(fd, &magic, 4) != 4 || magic != 1262698832)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Not a 'PACK' file", filename);
    int headerSize;
    if (_read(fd, &headerSize, 4) != 4)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Read error", filename);
    int idx = SearchPaths.Add();
    SSearchPath* sp = &SearchPaths.array[idx];
    sp->Archive.FileName = _strdup(filename);
    sp->Archive.HeaderSize = headerSize;
    sp->Archive.Header = (unsigned char*)operator new[](headerSize);
    if (_read(fd, sp->Archive.Header, sp->Archive.HeaderSize) != sp->Archive.HeaderSize)
        Logger.g->Panic("SFileSystem::OpenArchive: %s: Read error", filename);
    _close(fd);
    return sp;
}

//----- (004FE4F0) --------------------------------------------------------

void SFileSystem::ReadFile(const char* filename, char** buf, unsigned int* size, const char* caller)
{
    *buf = 0;
    *size = 0;
    SStream* is = OpenRead(filename, caller);
    if (is) {
        *size = is->Seek(0, 2);
        is->Seek(0, 0);
        *buf = (char*)operator new[](*size);
        is->Read(*buf, *size);
        is->Release();
    }
}

//----- (004FEA10) --------------------------------------------------------

void SFileSystem::RemoveAllSearchPaths()
{
    for (int i = 0; i < SearchPaths.size; i++) {
        SSearchPath* sp = &SearchPaths.array[i];
        if (sp->Path) {
            operator delete(sp->Path);
            sp->Path = 0;
        }
        if (sp->Archive.FileName) {
            operator delete(sp->Archive.FileName);
            sp->Archive.FileName = 0;
        }
        if (sp->Archive.Header) {
            operator delete(sp->Archive.Header);
            sp->Archive.Header = 0;
        }
    }
    if (SearchPaths.size && !SearchPaths.array)
        Logger.g->Panic("SDArray::Clear: array is damaged");
    int maxsize = SearchPaths.maxsize;
    SearchPaths.size = 0;
    if (maxsize < 0) {
        SearchPaths.maxsize = 0;
        SearchPaths.array = (SSearchPath*)realloc(SearchPaths.array, 0);
        maxsize = SearchPaths.maxsize;
    }
    memset(SearchPaths.array, 0, sizeof(SSearchPath) * maxsize);
}

//----- (004FED40) --------------------------------------------------------

int SFileSystem::Stat(const char* filename, struct _stat* buffer)
{
    struct _stat tmpbuf;
    if (IsFullPath(filename))
        return buffer ? _stat(filename, buffer) : _stat(filename, &tmpbuf);

    for (int i = 0; i < SearchPaths.size; i++) {
        SSearchPath* sp = &SearchPaths.array[i];
        if (sp->Path) {
            strcpy(fullname_6, sp->Path);
            strcat(fullname_6, filename);
            int result = buffer ? _stat(fullname_6, buffer) : _stat(fullname_6, &tmpbuf);
            if (result == 0)
                return 0;
        } else {
            SArchiveHeaderEntry* entry = sp->Archive.Lookup(filename);
            if (entry) {
                if (buffer) {
                    memset(buffer, 0, sizeof(struct _stat));
                    buffer->st_nlink = 1;
                    buffer->st_size = entry->Size;
                }
                return 0;
            }
        }
    }
    return buffer ? _stat(filename, buffer) : _stat(filename, &tmpbuf);
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

void SFileSystem::FindFiles(const char* path, const char* filter, SDArray<SString>* result)
{
    if (IsFullPath(path)) {
        strcpy(fullname_1, path);
        strcat(fullname_1, filter);
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(fullname_1, &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                // Check for duplicates
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
                    item.buf = _strdup(findData.cFileName);
                    item.size = (int)strlen(findData.cFileName);
                    result->Add(&item);
                }
            } while (FindNextFileA(hFind, &findData));
            FindClose(hFind);
        }
    } else {
        for (int i = 0; i < SearchPaths.size; i++) {
            SSearchPath* sp = &SearchPaths.array[i];
            if (sp->Path) {
                strcpy(fullname_1, sp->Path);
                strcat(fullname_1, path);
                strcat(fullname_1, filter);
                WIN32_FIND_DATAA findData;
                HANDLE hFind = FindFirstFileA(fullname_1, &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        bool duplicate = false;
                        for (int j = 0; j < result->size; j++) {
                            const char* existing = result->array[j].buf ? result->array[j].buf : "";
                            if (_stricmp(existing, findData.cFileName) == 0) {
                                duplicate = true;
                                break;
                            }
                        }
                        if (!duplicate) {
                            SString item;
                            item.buf = _strdup(findData.cFileName);
                            item.size = (int)strlen(findData.cFileName);
                            result->Add(&item);
                        }
                    } while (FindNextFileA(hFind, &findData));
                    FindClose(hFind);
                }
            } else if (sp->Archive.Header) {
                // Archive search — delegate to SArchiveInfo
                sp->Archive.FindFiles(path, filter, result);
            }
        }
    }
}
