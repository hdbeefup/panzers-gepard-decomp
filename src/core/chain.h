// core/chain.h
// SChain<T> — Intrusive doubly-linked list
// SHeap<T> — Object pool with free-list
// Array<T> — Simple dynamic array (different from SDArray)
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_CHAIN_H
#define CORE_CHAIN_H

#include "core_common.h"

// SChain<T> — intrusive linked list
// T must have 'next' and 'prev' pointers
// PDB layout: { T* first, T* last, T* current, bool Closed, int NumItems }
// Size: 20 bytes (with padding)
template<typename T>
struct SChain {
    T* first;
    T* last;
    T* current;
    bool Closed;
    int NumItems;

    SChain() : first(0), last(0), current(0), Closed(false), NumItems(0) {}

    T* Add(T* item);
    void Remove(T* item);
    void DeleteAll() {
        T* p = first;
        while (p) {
            T* n = p->next;
            delete p;
            p = n;
        }
        first = 0;
        last = 0;
        current = 0;
        NumItems = 0;
    }
    T* GetFirst() { return first; }
    T* GetNext(T* item) { return item ? item->next : 0; }
    T* GetNext2() {
        if (!current) return 0;
        T* n = current->next;
        if (!n) { if (Closed) n = first; else return 0; }
        T* n2 = n->next;
        if (!n2 && Closed) n2 = first;
        return n2;
    }
    T* GetLastButOne() { return last ? last->prev : 0; }
    T* GetPointer(int index) {
        T* p = first;
        for (int i = 0; i < index && p; i++) p = p->next;
        return p;
    }
    T* StepToNext() {
        if (current) current = current->next;
        if (!current && Closed) current = first;
        return current;
    }
};

// SHeap<T> — pool allocator with free list
// PDB layout: { __Tstruct* array, int size, int maxsize, int nextempty, int occupied }
// __Tstruct wraps T with { int use; T data; }
// Size: 20 bytes
template<typename T>
struct SHeap {
    struct Element {
        int use;
        T data;
    };
    typedef Element __Tstruct;
    Element* array;
    int size;
    int maxsize;
    int nextempty;
    int occupied;

    int Add() {
        ++occupied;
        int ne = nextempty;
        if (ne < 0) {
            int sz = size;
            int mx = maxsize;
            if (sz == mx) {
                int newmax = (mx >= 16) ? (6 * mx / 5) : 16;
                array = (Element*)realloc(array, sizeof(Element) * newmax);
                memset(&array[mx], 0, sizeof(Element) * (newmax - mx));
                sz = size;
                maxsize = newmax;
            }
            array[sz].use = 0x7FFFFFFF;
            int result = size;
            size = result + 1;
            return result;
        } else {
            nextempty = array[ne].use;
            array[ne].use = 0x7FFFFFFF;
            memset(&array[ne].data, 0, sizeof(T));
            return ne;
        }
    }
    void Remove(int index) {
        if (index < 0 || index >= size || array[index].use != 0x7FFFFFFF) return;
        array[index].use = nextempty;
        --occupied;
        nextempty = index;
    }
    void RemoveAll();
    void Clear();
    void Load(struct SStream *is);
    int GetOccupied() { return occupied; }
    int GetNext(int idx) {
        for (int i = idx + 1; i < size; ++i) {
            if (array[i].use == 0x7FFFFFFF)
                return i;
        }
        return -1;
    }
};

// SFifo<T, N> — fixed-size ring buffer
template<typename T, int N>
struct SFifo {
    T array[N];
    int pos;
    int quantity;
};

// Array<T> — simple dynamic array (not SDArray)
// PDB layout: { T* array, unsigned int count, unsigned int capacity }
// Size: 12 bytes
template<typename T>
struct Array {
    T* array;
    unsigned int count;
    unsigned int capacity;

    void Add(const T& item) {
        if (count >= capacity) {
            capacity = capacity ? capacity * 2 : 8;
            array = (T*)realloc(array, capacity * sizeof(T));
        }
        array[count++] = item;
    }
    void Add(const T* item) { Add(*item); }
    void SetCount(unsigned int n) { count = n; }
};

#endif // CORE_CHAIN_H
