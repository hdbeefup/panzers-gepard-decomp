// core/propertystruct.cpp
// Panzers typed property trees. Lifted from HD PANZERS.exe 0x661c70..0x669d60
// (+ STrackFloat::Evaluate 0x6abda0). See propertystruct.h.

#include "propertystruct.h"
#include "properties.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define PROP_PANIC(msg) Logger.g->Panic(msg)

// ===========================================================================
// Schema
// ===========================================================================

// PANZERS 0x661d60
SPProperty::SPProperty(int type, const char* name, const char* desc)
    : Type(type), Name(name ? name : ""), Desc(desc ? desc : ""), Flag(false)
{
}

// PANZERS 0x662430
SPPropertyInt::SPPropertyInt(const char* name, const char* desc, int min, int max, int def)
    : SPProperty(PROPERTY_TYPE_INT, name, desc), Min(0), Max(0), Default(0)
{
    if (max < min)
        Logger.g->Warning("SPPropertyInt::SPPropertyInt(): Invalid initial range values.");
    else {
        Min = min;
        Max = max;
    }
    if (Max < def || def < Min)
        Logger.g->Warning("SPPropertyInt::SPPropertyInt(): Invalid initial default value.");
    else
        Default = def;
}

// PANZERS 0x661f90
SPPropertyColor::SPPropertyColor(const char* name, const char* desc, unsigned def)
    : SPProperty(PROPERTY_TYPE_COLOR, name, desc), Default(0)
{
    if (def < 0x1000000)
        Default = def;
    else
        Logger.g->Warning("SPPropertyColor::SPPropertyColor(): Invalid initial default value.");
}

// PANZERS 0x662330
SPPropertyFloat::SPPropertyFloat(const char* name, const char* desc, float min, float max, float def)
    : SPProperty(PROPERTY_TYPE_FLOAT, name, desc), Min(0.0f), Max(0.0f), Default(0.0f)
{
    if (max < min)
        Logger.g->Warning("SPPropertyFloat::SPPropertyFloat(): Invalid initial range values.");
    else {
        Min = min;
        Max = max;
    }
    if ((Max <= def && def != Max) || def < Min)
        Logger.g->Warning("SPPropertyFloat::SPPropertyFloat(): Invalid initial default value.");
    else
        Default = def;
}

// PANZERS 0x661ef0
SPPropertyBool::SPPropertyBool(const char* name, const char* desc, bool def)
    : SPProperty(PROPERTY_TYPE_BOOL, name, desc), Default(def)
{
}

// PANZERS 0x6626c0
SPPropertyString::SPPropertyString(const char* name, const char* desc, const char* def)
    : SPProperty(PROPERTY_TYPE_STRING, name, desc), Default(def ? def : "")
{
}

// PANZERS 0x662050
// HD then calls 0x669080, which keeps `def` only when it is one of the item
// values (the field starts at 0).
SPPropertyEnum::SPPropertyEnum(const char* name, const char* desc, int def,
                               std::initializer_list<SItem> items)
    : SPProperty(PROPERTY_TYPE_ENUM, name, desc), Items(items), Default(0)
{
    for (const SItem& it : Items)
        if (it.Value == def)
            Default = def;
}

// PANZERS 0x662790
SPPropertyStruct::SPPropertyStruct(const char* name, const char* desc, bool flag,
                                   std::initializer_list<SPProperty*> children)
    : SPProperty(PROPERTY_TYPE_STRUCT, name, desc), Children(children)
{
    Flag = flag;
}

SPPropertyStruct::~SPPropertyStruct()
{
    for (SPProperty* p : Children)
        delete p;
}

// PANZERS 0x662520
SPPropertyMulti::SPPropertyMulti(const char* name, const char* desc, int def,
                                 std::initializer_list<SPProperty*> children)
    : SPProperty(PROPERTY_TYPE_MULTI, name, desc), Children(children), Default(def)
{
    for (SPProperty* p : Children)
        if (p->Type != PROPERTY_TYPE_STRUCT)
            Logger.g->Panic("SPPropertyMulti::SPPropertyMulti(): Invalid property-tree. "
                            "PROPERTY_TYPE_MULTI properties should only contain PROPERTY_TYPE_STRUCT properies...");
}

SPPropertyMulti::~SPPropertyMulti()
{
    for (SPProperty* p : Children)
        delete p;
}

