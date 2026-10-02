// core/string2.h
// SString class
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_STRING2_H
#define CORE_STRING2_H

#include "core_common.h"
#include <stdarg.h>

struct SString {
    char* buf = nullptr;
    int size = 0;

    void operator=(const char* str);
    void operator=(const SString& other);

    // Serialization (implemented in sstring.cpp)
    void Load(struct SStream *is);
    void Save(struct SStream *is);

    static void Format(SString *out, const char *format, ...);
    static void Dirname(const SString* source, SString* result);
};

#endif // CORE_STRING2_H
