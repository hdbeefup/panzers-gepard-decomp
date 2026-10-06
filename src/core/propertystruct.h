// core/propertystruct.h
// Panzers typed property trees (HD 0x661c70..0x669d60), used by the effect
// files (.fx, "[ExtendedFx2]") and the editors' unit/effect descriptions.
//
// Two class families, as in HD:
// - SPProperty*  the schema ("prototype") tree: type, name, tooltip, range
//                and default. Built once in code (SPixie ctor 0x6941e0
//                builds the effect schema) and never changed.
// - SProperty*   an instance tree created from a schema node
//                (SPProperty::CreateInstance 0x664960), filled from an
//                SProperties ini section by Load().
//
// Keys are the dotted path of the node names. Array elements are named
// "<index>.<element name>", and a Multi stores the selected index under its
// own key and loads every alternative. Example from a menu effect:
//   Effects = 1
//   Effects.0.Data.EffectType = 0
//   Effects.0.Data.EffectType.00_PARTICLES.Birth.LifeTime = 5.000000
//
// Recompile notes: HD keeps names in SString and children in SDArray; this
// port uses std::string / std::vector (the objects are internal to the
// loader, nothing outside reads their bytes). The SProperty vtable slot +0x08
// (Save, 0x668xxx, used only by the editors) is not ported.

#ifndef CORE_PROPERTYSTRUCT_H
#define CORE_PROPERTYSTRUCT_H

#include <string>
#include <vector>
#include <initializer_list>

struct SProperties;

// HD SPProperty+0x04 / SProperty+0x04->+0x04
enum EPropertyType {
    PROPERTY_TYPE_INT    = 1,
    PROPERTY_TYPE_COLOR  = 2,
    PROPERTY_TYPE_FLOAT  = 3,
    PROPERTY_TYPE_BOOL   = 4,
    PROPERTY_TYPE_STRING = 5,
    PROPERTY_TYPE_ENUM   = 6,
    PROPERTY_TYPE_STRUCT = 7,
    PROPERTY_TYPE_MULTI  = 8,
    PROPERTY_TYPE_ARRAY  = 9,
    PROPERTY_TYPE_TRACK  = 10,
    PROPERTY_TYPE_EXPR   = 11,
};

struct SProperty;

// ---------------------------------------------------------------------------
// Schema (HD SPProperty, vftable 0x80bc1c family; 1 virtual: the dtor)
// ---------------------------------------------------------------------------
struct SPProperty {
    virtual ~SPProperty() {}

    int         Type;      // +0x04 EPropertyType
    std::string Name;      // +0x08 SString
    std::string Desc;      // +0x10 SString (the editor tooltip, "(Tooltip!)" in the effect schema)
    bool        Flag;      // +0x18 (struct "expanded in the editor"; 0 elsewhere)

    SProperty* CreateInstance();   // 0x664960

protected:
    SPProperty(int type, const char* name, const char* desc);   // 0x661d60
};

struct SPPropertyInt : SPProperty {     // vftable 0x80bc30
    int Min, Max, Default;                // +0x1c +0x20 +0x24
    SPPropertyInt(const char* name, const char* desc, int min, int max, int def);   // 0x662430
};

struct SPPropertyColor : SPProperty {   // vftable 0x80bc4c
    unsigned Default;                     // +0x1c 0xRRGGBB
    SPPropertyColor(const char* name, const char* desc, unsigned def);              // 0x661f90
};

struct SPPropertyFloat : SPProperty {   // vftable 0x80bc68
    float Min, Max, Default;              // +0x1c +0x20 +0x24
    SPPropertyFloat(const char* name, const char* desc, float min, float max, float def); // 0x662330
};

struct SPPropertyBool : SPProperty {    // vftable 0x80bca0
    bool Default;                         // +0x1c
    SPPropertyBool(const char* name, const char* desc, bool def);                   // 0x661ef0
};

