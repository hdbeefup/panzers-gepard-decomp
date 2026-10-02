// 3dengine/stb_image_stubs.cpp
// stb_image implementation + SStream I/O callbacks for PNG loading
// Part of S.W.I.N.E. HD Remaster decompilation

#include "stream.h"

// SBitmap frees Data with delete[], so stb_image must allocate with new[]
#define STBI_MALLOC(sz)                  ((void*)new unsigned char[sz])
#define STBI_REALLOC_SIZED(p,oldsz,newsz) stbi_realloc_sized_impl(p, oldsz, newsz)
#define STBI_FREE(p)                     (delete[] (unsigned char*)(p))

static void* stbi_realloc_sized_impl(void* p, size_t oldsz, size_t newsz) {
    unsigned char* newp = new unsigned char[newsz];
    if (p) {
        memcpy(newp, p, oldsz < newsz ? oldsz : newsz);
        delete[] (unsigned char*)p;
    }
    return newp;
}

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#include "stb_image.h"

// SStream I/O callbacks bridging stb_image to the game's virtual file system

int stbi_read_stream(void* user, char* data, int size)
{
    SStream* stream = (SStream*)user;
    return stream->ReadMax(data, size);
}

void stbi_skip_stream(void* user, int n)
{
    SStream* stream = (SStream*)user;
    stream->Seek(n, 1); // SEEK_CUR
}

int stbi_eof_stream(void* user)
{
    SStream* stream = (SStream*)user;
    // Try to read 1 byte - if ReadMax returns 0, we're at EOF
    // Then seek back if we did read something
    char tmp;
    int got = stream->ReadMax(&tmp, 1);
    if (got > 0) {
        stream->Seek(-1, 1); // seek back
        return 0;
    }
    return 1;
}
