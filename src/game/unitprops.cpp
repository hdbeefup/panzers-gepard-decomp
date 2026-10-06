// src/game/unitprops.cpp
// Unit property trees (HD 0x661c70..0x669d60 + the Expr type and its
// evaluator 0x676870). OWNER: agent U. See unitprops.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unitprops.h"
#include "properties.h"
#include "logger.h"

namespace pz {

SProperties* g_UnitVariables = nullptr;

// ===========================================================================
// Schema

// PANZERS 0x661d60
SUPProp::SUPProp(int type, const char* name, const char* desc)
    : Type(type), Name(name ? name : ""), Desc(desc ? desc : ""), Flag(false)
{
}

// PANZERS 0x662430
SUPPropInt::SUPPropInt(const char* name, const char* desc, int min, int max, int def)
    : SUPProp(UPROP_INT, name, desc), Min(min), Max(max), Default(def)
{
}

// PANZERS 0x662330
SUPPropFloat::SUPPropFloat(const char* name, const char* desc, float min, float max, float def)
    : SUPProp(UPROP_FLOAT, name, desc), Min(min), Max(max), Default(def)
{
}

// PANZERS 0x661ef0
SUPPropBool::SUPPropBool(const char* name, const char* desc, bool def)
    : SUPProp(UPROP_BOOL, name, desc), Default(def)
{
}

// PANZERS 0x6626c0
SUPPropString::SUPPropString(const char* name, const char* desc, const char* def)
    : SUPProp(UPROP_STRING, name, desc), Default(def ? def : "")
{
}

// PANZERS 0x662050
SUPPropEnum::SUPPropEnum(const char* name, const char* desc, int def, std::initializer_list<SItem> items)
    : SUPProp(UPROP_ENUM, name, desc), Items(items), Default(def)
{
}

// PANZERS 0x662790
SUPPropStruct::SUPPropStruct(const char* name, const char* desc, bool flag, std::initializer_list<SUPProp*> children)
    : SUPProp(UPROP_STRUCT, name, desc), Children(children)
{
    Flag = flag;
}

SUPPropStruct::~SUPPropStruct()
{
    for (SUPProp* c : Children)
        delete c;
}

// PANZERS 0x662520
SUPPropMulti::SUPPropMulti(const char* name, const char* desc, int def, std::initializer_list<SUPProp*> children)
    : SUPProp(UPROP_MULTI, name, desc), Children(children), Default(def)
{
}

SUPPropMulti::~SUPPropMulti()
{
    for (SUPProp* c : Children)
        delete c;
}

// PANZERS 0x5cfc10
SUPPropArray::SUPPropArray(const char* name, const char* desc, int min, int max, int def, SUPProp* element)
    : SUPProp(UPROP_ARRAY, name, desc), Min(min), Max(max), Default(def), Element(element)
{
}

SUPPropArray::~SUPPropArray()
{
    delete Element;
}

// PANZERS 0x6621e0
// HD validates the default text (0x668f90 only stores it) and panics on a
// bad range; the variables source is the unitvariables.ini global 0x929a28.
SUPPropExpr::SUPPropExpr(const char* name, const char* desc, float min, float max, const char* def)
    : SUPProp(UPROP_EXPR, name, desc), Min(0.0f), Max(0.0f), Default(def ? def : "")
{
    if (max < min) {
        if (Logger.g)
            Logger.g->Warning("SPPropertyExpr::SPPropertyExpr(): Invalid initial range values.");
    } else {
        Min = min;
        Max = max;
    }
}

// PANZERS 0x664960
SUProp* SUPProp::CreateInstance()
{
    switch (Type) {
    case UPROP_INT: {
        SUPPropInt* p = static_cast<SUPPropInt*>(this);
        return new SUPropInt(p, p->Default);
    }
    case UPROP_FLOAT: {
        SUPPropFloat* p = static_cast<SUPPropFloat*>(this);
        return new SUPropFloat(p, p->Default);
    }
    case UPROP_BOOL: {
        SUPPropBool* p = static_cast<SUPPropBool*>(this);
        return new SUPropBool(p, p->Default);
    }
    case UPROP_STRING: {
        SUPPropString* p = static_cast<SUPPropString*>(this);
        return new SUPropString(p, p->Default.c_str());
    }
    case UPROP_ENUM: {
        SUPPropEnum* p = static_cast<SUPPropEnum*>(this);
        return new SUPropEnum(p, p->Default);
    }
    case UPROP_STRUCT:
        return new SUPropStruct(static_cast<SUPPropStruct*>(this));
    case UPROP_MULTI: {
        SUPPropMulti* p = static_cast<SUPPropMulti*>(this);
        return new SUPropMulti(p, p->Default);
    }
    case UPROP_ARRAY: {
        SUPPropArray* p = static_cast<SUPPropArray*>(this);
        return new SUPropArray(p, p->Default);
    }
    case UPROP_EXPR:
        return new SUPropExpr(static_cast<SUPPropExpr*>(this));        // 0x664e10
    default:
        if (Logger.g)
            Logger.g->Warning("SPProperty::CreateInstance(): Invalid property type (%d).", Type);
        return nullptr;
    }
}

// ===========================================================================
// Instances

SUProp::SUProp(SUPProp* proto)
    : Proto(proto), Name(proto->Name), Selected(true)
{
}

std::string SUProp::KeyOf(const char* prefix) const
{
    if (!prefix)
        return Name;
    std::string k(prefix);
    k += ".";
    k += Name;
    return k;
}

// PANZERS 0x6631b0
SUPropInt::SUPropInt(SUPPropInt* proto, int value)
    : SUProp(proto)
{
    if (value < proto->Min)
        Value = proto->Min;
    else
        Value = value > proto->Max ? proto->Max : value;
}

// PANZERS 0x667420
void SUPropInt::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropInt* p = static_cast<SUPPropInt*>(Proto);
    std::string key = KeyOf(prefix);
    int v = props->GetInt(section, key.c_str(), p->Default);
    if (v < p->Min)
        Value = p->Min;
    else
        Value = v > p->Max ? p->Max : v;
}