// PANZERS 0x5cfc10
SPPropertyArray::SPPropertyArray(const char* name, const char* desc, int min, int max, int def,
                                 SPProperty* element)
    : SPProperty(PROPERTY_TYPE_ARRAY, name, desc), Min(0), Max(0), Default(0), Element(element)
{
    if (max < min || min < 0)
        Logger.g->Warning("SPPropertyArray::SPPropertyArray(): Invalid initial array size range values.");
    else {
        Min = min;
        Max = max;
    }
    if (Max < def || def < Min)
        Logger.g->Warning("SPPropertyArray::SPPropertyArray(): Invalid initial array size default value.");
    else
        Default = def;
}

SPPropertyArray::~SPPropertyArray()
{
    delete Element;
}

// PANZERS 0x694170
SPPropertyTrack::SPPropertyTrack(const char* name, const char* desc, SPProperty* options, SPProperty* keys)
    : SPProperty(PROPERTY_TYPE_TRACK, name, desc), Options(options), Keys(keys)
{
}

SPPropertyTrack::~SPPropertyTrack()
{
    delete Keys;
    delete Options;
}

// PANZERS 0x664960
// Bool is built inline in HD (new 0x1c); the others go through one helper
// each (0x6650d0 Int, 0x664c30 Color, 0x664f70 Float, 0x665220 String,
// 0x664ca0 Enum, 0x665290 Struct, 0x665140 Multi, 0x665300 Track,
// 0x664e10 Expr). Expr is not used by the effect schema and is not ported.
SProperty* SPProperty::CreateInstance()
{
    switch (Type) {
    case PROPERTY_TYPE_INT: {
        SPPropertyInt* p = static_cast<SPPropertyInt*>(this);
        return new SPropertyInt(p, p->Default);
    }
    case PROPERTY_TYPE_COLOR: {
        SPPropertyColor* p = static_cast<SPPropertyColor*>(this);
        return new SPropertyColor(p, p->Default);
    }
    case PROPERTY_TYPE_FLOAT: {
        SPPropertyFloat* p = static_cast<SPPropertyFloat*>(this);
        return new SPropertyFloat(p, p->Default);
    }
    case PROPERTY_TYPE_BOOL: {
        SPPropertyBool* p = static_cast<SPPropertyBool*>(this);
        return new SPropertyBool(p, p->Default);
    }
    case PROPERTY_TYPE_STRING: {
        SPPropertyString* p = static_cast<SPPropertyString*>(this);
        return new SPropertyString(p, p->Default.c_str());
    }
    case PROPERTY_TYPE_ENUM: {
        SPPropertyEnum* p = static_cast<SPPropertyEnum*>(this);
        return new SPropertyEnum(p, p->Default);
    }
    case PROPERTY_TYPE_STRUCT:
        return new SPropertyStruct(static_cast<SPPropertyStruct*>(this));
    case PROPERTY_TYPE_MULTI: {
        SPPropertyMulti* p = static_cast<SPPropertyMulti*>(this);
        return new SPropertyMulti(p, p->Default);
    }
    case PROPERTY_TYPE_ARRAY: {
        SPPropertyArray* p = static_cast<SPPropertyArray*>(this);
        return new SPropertyArray(p, p->Default);
    }
    case PROPERTY_TYPE_TRACK:
        return new SPropertyTrack(static_cast<SPPropertyTrack*>(this));
    default:
        Logger.g->Warning("SPProperty::CreateInstance(): Invalid property type (%d).", Type);
        return nullptr;
    }
}

// ===========================================================================
// STrackFloat
// ===========================================================================

// PANZERS 0x6648a0
// The slope of the previous key is (dy / dx) to the new key; the last key's
// slope stays 0.
void STrackFloat::AddKey(float x, float y)
{
    SKey k = { x, y, 0.0f };
    Keys.push_back(k);
    int n = (int)Keys.size();
    if (n > 1) {
        SKey& a = Keys[n - 2];
        a.Slope = (Keys[n - 1].Y - a.Y) / (Keys[n - 1].X - a.X);
    }
}

