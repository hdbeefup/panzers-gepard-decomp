// core/smartptr.h
// Smart pointer template
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_SMARTPTR_H
#define CORE_SMARTPTR_H

// SmartPtr<T> — COM-style reference-counted pointer
// PDB name: SmartPtr<IDirect3DTexture9>, etc.
// Field name is "Ptr" in the PDB (not "ptr")
template<typename T>
struct SmartPtr {
    T* Ptr;
    SmartPtr() : Ptr(nullptr) {}
    SmartPtr(T* p) : Ptr(p) { if (Ptr) Ptr->AddRef(); }
    ~SmartPtr() { if (Ptr) Ptr->Release(); }
    SmartPtr(const SmartPtr& other) : Ptr(other.Ptr) { if (Ptr) Ptr->AddRef(); }
    SmartPtr& operator=(const SmartPtr& other) {
        if (other.Ptr) other.Ptr->AddRef();
        if (Ptr) Ptr->Release();
        Ptr = other.Ptr;
        return *this;
    }
    T* operator->() const { return Ptr; }
    operator T*() const { return Ptr; }
};

// Keep old name as alias
template<typename T>
using SSmartPtr = SmartPtr<T>;

#endif // CORE_SMARTPTR_H
