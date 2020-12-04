/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Allocator.hpp"

#include "GlobalData.hpp"
#include "HeapObject.hpp"
#include "Porting.h"
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

// This is a weird class that doesn't know it's size at compile-time.
class mm::Allocator::Block final : private Pinned {
public:
    // Allocate and initialize block.
    static Block& CreateObject(Block* nextBlock, const TypeInfo* typeInfo) noexcept {
        RuntimeAssert(!typeInfo->IsArray(), "Must not be an array");
        // TODO: Consider more straightforward ways of allocating an object.
        auto allocationSize = sizeof(Block) + typeInfo->instanceSize_;
        auto* block = new (konan::calloc(1, AlignUp(allocationSize, kObjectAlignment))) Block(typeInfo);
        return *block;
    }

    static Block& CreateArray(Block* nextBlock, const TypeInfo* typeInfo, uint32_t count) noexcept {
        RuntimeAssert(typeInfo->IsArray(), "Must be an array");
        // TODO: Consider more straightforward ways of allocating an array.
        // Note: array body is aligned, but for size computation it is enough to align the sum.
        auto allocationSize = sizeof(Block) + AlignUp(sizeof(ArrayHeader) - typeInfo->instanceSize_ * count, kObjectAlignment);
        auto* block = new (konan::calloc(1, AlignUp(allocationSize, kObjectAlignment))) Block(typeInfo, count);
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

    ObjHeader* GetObjHeader() noexcept {
        // TODO: Consider more straightforward ways of extracting an object.
        auto* objHeader = reinterpret_cast<ObjHeader*>(this + 1);
        RuntimeAssert(!objHeader->type_info()->IsArray(), "Must not be an array");
        return objHeader;
    }

    ArrayHeader* GetArrayHeader() noexcept {
        // TODO: Consider more straightforward ways of extracting an array.
        auto* arrayHeader = reinterpret_cast<ArrayHeader*>(this + 1);
        RuntimeAssert(arrayHeader->type_info()->IsArray(), "Must be an array");
        return arrayHeader;
    }

    HeapObject& GetHeapObject() noexcept { return heapObject_; }

    Block* Next() const noexcept { return next_; }

private:
    // Hide all the ways of constructing this class.
    explicit Block(const TypeInfo* typeInfo) noexcept {
        auto* objHeader = GetObjHeader();
        objHeader->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
    }

    Block(const TypeInfo* typeInfo, uint32_t count) noexcept {
        auto* arrayHeader = GetArrayHeader();
        arrayHeader->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
        arrayHeader->count_ = count;
    }

    ~Block() = default;

    Block* next_ = nullptr;
    HeapObject heapObject_;
    // The rest of the data is of variable size and so is inexpressible via C++ members. But it's here.
};

// static
mm::Allocator& mm::Allocator::Instance() noexcept {
    return mm::GlobalData::Instance().allocator();
}

ObjHeader* mm::Allocator::AllocateObject(ThreadData* threadData, const TypeInfo* typeInfo) noexcept {
    auto& block = Block::CreateObject(root_, typeInfo);
    root_ = &block;
    return block.GetObjHeader();
}

ArrayHeader* mm::Allocator::AllocateArray(ThreadData* threadData, const TypeInfo* typeInfo, uint32_t count) noexcept {
    auto& block = Block::CreateArray(root_, typeInfo, count);
    root_ = &block;
    return block.GetArrayHeader();
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