// PANZERS 0x6abda0
// `cursor` caches the segment of the previous call (search from it first,
// then from 0). A looping track wraps t by the last key's X.
float STrackFloat::Evaluate(float t, int* cursor) const
{
    int n = (int)Keys.size();
    if (n == 1)
        return Keys[0].Y;
    if (n == 0)
        return 1.0f;
    if (Loop) {
        float len = Keys[n - 1].X;
        // HD: t - len * ROUND(t / len) with the x87 round-to-nearest of the
        // 0x007F control word.
        t = t - len * (float)(int)floor(t / len + 0.5f);
    }
    if (t < Keys[0].X)
        return Keys[0].Y;
    if (!Loop && Keys[n - 1].X < t)
        return Keys[n - 1].Y;

    int i = *cursor;
    if (i < n - 1) {
        for (; i < n - 1; ++i)
            if (Keys[i].X <= t && t < Keys[i + 1].X)
                break;
        if (i < n - 1)
            goto found;
    }
    for (i = 0; i < *cursor; ++i)
        if (Keys[i].X <= t && t < Keys[i + 1].X)
            break;
found:
    *cursor = i;
    if (i < 0 || i >= n)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct STrackFloat::SKey", i);
    return (t - Keys[i].X) * Keys[i].Slope + Keys[i].Y;
}

// ===========================================================================
// Instances
// ===========================================================================

SProperty::SProperty(SPProperty* proto)
    : Proto(proto), Name(proto->Name), Selected(true)
{
}

// PANZERS 0x6695a0
void SProperty::SetName(const char* name)
{
    Name = name ? name : "";
}

std::string SProperty::KeyOf(const char* prefix) const
{
    if (!prefix)
        return Name;
    std::string k(prefix);
    k += ".";
    k += Name;
    return k;
}

// PANZERS 0x6631b0
SPropertyInt::SPropertyInt(SPPropertyInt* proto, int value)
    : SProperty(proto)
{
    if (value < proto->Min)
        Value = proto->Min;
    else
        Value = value > proto->Max ? proto->Max : value;
}

// PANZERS 0x667420
bool SPropertyInt::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyInt* p = static_cast<SPPropertyInt*>(Proto);
    std::string key = KeyOf(prefix);
    int v = props->GetInt(section, key.c_str(), p->Default);
    if (v < p->Min)
        Value = p->Min;
    else
        Value = v > p->Max ? p->Max : v;
    return true;
}

// PANZERS 0x664c30
SPropertyColor::SPropertyColor(SPPropertyColor* proto, unsigned value)
    : SProperty(proto), Value(value)
{
}

// PANZERS 0x666db0
bool SPropertyColor::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyColor* p = static_cast<SPPropertyColor*>(Proto);
    std::string key = KeyOf(prefix);
    int v = props->GetInt(section, key.c_str(), (int)p->Default);
    if (v < 0)
        Value = 0;
    else
        Value = v > 0xffffff ? 0xffffff : (unsigned)v;
    return true;
}

// PANZERS 0x664f70
SPropertyFloat::SPropertyFloat(SPPropertyFloat* proto, float value)
    : SProperty(proto), Value(value)
{
}

// PANZERS 0x667290
bool SPropertyFloat::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyFloat* p = static_cast<SPPropertyFloat*>(Proto);
    std::string key = KeyOf(prefix);
    float v = props->GetFloat(section, key.c_str(), p->Default);
    if (v < p->Min)
        Value = p->Min;
    else
        Value = v <= p->Max ? v : p->Max;
    return true;
}

SPropertyBool::SPropertyBool(SPPropertyBool* proto, bool value)
    : SProperty(proto), Value(value)
{
}

// PANZERS 0x666c40
bool SPropertyBool::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyBool* p = static_cast<SPPropertyBool*>(Proto);
    std::string key = KeyOf(prefix);
    Value = props->GetInt(section, key.c_str(), p->Default ? 1 : 0) != 0;
    return true;
}

// PANZERS 0x665220
SPropertyString::SPropertyString(SPPropertyString* proto, const char* value)
    : SProperty(proto), Value(value ? value : "")
{
}

// PANZERS 0x667750
bool SPropertyString::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyString* p = static_cast<SPPropertyString*>(Proto);
    std::string key = KeyOf(prefix);
    const char* v = props->GetString(section, key.c_str(), p->Default.c_str());
    Value = v ? v : "";
    return true;
}

// PANZERS 0x664ca0
SPropertyEnum::SPropertyEnum(SPPropertyEnum* proto, int value)
    : SProperty(proto), Value(value)
{
}

