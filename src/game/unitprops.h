// src/game/unitprops.h
// The typed property trees of the .unit files (HD SPProperty / SProperty,
// 0x661c70..0x669d60), as the unit prototypes read them. OWNER: agent U.
//
// Why a unit-side copy: the .unit schema (FUN_004c8680, unitschema.cpp) uses
// the Expr type (HD SPPropertyExpr 0x6621e0 / SPropertyExpr vftable
// 0x80bc8c: a float written as an expression over the [Values] of
// unitvariables.ini, e.g. "Sight = Vehicle_Sight_Tank"), which core's
// propertystruct.cpp does not port, and core's CreateInstance cannot be
// extended from outside. The classes below follow the same HD functions as
// core (load keys, array element names, multi index) plus Expr, and expose the
// HD getters the unit code calls (GetStruct 0x6664e0, GetArraySize 0x6654a0,
// GetArrayItem 0x665370, ...).
//
// For the other agents: SP*Driver and SP*UnitAnimation prototypes are loaded
// from sub-trees of the same instance (Unit.Drivers.N.Driver, Unit.Animation)
// and get an SUPropStruct* (see punit.h, the prototype factories).
//
// Keys: the dotted path of the node names, as in core. Array elements are
// named "<index>.<element name>"; a Multi stores its selected index under its
// own key and loads every alternative.

#ifndef PZ_UNITPROPS_H
#define PZ_UNITPROPS_H

#include <string>
#include <vector>
#include <initializer_list>

struct SProperties;

namespace pz {

// HD SPProperty +0x04 (the same numbering as core's EPropertyType).
enum EUnitPropType {
    UPROP_INT    = 1,
    UPROP_COLOR  = 2,
    UPROP_FLOAT  = 3,
    UPROP_BOOL   = 4,
    UPROP_STRING = 5,
    UPROP_ENUM   = 6,
    UPROP_STRUCT = 7,
    UPROP_MULTI  = 8,
    UPROP_ARRAY  = 9,
    UPROP_TRACK  = 10,
    UPROP_EXPR   = 11,
};

struct SUProp;
struct SUPropStruct;

// ---------------------------------------------------------------------------
// Schema (HD SPProperty family)

struct SUPProp {
    virtual ~SUPProp() {}
    int         Type;      // +0x04
    std::string Name;      // +0x08
    std::string Desc;      // +0x10
    bool        Flag;      // +0x18
    SUProp* CreateInstance();                                    // 0x664960
protected:
    SUPProp(int type, const char* name, const char* desc);       // 0x661d60
};

struct SUPPropInt : SUPProp {
    int Min, Max, Default;
    SUPPropInt(const char* name, const char* desc, int min, int max, int def);      // 0x662430
};

struct SUPPropFloat : SUPProp {
    float Min, Max, Default;
    SUPPropFloat(const char* name, const char* desc, float min, float max, float def); // 0x662330
};

struct SUPPropBool : SUPProp {
    bool Default;
    SUPPropBool(const char* name, const char* desc, bool def);                     // 0x661ef0
};

struct SUPPropString : SUPProp {
    std::string Default;
    SUPPropString(const char* name, const char* desc, const char* def);            // 0x6626c0
};

struct SUPPropEnum : SUPProp {
    struct SItem { const char* Name; int Value; };
    std::vector<SItem> Items;
    int Default;
    SUPPropEnum(const char* name, const char* desc, int def, std::initializer_list<SItem> items); // 0x662050
};

struct SUPPropStruct : SUPProp {
    std::vector<SUPProp*> Children;
    SUPPropStruct(const char* name, const char* desc, bool flag, std::initializer_list<SUPProp*> children); // 0x662790
    ~SUPPropStruct();
};

struct SUPPropMulti : SUPProp {
    std::vector<SUPProp*> Children;
    int Default;
    SUPPropMulti(const char* name, const char* desc, int def, std::initializer_list<SUPProp*> children); // 0x662520
    ~SUPPropMulti();
};

struct SUPPropArray : SUPProp {
    int Min, Max, Default;
    SUPProp* Element;
    SUPPropArray(const char* name, const char* desc, int min, int max, int def, SUPProp* element); // 0x5cfc10
    ~SUPPropArray();
};

// HD SPPropertyExpr (0x30 bytes): +0x1c the SProperties the identifiers are
// read from, +0x20/+0x24 range, +0x28 default expression text.
struct SUPPropExpr : SUPProp {
    float Min, Max;
    std::string Default;
    SUPPropExpr(const char* name, const char* desc, float min, float max, const char* def); // 0x6621e0
};

// ---------------------------------------------------------------------------
// Instances (HD SProperty family: dtor, SetName, Save, Load)

struct SUProp {
    virtual ~SUProp() {}
    virtual void Load(SProperties* props, const char* section, const char* prefix) = 0; // +0x0c
    SUPProp*    Proto;     // +0x04
    std::string Name;      // +0x08
    bool        Selected;  // +0x10
    int Type() const { return Proto->Type; }
    void SetName(const char* name) { Name = name ? name : ""; }      // 0x6695a0
protected:
    explicit SUProp(SUPProp* proto);
    std::string KeyOf(const char* prefix) const;                     // 0x52c580 / 0x52c4a0
};

struct SUPropInt : SUProp {
    int Value;
    SUPropInt(SUPPropInt* proto, int value);                                        // 0x6631b0
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x667420
};

struct SUPropFloat : SUProp {
    float Value;
    SUPropFloat(SUPPropFloat* proto, float value);                                  // 0x664f70
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x667290
};

struct SUPropBool : SUProp {
    bool Value;
    SUPropBool(SUPPropBool* proto, bool value);                                     // 0x664960 case 4
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x666c40
};

struct SUPropString : SUProp {
    std::string Value;
    SUPropString(SUPPropString* proto, const char* value);                          // 0x665220
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x667750
};

struct SUPropEnum : SUProp {
    int Value;
    SUPropEnum(SUPPropEnum* proto, int value);                                      // 0x664ca0
    int SetValue(int value);                                                        // 0x669aa0
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x666f20
};

// HD SPropertyExpr (0x24 bytes): +0x18 expression text, +0x20 value.
struct SUPropExpr : SUProp {
    std::string Expr;
    float Value;
    explicit SUPropExpr(SUPPropExpr* proto);                                        // 0x662dd0
    void SetExpression(const char* text);                                           // 0x669220
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x667080
};

struct SUPropStruct : SUProp {
    std::vector<SUProp*> Children;                                                  // +0x18
    // Recompile: the file and full key of the last Load, so loaders that read
    // the flat .unit keys (agent A's SAnimProps::FromFlat) find this subtree.
    SProperties*         Source = nullptr;
    std::string          Key;
    explicit SUPropStruct(SUPPropStruct* proto);                                    // 0x663510
    ~SUPropStruct();
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x667920