struct SPPropertyString : SPProperty {  // vftable 0x80bcbc
    std::string Default;                  // +0x1c SString
    SPPropertyString(const char* name, const char* desc, const char* def);          // 0x6626c0
};

struct SPPropertyEnum : SPProperty {    // vftable 0x80bcd8
    struct SItem { std::string Name; int Value; };
    std::vector<SItem> Items;             // +0x1c SDArray<{SString*, int}>
    int Default;                          // +0x28
    // HD is variadic: (name, value) pairs ended by a null name.
    SPPropertyEnum(const char* name, const char* desc, int def,
                   std::initializer_list<SItem> items);                             // 0x662050
};

struct SPPropertyStruct : SPProperty {  // vftable 0x80bcf4
    std::vector<SPProperty*> Children;    // +0x1c SDArray
    // HD is variadic: children ended by a null pointer.
    SPPropertyStruct(const char* name, const char* desc, bool flag,
                     std::initializer_list<SPProperty*> children);                  // 0x662790
    ~SPPropertyStruct();
};

struct SPPropertyMulti : SPProperty {   // vftable 0x80bd10
    std::vector<SPProperty*> Children;    // +0x1c SDArray, all PROPERTY_TYPE_STRUCT
    int Default;                          // +0x28 selected alternative
    SPPropertyMulti(const char* name, const char* desc, int def,
                    std::initializer_list<SPProperty*> children);                   // 0x662520
    ~SPPropertyMulti();
};

struct SPPropertyArray : SPProperty {   // vftable 0x7fe144
    int Min, Max, Default;                // +0x1c +0x20 +0x24 element count
    SPProperty* Element;                  // +0x28
    SPPropertyArray(const char* name, const char* desc, int min, int max, int def,
                    SPProperty* element);                                           // 0x5cfc10
    ~SPPropertyArray();
};

struct SPPropertyTrack : SPProperty {   // vftable 0x8797dc
    SPProperty* Options;                  // +0x1c SPPropertyStruct "Options" {Loop, Max. Y}
    SPProperty* Keys;                     // +0x20 SPPropertyArray "Keys" of Struct "Key" {X, Y}
    SPPropertyTrack(const char* name, const char* desc, SPProperty* options, SPProperty* keys); // 0x694170
    ~SPPropertyTrack();
};

// ---------------------------------------------------------------------------
// Piecewise-linear float track (keys {x, y, slope}, +0x0c loop).
// AddKey 0x6648a0, Evaluate 0x6abda0. Filled by SPropertyStruct::GetTrackFloat.
// ---------------------------------------------------------------------------
struct STrackFloat {
    struct SKey { float X, Y, Slope; };
    std::vector<SKey> Keys;               // +0x00 SDArray (stride 0xc)
    bool Loop = false;                    // +0x0c

    void AddKey(float x, float y);                      // 0x6648a0
    float Evaluate(float t, int* cursor) const;         // 0x6abda0
    int Count() const { return (int)Keys.size(); }
};

// ---------------------------------------------------------------------------
// Instances (HD SProperty, vftable 0x80bc1c: dtor, SetName, Save, Load)
// ---------------------------------------------------------------------------
struct SProperty {
    virtual ~SProperty() {}
    virtual void SetName(const char* name);                                          // +0x04 0x6695a0
    // +0x08 Save (editors only) is not ported.
    virtual bool Load(SProperties* props, const char* section, const char* prefix) = 0; // +0x0c

    SPProperty* Proto;    // +0x04
    std::string Name;     // +0x08
    bool Selected;        // +0x10 (Multi: the active alternative)

    int Type() const { return Proto->Type; }

protected:
    explicit SProperty(SPProperty* proto);
    // HD: key = prefix ? prefix + "." + Name : Name (0x52c580/0x52c4a0).
    std::string KeyOf(const char* prefix) const;
};

struct SPropertyInt : SProperty {
    int Value;                                                                       // +0x18
    SPropertyInt(SPPropertyInt* proto, int value);                                   // 0x6631b0
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x667420
};