// PANZERS 0x669aa0
// Values that are not one of the items are ignored.
int SPropertyEnum::SetValue(int value)
{
    SPPropertyEnum* p = static_cast<SPPropertyEnum*>(Proto);
    for (const SPPropertyEnum::SItem& it : p->Items) {
        if (it.Value == value) {
            Value = value;
            return value;
        }
    }
    return Value;
}

// PANZERS 0x666f20
bool SPropertyEnum::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyEnum* p = static_cast<SPPropertyEnum*>(Proto);
    std::string key = KeyOf(prefix);
    SetValue(props->GetInt(section, key.c_str(), p->Default));
    return true;
}

// PANZERS 0x663510
SPropertyStruct::SPropertyStruct(SPPropertyStruct* proto)
    : SProperty(proto)
{
    for (SPProperty* c : proto->Children)
        Children.push_back(c->CreateInstance());
}

SPropertyStruct::~SPropertyStruct()
{
    for (SProperty* c : Children)
        delete c;
}

// PANZERS 0x667920
bool SPropertyStruct::Load(SProperties* props, const char* section, const char* prefix)
{
    std::string key = KeyOf(prefix);
    for (SProperty* c : Children)
        c->Load(props, section, key.c_str());
    return true;
}

SProperty* SPropertyStruct::Find(int type, const char* name, const char* who)
{
    for (SProperty* c : Children) {
        if (c->Type() != type)
            continue;
        bool emptyName = c->Name.empty();
        bool emptyArg = !name || !*name;
        if (emptyName ? emptyArg : (!emptyArg && _stricmp(c->Name.c_str(), name) == 0))
            return c;
    }
    char msg[128];
    sprintf(msg, "SPropertyStruct::%s(): Invalid property-tree.", who);
    Logger.g->Panic("%s", msg);
}

SProperty* SPropertyStruct::At(int index, int type, const char* name, const char* who)
{
    if (index < 0 || index >= (int)Children.size())
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", index);
    SProperty* c = Children[index];
    bool emptyName = c->Name.empty();
    bool emptyArg = !name || !*name;
    if (c->Type() != type
        || !(emptyName ? emptyArg : (!emptyArg && _stricmp(c->Name.c_str(), name) == 0))) {
        char msg[128];
        sprintf(msg, "SPropertyStruct::%s(): Invalid property-tree.", who);
        Logger.g->Panic("%s", msg);
    }
    return c;
}

// PANZERS 0x665e50
int SPropertyStruct::GetInt(const char* name)
{
    return static_cast<SPropertyInt*>(Find(PROPERTY_TYPE_INT, name, "GetInt"))->Value;
}

// PANZERS 0x665610
bool SPropertyStruct::GetBool(const char* name)
{
    return static_cast<SPropertyBool*>(Find(PROPERTY_TYPE_BOOL, name, "GetBool"))->Value;
}

// PANZERS 0x665560
bool SPropertyStruct::GetBool(int index, const char* name)
{
    return static_cast<SPropertyBool*>(At(index, PROPERTY_TYPE_BOOL, name, "GetBool"))->Value;
}

// PANZERS 0x665780
unsigned SPropertyStruct::GetColor(const char* name)
{
    return static_cast<SPropertyColor*>(Find(PROPERTY_TYPE_COLOR, name, "GetColor"))->Value;
}

// PANZERS 0x665950
int SPropertyStruct::GetEnum(const char* name)
{
    return static_cast<SPropertyEnum*>(Find(PROPERTY_TYPE_ENUM, name, "GetEnum"))->Value;
}

// PANZERS 0x665c00
// HD also accepts an Expr child (value at +0x20); the effect schema has none.
float SPropertyStruct::GetFloat(const char* name)
{
    return static_cast<SPropertyFloat*>(Find(PROPERTY_TYPE_FLOAT, name, "GetFloat"))->Value;
}

// PANZERS 0x665b50
float SPropertyStruct::GetFloat(int index, const char* name)
{
    return static_cast<SPropertyFloat*>(At(index, PROPERTY_TYPE_FLOAT, name, "GetFloat"))->Value;
}

// PANZERS 0x665fc0
int SPropertyStruct::GetMultiIndex(const char* name)
{
    return static_cast<SPropertyMulti*>(Find(PROPERTY_TYPE_MULTI, name, "GetMultiIndex"))->Index;
}

