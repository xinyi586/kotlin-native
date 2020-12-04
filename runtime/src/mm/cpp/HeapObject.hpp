/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_HEAP_OBJECT_H
#define RUNTIME_MM_HEAP_OBJECT_H

#include "Memory.h"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

// MM-specific object info. Common between object and array.
// This is a weird class that doesn't know its size at compile-time.
class HeapObject : private Pinned {
public:
    static size_t Sizeof(const TypeInfo* typeInfo) noexcept;
    static size_t Sizeof(const TypeInfo* typeInfo, uint32_t count) noexcept;

    static HeapObject& Create(HeapObject* location, const TypeInfo* typeInfo) noexcept;
    static HeapObject& Create(HeapObject* location, const TypeInfo* typeInfo, uint32_t count) noexcept;

    static void Destroy(HeapObject* location) noexcept;

    ObjHeader* GetObjHeader() noexcept;
    ArrayHeader* GetArrayHeader() noexcept;

private:
    // Hide all the ways of constructing this class on the stack.
    explicit HeapObject(const TypeInfo* typeInfo) noexcept;
    HeapObject(const TypeInfo* typeInfo, uint32_t count) noexcept;

    ~HeapObject();
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_OBJECT_INFO_H