struct SPropertyColor : SProperty {
    unsigned Value;
    SPropertyColor(SPPropertyColor* proto, unsigned value);                          // 0x664c30
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x666db0
};

struct SPropertyFloat : SProperty {
    float Value;
    SPropertyFloat(SPPropertyFloat* proto, float value);                             // 0x664f70
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x667290
};

struct SPropertyBool : SProperty {
    bool Value;
    SPropertyBool(SPPropertyBool* proto, bool value);                                // 0x664960 case 4
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x666c40
};

struct SPropertyString : SProperty {
    std::string Value;
    SPropertyString(SPPropertyString* proto, const char* value);                     // 0x665220
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x667750
};

struct SPropertyEnum : SProperty {
    int Value;
    SPropertyEnum(SPPropertyEnum* proto, int value);                                 // 0x664ca0
    int SetValue(int value);                                                         // 0x669aa0
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x666f20
};

struct SPropertyStruct : SProperty {
    std::vector<SProperty*> Children;                                                // +0x18
    explicit SPropertyStruct(SPPropertyStruct* proto);                               // 0x663510
    ~SPropertyStruct();
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x667920

    // Typed getters. The by-name forms return the first child of that type
    // and name (_stricmp); the (index, name) forms check the child at index.
    // A missing child panics "SPropertyStruct::GetX(): Invalid property-tree."
    int GetInt(const char* name);                                                    // 0x665e50
    bool GetBool(const char* name);                                                  // 0x665610
    bool GetBool(int index, const char* name);                                       // 0x665560
    unsigned GetColor(const char* name);                                             // 0x665780
    int GetEnum(const char* name);                                                   // 0x665950
    float GetFloat(const char* name);                                                // 0x665c00
    float GetFloat(int index, const char* name);                                     // 0x665b50
    int GetMultiIndex(const char* name);                                             // 0x665fc0
    SPropertyStruct* GetMultiSubStruct(const char* name);                            // 0x666190
    SPropertyStruct* GetMultiSubStruct(int index, const char* name);                 // 0x666080
    const char* GetString(const char* name);                                         // 0x666360
    const char* GetString(int index, const char* name);                              // 0x6662a0
    SPropertyStruct* GetStruct(int index, const char* name);                         // 0x666430
    struct SPropertyTrack* GetTrack(const char* name);                               // 0x666640
    void GetTrackFloat(const char* name, STrackFloat* out);                          // 0x667cf0
    int GetInt(int index, const char* name);                                         // 0x665da0
    unsigned GetColor(int index, const char* name);                                  // 0x6656d0
    int GetEnum(int index, const char* name);                                        // 0x6658a0
    int GetArraySize(const char* name);                                              // 0x6654a0
    SProperty* GetArrayItem(const char* name, int index);                            // 0x665370

private:
    SProperty* Find(int type, const char* name, const char* who);
    SProperty* At(int index, int type, const char* name, const char* who);
};

struct SPropertyMulti : SProperty {
    std::vector<SProperty*> Children;                                                // +0x14
    int Index;                                                                       // +0x24
    SPropertyMulti(SPPropertyMulti* proto, int index);                               // 0x663250
    ~SPropertyMulti();
    int SetIndex(int index);                                                         // 0x669450
    SPropertyStruct* Current();
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x667590
};

struct SPropertyArray : SProperty {
    std::vector<SProperty*> Children;                                                // +0x18
    SPropertyArray(SPPropertyArray* proto, int count);                               // 0x662980
    ~SPropertyArray();
    void Clear();
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x6666f0
};

struct SPropertyTrack : SProperty {
    SPropertyStruct* Options;                                                        // +0x18
    SPropertyArray*  Keys;                                                           // +0x1c
    explicit SPropertyTrack(SPPropertyTrack* proto);                                 // 0x6636b0
    ~SPropertyTrack();
    bool Load(SProperties* props, const char* section, const char* prefix) override; // 0x667ac0
};

#endif // CORE_PROPERTYSTRUCT_H
