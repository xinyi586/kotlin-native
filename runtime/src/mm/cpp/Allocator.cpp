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

mm::HeapObject& mm::Allocator::GetHeapObject(Block& block) noexcept {
    return *GetHeapObjectPlace(&block);
}

mm::Allocator::Allocator() noexcept = default;
mm::Allocator::~Allocator() = default;

mm::HeapObject* mm::Allocator::GetHeapObjectPlace(mm::Allocator::Block* blockPlace) noexcept {
    return reinterpret_cast<HeapObject*>(blockPlace + 1);
}

void mm::Allocator::Erase(Block* previousBlock, Block& block) noexcept {
    HeapObject::Destroy(GetHeapObjectPlace(&block));
    previousBlock->next_ = block.next_;
    block.~Block();
}
