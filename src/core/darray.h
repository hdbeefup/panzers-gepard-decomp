// core/darray.h
// Dynamic array template (SDArray<T>) and stack (SStack<T>)
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_DARRAY_H
#define CORE_DARRAY_H

#include "core_common.h"

template<typename T>
struct SDArray {
    int size;
    int maxsize;
    T* array;

    int Add() {
        if (size == maxsize) {
            int newmax = (maxsize >= 16) ? (6 * maxsize / 5) : 16;
            T* newarray = (T*)realloc(array, newmax * sizeof(T));
            int oldmax = maxsize;
            array = newarray;
            memset(&newarray[oldmax], 0, sizeof(T) * (newmax - oldmax));
            maxsize = newmax;
        }
        return size++;
    }

    void Add(const T* item) {
        int idx = Add();
        array[idx] = *item;
    }

    void Remove(int index) {
        if (index < 0 || index >= size)
            return;
        size--;
        for (int i = index; i < size; i++)
            array[i] = array[i + 1];
        memset(&array[size], 0, sizeof(T));
    }

    void Clear() {
        if (size && !array)
            return; // damaged
        int oldmax = maxsize;
        size = 0;
        if (oldmax < 0) {
            maxsize = 0;
            array = (T*)realloc(array, 0);
            oldmax = maxsize;
        }
        memset(array, 0, sizeof(T) * oldmax);
    }

    void Load(struct SStream *is);

    void Clear(int newsize) {
        if (newsize > maxsize) {
            array = (T*)realloc(array, newsize * sizeof(T));
            memset(&array[maxsize], 0, sizeof(T) * (newsize - maxsize));
            maxsize = newsize;
        }
        size = newsize;
    }
};

template<typename T>
struct SStack {
    T* array;
    int size;
    int maxsize;
    float userdata;

    void Push(T value) {
        if (size == maxsize) {
            int newmax = (maxsize >= 16) ? (6 * maxsize / 5) : 16;
            array = (T*)realloc(array, newmax * sizeof(T));
            maxsize = newmax;
        }
        array[size++] = value;
    }

    T Peek() {
        return array[size - 1];
    }

    T Pop() {
        return array[--size];
    }
};

template<typename T>
using DArray = SDArray<T>;

#endif // CORE_DARRAY_H
