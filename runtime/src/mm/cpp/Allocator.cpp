/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Allocator.hpp"

#include "Alignment.hpp"
#include "Alloc.h"
#include "GlobalData.hpp"
#include "Types.h"

using namespace kotlin;

namespace {

constexpr uint32_t kObjectAlignment = 8;
static_assert(kObjectAlignment % alignof(KLong) == 0, "");
static_assert(kObjectAlignment % alignof(KDouble) == 0, "");

} // namespace

ObjHeader* mm::Allocator::ThreadQueue::AllocateObject(const TypeInfo* typeInfo) noexcept {
    RuntimeAssert(!typeInfo->IsArray(), "Must not be an array");
    uint32_t allocSize = typeInfo->instanceSize_;
    auto& node = producer_.Insert(allocSize, kObjectAlignment);
    auto* object = static_cast<ObjHeader*>(node.Data());
    object->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
    return object;
}

ArrayHeader* mm::Allocator::ThreadQueue::AllocateArray(const TypeInfo* typeInfo, uint32_t count) noexcept {
    RuntimeAssert(typeInfo->IsArray(), "Must be an array");
    // Note: array body is aligned, but for size computation it is enough to align the sum.
    uint32_t allocSize = AlignUp(static_cast<uint32_t>(sizeof(ArrayHeader)) - typeInfo->instanceSize_ * count, kObjectAlignment);
    auto& node = producer_.Insert(allocSize, kObjectAlignment);
    auto* array = static_cast<ArrayHeader*>(node.Data());
    array->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
    array->count_ = count;
    return array;
}

// static
mm::Allocator& mm::Allocator::Instance() noexcept {
    return GlobalData::Instance().allocator();
}

mm::Allocator::Allocator() noexcept = default;
mm::Allocator::~Allocator() = default;
