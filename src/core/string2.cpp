// core/string2.cpp — SString implementation
// Part of S.W.I.N.E. HD Remaster decompilation

#include "string2.h"
#include "stream.h"
#include <string.h>
#include <stdio.h>

//----- (00499860) --------------------------------------------------------
void SString::operator=(const char* str)
{
    if (buf) { delete[] buf; buf = nullptr; }
    if (str) {
        size = (int)strlen(str);
        buf = new char[size + 1];
        memcpy(buf, str, size + 1);
    } else {
        size = 0;
    }
}

//----- (004998C0) --------------------------------------------------------
void SString::operator=(const SString& other)
{
    if (buf) { delete[] buf; buf = nullptr; }
    if (other.buf) {
        size = other.size;
        buf = new char[size + 1];
        memcpy(buf, other.buf, size + 1);
    } else {
        size = 0;
    }
}

//----- (00499930) --------------------------------------------------------
void SString::Load(SStream *is)
{
    bool newTypeStrings = is->NewTypeStrings;
    int word = is->ReadWord();
    if (!newTypeStrings)
        --word;
    this->size = word;
    if (this->buf)
    {
        delete[] this->buf;
        this->buf = 0;
    }
    int sz = this->size;
    if (sz >= 0)
    {
        char *v7 = (char *)operator new[](sz + 1);
        this->buf = v7;
        if (!v7)
            throw "Out of memory";
        is->Read(v7, this->size);
        this->buf[this->size] = 0;
    }
}

//----- (004B2600) --------------------------------------------------------
void SString::Save(SStream *is)
{
    if (this->buf)
    {
        int sz = this->size;
        if (sz >= 0xFFFF)
            throw "String is too long";
        is->WriteWord(sz);
        is->Write(this->buf, this->size);
    }
    else
    {
        is->WriteWord(0);
    }
}

// Static methods

void SString::Format(SString *out, const char *format, ...)
{
    char tmp[1024];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(tmp, sizeof(tmp), format, args);
    va_end(args);
    if (out->buf) { delete[] out->buf; out->buf = nullptr; }
    if (len > 0 && len < 1024) {
        out->size = len;
        out->buf = new char[len + 1];
        memcpy(out->buf, tmp, len + 1);
    } else {
        out->size = 0;
    }
}

void SString::Dirname(const SString* source, SString* result)
{
    if (!source->buf) { result->buf = nullptr; result->size = 0; return; }
    const char* last_sep = nullptr;
    for (const char* p = source->buf; *p; p++) {
        if (*p == '/' || *p == '\\') last_sep = p;
    }
    if (!last_sep) {
        result->buf = (char*)operator new[](2);
        result->buf[0] = '.'; result->buf[1] = 0;
        result->size = 1;
    } else {
        int len = (int)(last_sep - source->buf);
        result->size = len;
        result->buf = (char*)operator new[](len + 1);
        memcpy(result->buf, source->buf, len);
        result->buf[len] = 0;
    }
}