// PANZERS 0x666190
SPropertyStruct* SPropertyStruct::GetMultiSubStruct(const char* name)
{
    return static_cast<SPropertyMulti*>(Find(PROPERTY_TYPE_MULTI, name, "GetMultiSubStruct"))->Current();
}

// PANZERS 0x666080
SPropertyStruct* SPropertyStruct::GetMultiSubStruct(int index, const char* name)
{
    return static_cast<SPropertyMulti*>(At(index, PROPERTY_TYPE_MULTI, name, "GetMultiSubStruct"))->Current();
}

// PANZERS 0x666360
const char* SPropertyStruct::GetString(const char* name)
{
    return static_cast<SPropertyString*>(Find(PROPERTY_TYPE_STRING, name, "GetString"))->Value.c_str();
}

// PANZERS 0x6662a0
const char* SPropertyStruct::GetString(int index, const char* name)
{
    return static_cast<SPropertyString*>(At(index, PROPERTY_TYPE_STRING, name, "GetString"))->Value.c_str();
}

// PANZERS 0x666430
SPropertyStruct* SPropertyStruct::GetStruct(int index, const char* name)
{
    return static_cast<SPropertyStruct*>(At(index, PROPERTY_TYPE_STRUCT, name, "GetStruct"));
}

// PANZERS 0x666640
SPropertyTrack* SPropertyStruct::GetTrack(const char* name)
{
    SProperty* p = nullptr;
    for (SProperty* c : Children) {
        if (c->Type() == PROPERTY_TYPE_TRACK
            && (c->Name.empty() ? (!name || !*name)
                                : (name && *name && _stricmp(c->Name.c_str(), name) == 0))) {
            p = c;
            break;
        }
    }
    if (!p)
        Logger.g->Panic("SPropertyTrack::GetTrack(): Invalid property-tree.");
    return static_cast<SPropertyTrack*>(p);
}

// PANZERS 0x667cf0
// Options.Loop (child 0) becomes the loop flag; each Keys element struct
// gives X (child 0) and Y (child 1). "Max. Y" is editor-only.
void SPropertyStruct::GetTrackFloat(const char* name, STrackFloat* out)
{
    SPropertyTrack* t = GetTrack(name);
    out->Loop = t->Options->GetBool(0, "Loop");
    int n = (int)t->Keys->Children.size();
    for (int i = 0; i < n; ++i) {
        SProperty* k = t->Keys->Children[i];
        if (k->Type() != PROPERTY_TYPE_STRUCT)
            Logger.g->Panic("SPropertyTrack::GetTrackFloat(): Invalid property-tree.");
        SPropertyStruct* ks = static_cast<SPropertyStruct*>(k);
        float x = ks->GetFloat(0, "X");
        float y = ks->GetFloat(1, "Y");
        out->AddKey(x, y);
    }
}

// PANZERS 0x665da0
int SPropertyStruct::GetInt(int index, const char* name)
{
    return static_cast<SPropertyInt*>(At(index, PROPERTY_TYPE_INT, name, "GetInt"))->Value;
}

// PANZERS 0x6656d0
unsigned SPropertyStruct::GetColor(int index, const char* name)
{
    return static_cast<SPropertyColor*>(At(index, PROPERTY_TYPE_COLOR, name, "GetColor"))->Value;
}

// PANZERS 0x6658a0
int SPropertyStruct::GetEnum(int index, const char* name)
{
    return static_cast<SPropertyEnum*>(At(index, PROPERTY_TYPE_ENUM, name, "GetEnum"))->Value;
}

// PANZERS 0x6654a0
// The first Array child of that name (HD's message names SPropertyTrack).
int SPropertyStruct::GetArraySize(const char* name)
{
    for (SProperty* c : Children) {
        if (c->Type() != PROPERTY_TYPE_ARRAY)
            continue;
        if (c->Name.empty() ? (!name || !*name) : (name && *name && _stricmp(c->Name.c_str(), name) == 0))
            return (int)static_cast<SPropertyArray*>(c)->Children.size();
    }
    Logger.g->Panic("SPropertyTrack::GetArraySize(): Invalid property-tree.");
    return 0;
}

