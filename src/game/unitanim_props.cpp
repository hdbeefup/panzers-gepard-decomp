// src/game/unitanim_props.cpp
// SAnimProps: the property reader of the SP*Animation loaders, on the HD
// typed tree (SPropertyStruct) or on the flat .unit keys. OWNER: agent A.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unitanim.h"
#include "propertystruct.h"
#include "properties.h"
#include "logger.h"

namespace pz {

float (*SAnimProps::ResolveSymbol)(const char* value) = nullptr;

SAnimProps SAnimProps::FromTree(::SPropertyStruct* tree)
{
    SAnimProps p;
    p.Tree = tree;
    p.Flat = nullptr;
    p.Prefix[0] = 0;
    return p;
}

SAnimProps SAnimProps::FromFlat(::SProperties* file, const char* prefix)
{
    SAnimProps p;
    p.Tree = nullptr;
    p.Flat = file;
    _snprintf(p.Prefix, sizeof(p.Prefix) - 1, "%s", prefix ? prefix : "");
    p.Prefix[sizeof(p.Prefix) - 1] = 0;
    return p;
}

static void FlatKey(const SAnimProps& p, const char* name, char* key, size_t size)
{
    _snprintf(key, size - 1, "%s%s%s", p.Prefix, p.Prefix[0] ? "." : "", name);
    key[size - 1] = 0;
}

static const char* FlatValue(const SAnimProps& p, const char* name)
{
    char key[256];
    FlatKey(p, name, key, sizeof(key));
    return p.Flat->GetString("Unit", key, nullptr);
}

// First key component below "<prefix>.<name>." in the flat file: the name of
// the Multi alternative or of the array element struct.
static bool FlatChild(const SAnimProps& p, const char* name, char* out, size_t size)
{
    char key[256];
    FlatKey(p, name, key, sizeof(key));
    size_t n = strlen(key);
    p.Flat->EnumProperties("Unit");
    while (const char* k = p.Flat->GetNextProperty()) {
        if (_strnicmp(k, key, n) != 0 || k[n] != '.')
            continue;
        const char* c = k + n + 1;
        const char* e = strchr(c, '.');
        if (!e)
            continue;
        size_t len = (size_t)(e - c);
        if (len >= size)
            len = size - 1;
        memcpy(out, c, len);
        out[len] = 0;
        return true;
    }
    return false;
}

static ::SProperty* TreeArray(::SPropertyStruct* t, const char* name)
{
    for (::SProperty* c : t->Children)
        if (c->Type() == PROPERTY_TYPE_ARRAY && _stricmp(c->Name.c_str(), name ? name : "") == 0)
            return c;
    Logger.g->Panic("SPropertyTrack::GetArraySize(): Invalid property-tree.");
}

float SAnimProps::GetFloat(const char* name) const
{
    if (Tree)
        return Tree->GetFloat(name);
    const char* v = Flat ? FlatValue(*this, name) : nullptr;
    if (!v || !*v)
        return 0.0f;
    char* end = nullptr;
    double d = strtod(v, &end);
    if (end != v)
        return (float)d;
    return ResolveSymbol ? ResolveSymbol(v) : 0.0f;
}

int SAnimProps::GetInt(const char* name) const
{
    if (Tree)
        return Tree->GetInt(name);
    const char* v = Flat ? FlatValue(*this, name) : nullptr;
    return v ? atoi(v) : 0;
}

int SAnimProps::GetEnum(const char* name) const
{
    if (Tree)
        return Tree->GetEnum(name);
    return GetInt(name);
}

const char* SAnimProps::GetString(const char* name) const
{
    if (Tree)
        return Tree->GetString(name);
    const char* v = Flat ? FlatValue(*this, name) : nullptr;
    return v ? v : "";
}

int SAnimProps::GetMultiIndex(const char* name) const
{
    if (Tree)
        return Tree->GetMultiIndex(name);
    return GetInt(name);
}

SAnimProps SAnimProps::GetMultiSub(const char* name) const
{
    if (Tree)
        return FromTree(Tree->GetMultiSubStruct(name));
    SAnimProps r = FromFlat(Flat, "");
    char child[96];
    if (Flat && FlatChild(*this, name, child, sizeof(child))) {
        _snprintf(r.Prefix, sizeof(r.Prefix) - 1, "%s%s%s.%s", Prefix, Prefix[0] ? "." : "", name, child);
    } else {
        r.Flat = nullptr;          // no keys: an empty alternative
    }
    return r;
}

// PANZERS 0x6654a0
int SAnimProps::GetArraySize(const char* name) const
{
    if (Tree)
        return (int)static_cast<::SPropertyArray*>(TreeArray(Tree, name))->Children.size();
    return GetInt(name);
}

// PANZERS 0x665370
SAnimProps SAnimProps::GetArrayItem(const char* name, int i) const
{
    if (Tree) {
        ::SPropertyArray* a = static_cast<::SPropertyArray*>(TreeArray(Tree, name));
        if (i < 0 || i >= (int)a->Children.size())
            Logger.g->Panic("SPropertyStruct::GetArrayItem(): Invalid array property index.");
        return FromTree(static_cast<::SPropertyStruct*>(a->Children[i]));
    }
    char sub[128];
    _snprintf(sub, sizeof(sub) - 1, "%s.%d", name, i);
    sub[sizeof(sub) - 1] = 0;
    return GetMultiSub(sub);
}

} // namespace pz
