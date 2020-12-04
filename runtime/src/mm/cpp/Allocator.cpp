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

// This is a weird class that doesn't know it's size at compile-time.
class mm::Allocator::Block final : private Pinned {
public:
    // Allocate and initialize block.
    template <typename... Args>
    static Block& Create(Block* nextBlock, Args&&... args) noexcept {
        auto allocationSize = sizeof(Block) + HeapObject::Sizeof(std::forward<Args>(args)...);
        auto* block = new (konan::calloc(1, AlignUp(allocationSize, kObjectAlignment))) Block(std::forward<Args>(args)...);
        return *block;
    }

    // Deinitialize and destroy `this`.
    void Destroy(Block* previousBlock) noexcept {
        if (previousBlock != nullptr) {
            previousBlock->next_ = next_;
        }
        this->~Block();
        konan::free(this);
    }

    HeapObject& GetHeapObject() noexcept { return *GetHeapObjectPlace(); }

    Block* Next() const noexcept { return next_; }

private:
    // Hide all the ways of constructing this class.
    template <typename... Args>
    explicit Block(Args&&... args) noexcept {
        HeapObject::Create(GetHeapObjectPlace(), std::forward<Args>(args)...);
    }

    ~Block() { HeapObject::Destroy(GetHeapObjectPlace()); }

    HeapObject* GetHeapObjectPlace() noexcept { return reinterpret_cast<HeapObject*>(this + 1); }

    Block* next_ = nullptr;
};

// static
mm::Allocator& mm::Allocator::Instance() noexcept {
    return mm::GlobalData::Instance().allocator();
}

ObjHeader* mm::Allocator::AllocateObject(ThreadData* threadData, const TypeInfo* typeInfo) noexcept {
    auto& block = Block::Create(root_, typeInfo);
    root_ = &block;
    return block.GetHeapObject().GetObjHeader();
}

ArrayHeader* mm::Allocator::AllocateArray(ThreadData* threadData, const TypeInfo* typeInfo, uint32_t count) noexcept {
    auto& block = Block::Create(root_, typeInfo, count);
    root_ = &block;
    return block.GetHeapObject().GetArrayHeader();
}

mm::HeapObject& mm::Allocator::GetHeapObject(Block& block) noexcept {
    return block.GetHeapObject();
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
    block.Destroy(previousBlock);
}