// PANZERS 0x664f70
SUPropFloat::SUPropFloat(SUPPropFloat* proto, float value)
    : SUProp(proto), Value(value)
{
}

// PANZERS 0x667290
void SUPropFloat::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropFloat* p = static_cast<SUPPropFloat*>(Proto);
    std::string key = KeyOf(prefix);
    float v = props->GetFloat(section, key.c_str(), p->Default);
    if (v < p->Min)
        Value = p->Min;
    else
        Value = v <= p->Max ? v : p->Max;
}

SUPropBool::SUPropBool(SUPPropBool* proto, bool value)
    : SUProp(proto), Value(value)
{
}

// PANZERS 0x666c40
void SUPropBool::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropBool* p = static_cast<SUPPropBool*>(Proto);
    std::string key = KeyOf(prefix);
    Value = props->GetInt(section, key.c_str(), p->Default ? 1 : 0) != 0;
}

// PANZERS 0x665220
SUPropString::SUPropString(SUPPropString* proto, const char* value)
    : SUProp(proto), Value(value ? value : "")
{
}

// PANZERS 0x667750
void SUPropString::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropString* p = static_cast<SUPPropString*>(Proto);
    std::string key = KeyOf(prefix);
    const char* v = props->GetString(section, key.c_str(), p->Default.c_str());
    Value = v ? v : "";
}

// PANZERS 0x664ca0
SUPropEnum::SUPropEnum(SUPPropEnum* proto, int value)
    : SUProp(proto), Value(value)
{
}

// PANZERS 0x669aa0
int SUPropEnum::SetValue(int value)
{
    SUPPropEnum* p = static_cast<SUPPropEnum*>(Proto);
    for (const SUPPropEnum::SItem& it : p->Items) {
        if (it.Value == value) {
            Value = value;
            return value;
        }
    }
    return Value;
}

// PANZERS 0x666f20
void SUPropEnum::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropEnum* p = static_cast<SUPPropEnum*>(Proto);
    std::string key = KeyOf(prefix);
    SetValue(props->GetInt(section, key.c_str(), p->Default));
}

// PANZERS 0x662dd0
// Starts from the default expression (0x665840) through SetExpression.
SUPropExpr::SUPropExpr(SUPPropExpr* proto)
    : SUProp(proto), Value(0.0f)
{
    SetExpression(proto->Default.c_str());
}

// PANZERS 0x669220
// The value changes only when the text evaluates; the text is kept either
// way. HD does not clamp to the schema range here.
void SUPropExpr::SetExpression(const char* text)
{
    float v;
    if (EvalUnitExpression(g_UnitVariables, text ? text : "", &v))
        Value = v;
    Expr = text ? text : "";
}

// PANZERS 0x667080
void SUPropExpr::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropExpr* p = static_cast<SUPPropExpr*>(Proto);
    std::string key = KeyOf(prefix);
    const char* v = props->GetString(section, key.c_str(), p->Default.c_str());
    SetExpression(v);
}

// PANZERS 0x663510
SUPropStruct::SUPropStruct(SUPPropStruct* proto)
    : SUProp(proto)
{
    for (SUPProp* c : proto->Children)
        Children.push_back(c->CreateInstance());
}

SUPropStruct::~SUPropStruct()
{
    for (SUProp* c : Children)
        delete c;
}

// PANZERS 0x667920
void SUPropStruct::Load(SProperties* props, const char* section, const char* prefix)
{
    std::string key = KeyOf(prefix);
    Source = props;
    Key = key;
    for (SUProp* c : Children)
        c->Load(props, section, key.c_str());
}

