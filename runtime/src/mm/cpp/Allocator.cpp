/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Allocator.hpp"

#include "GlobalData.hpp"

using namespace kotlin;

// static
mm::Allocator& mm::Allocator::Instance() noexcept {
    return mm::GlobalData::Instance().allocator();
}

ObjHeader* mm::Allocator::AllocateObject(ThreadData* threadData, const TypeInfo* typeInfo) noexcept {
    RuntimeCheck(false, "Unimplemented");
}

ArrayHeader* mm::Allocator::AllocateArray(ThreadData* threadData, const TypeInfo* typeInfo, int32_t count) noexcept {
    RuntimeCheck(false, "Unimplemented");
}

mm::Allocator::Block& mm::Allocator::GetBlock(ObjectInfo& objectInfo) noexcept {
    RuntimeCheck(false, "Unimplemented");
}

mm::ObjectInfo& mm::Allocator::GetObjectInfo(Block& block) noexcept {
    RuntimeCheck(false, "Unimplemented");
}

mm::Allocator::Allocator() noexcept = default;
mm::Allocator::~Allocator() = default;

mm::Allocator::Block* mm::Allocator::FirstBlock() noexcept {
    RuntimeCheck(false, "Unimplemented");
}

// static
mm::Allocator::Block* mm::Allocator::NextBlock(Block& block) noexcept {
    RuntimeCheck(false, "Unimplemented");
}

void mm::Allocator::Erase(Block& block) noexcept {
    RuntimeCheck(false, "Unimplemented");
}