    // HD getters: the first child of that type and name (_stricmp); a
    // missing child panics "SPropertyStruct::GetX(): Invalid property-tree."
    int GetInt(const char* name);                                    // 0x665e50
    bool GetBool(const char* name);                                  // 0x665610
    int GetEnum(const char* name);                                   // 0x665950
    float GetFloat(const char* name);                                // 0x665c00 (Float or Expr)
    const char* GetString(const char* name);                         // 0x666360
    SUPropStruct* GetStruct(const char* name);                       // 0x6664e0
    int GetMultiIndex(const char* name);                             // 0x665fc0
    SUPropStruct* GetMultiSubStruct(const char* name);               // 0x666190
    int GetArraySize(const char* name);                              // 0x6654a0
    SUPropStruct* GetArrayItem(const char* name, int index);         // 0x665370

private:
    SUProp* Find(int type, const char* name, const char* who);
};

struct SUPropMulti : SUProp {
    std::vector<SUProp*> Children;                                                  // +0x14
    int Index;                                                                      // +0x24
    SUPropMulti(SUPPropMulti* proto, int index);                                    // 0x663250
    ~SUPropMulti();
    int SetIndex(int index);                                                        // 0x669450
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x667590
};

struct SUPropArray : SUProp {
    std::vector<SUProp*> Children;                                                  // +0x18
    SUPropArray(SUPPropArray* proto, int count);                                    // 0x662980
    ~SUPropArray();
    void Clear();
    void Load(SProperties* props, const char* section, const char* prefix) override; // 0x6666f0
};

// HD 0x676870: the expression evaluator of SPropertyExpr (sum of products of
// numbers, parenthesised sub-expressions and identifiers; an identifier is
// GetFloat("Values", name, 0) of `vars`). Returns false on a syntax error.
bool EvalUnitExpression(SProperties* vars, const char* text, float* out);

// The .unit schema root (HD 0x929a44, built by FUN_004c8680 in
// unitschema.cpp) and the unitvariables.ini properties its Expr nodes read
// (HD 0x929a28). The registry sets the variables before it loads the units.
SUPProp* BuildUnitSchema();
SUPPropStruct* GetUnitSchema();
extern SProperties* g_UnitVariables;

// Creates and loads the instance tree of one .unit file ([Unit] section).
// The caller deletes it.
SUPropStruct* LoadUnitProperties(SProperties* file);

} // namespace pz

#endif // PZ_UNITPROPS_H