SUProp* SUPropStruct::Find(int type, const char* name, const char* who)
{
    for (SUProp* c : Children) {
        if (!c || c->Type() != type)
            continue;
        bool emptyName = c->Name.empty();
        bool emptyArg = !name || !*name;
        if (emptyName ? emptyArg : (!emptyArg && _stricmp(c->Name.c_str(), name) == 0))
            return c;
    }
    Logger.g->Panic("SPropertyStruct::%s(): Invalid property-tree.", who);
    return nullptr;
}

// PANZERS 0x665e50
int SUPropStruct::GetInt(const char* name)
{
    return static_cast<SUPropInt*>(Find(UPROP_INT, name, "GetInt"))->Value;
}

// PANZERS 0x665610
bool SUPropStruct::GetBool(const char* name)
{
    return static_cast<SUPropBool*>(Find(UPROP_BOOL, name, "GetBool"))->Value;
}

// PANZERS 0x665950
int SUPropStruct::GetEnum(const char* name)
{
    return static_cast<SUPropEnum*>(Find(UPROP_ENUM, name, "GetEnum"))->Value;
}

// PANZERS 0x665c00
// One pass over the children: the first Float or Expr child of that name.
float SUPropStruct::GetFloat(const char* name)
{
    for (SUProp* c : Children) {
        if (!c || (c->Type() != UPROP_FLOAT && c->Type() != UPROP_EXPR))
            continue;
        bool emptyName = c->Name.empty();
        bool emptyArg = !name || !*name;
        if (!(emptyName ? emptyArg : (!emptyArg && _stricmp(c->Name.c_str(), name) == 0)))
            continue;
        if (c->Type() == UPROP_FLOAT)
            return static_cast<SUPropFloat*>(c)->Value;    // +0x18
        return static_cast<SUPropExpr*>(c)->Value;         // +0x20
    }
    Logger.g->Panic("SPropertyStruct::GetFloat(): Invalid property-tree.");
    return 0.0f;
}

// PANZERS 0x666360
const char* SUPropStruct::GetString(const char* name)
{
    return static_cast<SUPropString*>(Find(UPROP_STRING, name, "GetString"))->Value.c_str();
}

// PANZERS 0x6664e0
SUPropStruct* SUPropStruct::GetStruct(const char* name)
{
    return static_cast<SUPropStruct*>(Find(UPROP_STRUCT, name, "GetStruct"));
}

// PANZERS 0x665fc0
int SUPropStruct::GetMultiIndex(const char* name)
{
    return static_cast<SUPropMulti*>(Find(UPROP_MULTI, name, "GetMultiIndex"))->Index;
}

// PANZERS 0x666190
SUPropStruct* SUPropStruct::GetMultiSubStruct(const char* name)
{
    SUPropMulti* m = static_cast<SUPropMulti*>(Find(UPROP_MULTI, name, "GetMultiSubStruct"));
    if (m->Index < 0 || m->Index >= (int)m->Children.size())
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", m->Index);
    return static_cast<SUPropStruct*>(m->Children[m->Index]);
}

// PANZERS 0x6654a0
int SUPropStruct::GetArraySize(const char* name)
{
    return (int)static_cast<SUPropArray*>(Find(UPROP_ARRAY, name, "GetArraySize"))->Children.size();
}

// PANZERS 0x665370
SUPropStruct* SUPropStruct::GetArrayItem(const char* name, int index)
{
    SUPropArray* a = static_cast<SUPropArray*>(Find(UPROP_ARRAY, name, "GetArrayItem"));
    if (index < 0 || index >= (int)a->Children.size())
        Logger.g->Panic("SPropertyStruct::GetArrayItem(): Invalid array property index.");
    return static_cast<SUPropStruct*>(a->Children[index]);
}

// PANZERS 0x663250
SUPropMulti::SUPropMulti(SUPPropMulti* proto, int index)
    : SUProp(proto), Index(0)
{
    for (SUPProp* c : proto->Children) {
        SUProp* i = c->CreateInstance();
        i->Selected = false;
        Children.push_back(i);
    }
    SetIndex(index);
}

SUPropMulti::~SUPropMulti()
{
    for (SUProp* c : Children)
        delete c;
}

// PANZERS 0x669450
int SUPropMulti::SetIndex(int index)
{
    if (Index < 0 || Index >= (int)Children.size())
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", Index);
    Children[Index]->Selected = false;
    if (index >= 0 && index < (int)Children.size())
        Index = index;
    Children[Index]->Selected = true;
    return Index;
}

// PANZERS 0x667590
void SUPropMulti::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropMulti* p = static_cast<SUPPropMulti*>(Proto);
    std::string key = KeyOf(prefix);
    SetIndex(props->GetInt(section, key.c_str(), p->Default));
    for (SUProp* c : Children)
        c->Load(props, section, key.c_str());
}