// PANZERS 0x665370
SProperty* SPropertyStruct::GetArrayItem(const char* name, int index)
{
    for (SProperty* c : Children) {
        if (c->Type() != PROPERTY_TYPE_ARRAY)
            continue;
        if (c->Name.empty() ? (!name || !*name) : (name && *name && _stricmp(c->Name.c_str(), name) == 0)) {
            SPropertyArray* a = static_cast<SPropertyArray*>(c);
            if (index < 0 || index >= (int)a->Children.size())
                Logger.g->Panic("SPropertyStruct::GetArrayItem(): Invalid array property index.");
            return a->Children[index];
        }
    }
    Logger.g->Panic("SPropertyTrack::GetArraySize(): Invalid property-tree.");
    return nullptr;
}

// PANZERS 0x663250
SPropertyMulti::SPropertyMulti(SPPropertyMulti* proto, int index)
    : SProperty(proto), Index(index)
{
    for (SPProperty* c : proto->Children) {
        SProperty* inst = c->CreateInstance();
        inst->Selected = false;
        Children.push_back(inst);
    }
    if (index < 0 || index >= (int)Children.size())
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", index);
    Children[index]->Selected = true;
}

SPropertyMulti::~SPropertyMulti()
{
    for (SProperty* c : Children)
        delete c;
}

// PANZERS 0x669450
// An index out of range keeps the current selection.
int SPropertyMulti::SetIndex(int index)
{
    if (Index < 0 || Index >= (int)Children.size())
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", Index);
    Children[Index]->Selected = false;
    if (index >= 0 && index < (int)Children.size())
        Index = index;
    Children[Index]->Selected = true;
    return Index;
}

SPropertyStruct* SPropertyMulti::Current()
{
    if (Index < 0 || Index >= (int)Children.size())
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", Index);
    return static_cast<SPropertyStruct*>(Children[Index]);
}

// PANZERS 0x667590
// Reads the selected index, then loads every alternative under the same key.
bool SPropertyMulti::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyMulti* p = static_cast<SPPropertyMulti*>(Proto);
    std::string key = KeyOf(prefix);
    SetIndex(props->GetInt(section, key.c_str(), p->Default));
    for (SProperty* c : Children)
        c->Load(props, section, key.c_str());
    return true;
}

// PANZERS 0x662980
SPropertyArray::SPropertyArray(SPPropertyArray* proto, int count)
    : SProperty(proto)
{
    char name[64];
    for (int i = 0; i < count; ++i) {
        SProperty* c = proto->Element->CreateInstance();
        sprintf(name, "%d.%s", i, proto->Element->Name.c_str());
        c->SetName(name);
        Children.push_back(c);
    }
}

SPropertyArray::~SPropertyArray()
{
    Clear();
}

void SPropertyArray::Clear()
{
    for (SProperty* c : Children)
        delete c;
    Children.clear();
}

// PANZERS 0x6666f0
// The count is not clamped to the schema range.
bool SPropertyArray::Load(SProperties* props, const char* section, const char* prefix)
{
    SPPropertyArray* p = static_cast<SPPropertyArray*>(Proto);
    std::string key = KeyOf(prefix);
    int count = props->GetInt(section, key.c_str(), p->Default);
    Clear();
    char name[64];
    for (int i = 0; i < count; ++i) {
        SProperty* c = p->Element->CreateInstance();
        Children.push_back(c);
        sprintf(name, "%d.%s", i, c->Proto->Name.c_str());
        c->SetName(name);
        c->Load(props, section, key.c_str());
    }
    return true;
}

// PANZERS 0x6636b0
SPropertyTrack::SPropertyTrack(SPPropertyTrack* proto)
    : SProperty(proto)
{
    Options = new SPropertyStruct(static_cast<SPPropertyStruct*>(proto->Options));
    SPPropertyArray* keys = static_cast<SPPropertyArray*>(proto->Keys);
    Keys = new SPropertyArray(keys, keys->Default);
}

SPropertyTrack::~SPropertyTrack()
{
    delete Options;
    delete Keys;
}

// PANZERS 0x667ac0
bool SPropertyTrack::Load(SProperties* props, const char* section, const char* prefix)
{
    std::string key = KeyOf(prefix);
    if (Options->Load(props, section, key.c_str()) && Keys->Load(props, section, key.c_str()))
        return true;
    return false;
}
