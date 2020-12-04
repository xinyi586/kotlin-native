/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Allocator.hpp"

#include "Alignment.hpp"
#include "GlobalData.hpp"
#include "HeapObject.hpp"
#include "Porting.h"

using namespace kotlin;

class mm::Allocator::Block final : private Pinned {
public:
    explicit Block(Block* next) noexcept : next_(next) {}
    ~Block() = default;

private:
    friend class Allocator;

    Block* next_ = nullptr;
};

namespace {

HeapObject* GetHeapObjectPlace(mm::Allocator::Block* blockPlace) noexcept {
    return reinterpret_cast<HeapObject*>(blockPlace + 1);
}

}

// static
mm::Allocator& mm::Allocator::Instance() noexcept {
    return mm::GlobalData::Instance().allocator();
}

ObjHeader* mm::Allocator::AllocateObject(ThreadData* threadData, const TypeInfo* typeInfo) noexcept {
    auto allocationSize = sizeof(Block) + HeapObject::Sizeof(std::forward<Args>(args)...);
    void* location = konan::calloc(1, AlignUp(allocationSize, kObjectAlignment));
    auto* block = new (location) Block(root_);
    auto& heapObject = HeapObject::Create(GetHeapObjectPlace(block), std::forward<Args>(args)...);
    root_ = block;
    return heapObject.GetObjHeader();
}

ArrayHeader* mm::Allocator::AllocateArray(ThreadData* threadData, const TypeInfo* typeInfo, uint32_t count) noexcept {
    auto allocationSize = sizeof(Block) + HeapObject::Sizeof(std::forward<Args>(args)...);
    void* location = konan::calloc(1, AlignUp(allocationSize, kObjectAlignment))
    auto* block = new (location) Block(root_);
    auto& heapObject = HeapObject::Create(GetHeapObjectPlace(block), std::forward<Args>(args)...);
    root_ = block;
    return heapObject.GetArrayHeader();
}

mm::HeapObject& mm::Allocator::GetHeapObject(Block& block) noexcept {
    return *GetHeapObjectPlace(&block);
}

mm::Allocator::Allocator() noexcept = default;
mm::Allocator::~Allocator() = default;

mm::Allocator::Block* mm::Allocator::FirstBlock() noexcept {
    return root_;
}

// static
mm::Allocator::Block* mm::Allocator::NextBlock(Block& block) noexcept {
    return block.Next();
}

void mm::Allocator::Erase(Block* previousBlock, Block& block) noexcept {
    HeapObject::Destroy(GetHeapObjectPlace(&block));
    previousBlock->next_ = block.next_;
    block.~Block();
}