// PANZERS 0x662980
SUPropArray::SUPropArray(SUPPropArray* proto, int count)
    : SUProp(proto)
{
    char name[64];
    for (int i = 0; i < count; ++i) {
        SUProp* c = proto->Element->CreateInstance();
        sprintf(name, "%d.%s", i, proto->Element->Name.c_str());
        c->SetName(name);
        Children.push_back(c);
    }
}

SUPropArray::~SUPropArray()
{
    Clear();
}

void SUPropArray::Clear()
{
    for (SUProp* c : Children)
        delete c;
    Children.clear();
}

// PANZERS 0x6666f0
void SUPropArray::Load(SProperties* props, const char* section, const char* prefix)
{
    SUPPropArray* p = static_cast<SUPPropArray*>(Proto);
    std::string key = KeyOf(prefix);
    int count = props->GetInt(section, key.c_str(), p->Default);
    Clear();
    char name[64];
    for (int i = 0; i < count; ++i) {
        SUProp* c = p->Element->CreateInstance();
        Children.push_back(c);
        sprintf(name, "%d.%s", i, c->Proto->Name.c_str());
        c->SetName(name);
        c->Load(props, section, key.c_str());
    }
}

// ===========================================================================
// HD 0x676870 / 0x676950 / 0x6769b0 / 0x676a10: recursive descent over the
// text, every intermediate result rounded to float. HD quirks kept:
// a leading '-' is skipped without negating (0x676a10 returns the factor as
// is), and an identifier that is not in [Values] reads as 0.

namespace {
struct SExprParser {
    SProperties* Vars;
    const char* P;
    bool Error;
    void Skip() { while (*P == ' ') ++P; }
    float Expr();
    float Term();
    float Factor();
};

// PANZERS 0x676a10
float SExprParser::Factor()
{
    Skip();
    char c = *P;
    if (c == '-') {
        ++P;
        return Factor();
    }
    if (c == '(') {
        ++P;
        float v = Expr();
        Skip();
        if (*P != ')') {
            Error = true;                                 // "Syntax error: ')' expected"
            return 0.0f;
        }
        ++P;
        return v;
    }
    bool alpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
    if (!alpha) {
        if (!((c >= '0' && c <= '9') || c == '.')) {
            Error = true;                                 // "Syntax error"
            return 0.0f;
        }
        char* end;
        double d = strtod(P, &end);
        P = end;
        return (float)d;
    }
    std::string name;
    for (;;) {
        char k = *P;
        if (!((k >= 'A' && k <= 'Z') || (k >= 'a' && k <= 'z') || (k >= '0' && k <= '9') || k == '_'))
            break;
        ++P;
        name += k;
    }
    return Vars ? Vars->GetFloat("Values", name.c_str(), 0.0f) : 0.0f;    // 0x660490
}

// PANZERS 0x6769b0
float SExprParser::Term()
{
    float v = Factor();
    for (;;) {
        if (Error)
            return v;
        Skip();
        if (*P == '*') {
            ++P;
            v = Factor() * v;
        } else if (*P == '/') {
            ++P;
            v = v / Factor();
        } else {
            return v;
        }
    }
}

// PANZERS 0x676950
float SExprParser::Expr()
{
    float v = Term();
    for (;;) {
        if (Error)
            return v;
        Skip();
        if (*P == '+') {
            ++P;
            v = Term() + v;
        } else if (*P == '-') {
            ++P;
            v = v - Term();
        } else {
            return v;
        }
    }
}
} // namespace

// PANZERS 0x676870
bool EvalUnitExpression(SProperties* vars, const char* text, float* out)
{
    SExprParser ps = { vars, text ? text : "", false };
    float v = ps.Expr();
    if (ps.Error || *ps.P != '\0')
        return false;                                     // "Syntax error" (caught by 0x669220)
    *out = v;
    return true;
}

// ===========================================================================

static SUPPropStruct* s_UnitSchema = nullptr;

// HD: the static initializer FUN_004c8680 builds it at process start.
SUPPropStruct* GetUnitSchema()
{
    if (!s_UnitSchema)
        s_UnitSchema = static_cast<SUPPropStruct*>(BuildUnitSchema());
    return s_UnitSchema;
}

// HD 0x5d0e70 / 0x5d1050: SProperty instance of 0x929a44 (0x664960), then
// vtbl +0x0c Load(props, "Unit", 0).
SUPropStruct* LoadUnitProperties(SProperties* file)
{
    SUPropStruct* root = static_cast<SUPropStruct*>(GetUnitSchema()->CreateInstance());
    root->Load(file, "Unit", nullptr);
    return root;
}

} // namespace pz
