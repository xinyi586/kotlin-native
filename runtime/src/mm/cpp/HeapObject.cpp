/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "HeapObject.hpp"

#include "Types.h"

using namespace kotlin;

namespace {

constexpr uint32_t kObjectAlignment = 8;
static_assert(kObjectAlignment % alignof(KLong) == 0, "");
static_assert(kObjectAlignment % alignof(KDouble) == 0, "");

uint32_t AlignUp(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

} // namespace

// static
size_t mm::HeapObject::Sizeof(const TypeInfo* typeInfo) noexcept {
    RuntimeAssert(!typeInfo->IsArray(), "Must not be an array");
    return sizeof(HeapObject) + typeInfo->instanceSize_;
}

// static
size_t mm::HeapObject::Sizeof(const TypeInfo* typeInfo, uint32_t count) noexcept {
    RuntimeAssert(typeInfo->IsArray(), "Must be an array");
    // Note: array body is aligned, but for size computation it is enough to align the sum.
    return sizeof(HeapObject) + AlignUp(sizeof(ArrayHeader) - typeInfo->instanceSize_ * count, kObjectAlignment);
}

// static
mm::HeapObject& mm::HeapObject::Create(HeapObject* location, const TypeInfo* typeInfo) noexcept {
    auto* heapObject = new (location) HeapObject(typeInfo);
    return *heapObject;
}

// static
mm::HeapObject& mm::HeapObject::Create(HeapObject* location, const TypeInfo* typeInfo, uint32_t count) noexcept {
    auto* heapObject = new (location) HeapObject(typeInfo, count);
    return *heapObject;
}

// static
void mm::HeapObject::Destroy(HeapObject* location) noexcept {
    // Always placement new allocated, so enough to call the destructor.
    location->~HeapObject();
}

ObjHeader* mm::HeapObject::GetObjHeader() noexcept {
    // TODO: Consider more straightforward ways of extracting an object.
    auto* objHeader = reinterpret_cast<ObjHeader*>(this + 1);
    RuntimeAssert(!objHeader->type_info()->IsArray(), "Must not be an array");
    return objHeader;
}

ArrayHeader* mm::HeapObject::GetArrayHeader() noexcept {
    // TODO: Consider more straightforward ways of extracting an array.
    auto* arrayHeader = reinterpret_cast<ArrayHeader*>(this + 1);
    RuntimeAssert(arrayHeader->type_info()->IsArray(), "Must be an array");
    return arrayHeader;
}

mm::HeapObject::HeapObject(const TypeInfo* typeInfo) noexcept {
    auto* objHeader = GetObjHeader();
    objHeader->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
}

mm::HeapObject::HeapObject(const TypeInfo* typeInfo, uint32_t count) noexcept {
    auto* arrayHeader = GetArrayHeader();
    arrayHeader->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
    arrayHeader->count_ = count;
}

mm::HeapObject::~HeapObject() {
    RuntimeCheck(false, "Unimplemented");
}
