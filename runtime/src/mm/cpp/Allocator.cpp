/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Allocator.hpp"

#include "GlobalData.hpp"
#include "Porting.h"
#include "ObjectInfo.hpp"

using namespace kotlin;

// This is a weird class that doesn't know it's size at compile-time.
class mm::Allocator::Block final : private Pinned {
public:
    // Allocate and initialize block.
    static Block& CreateObject(Block* nextBlock, const TypeInfo* typeInfo) noexcept;
    static Block& CreateArray(Block* nextBlock, const TypeInfo* typeInfo, int32_t count) noexcept;

    // Deinitialize and destroy `this`.
    void Destroy(Block* previousBlock) noexcept {
        if (previousBlock != nullptr) {
            previousBlock->next_ = next_;
        }
        this->~Block();
        konan::free(this);
    }

    ObjHeader* GetObjHeader() noexcept {
        RuntimeAssert(!objectInfo_.Type()->IsArray(), "Must not be an array");
        return &objHeader;
    }

    ArrayHeader* GetArrayHeader() noexcept {
        RuntimeAssert(objectInfo_.Type()->IsArray(), "Must be an array");
        return &arrayHeader;
    }

    ObjectInfo& GetObjectInfo() noexcept { return objectInfo_; }

    Block* Next() const noexcept { return next_; }

private:
    // Hide all the ways of constructing this class.
    explicit Block(const TypeInfo* typeInfo) noexcept : objectInfo_(typeInfo), objHeader() {
        RuntimeAssert(!typeInfo->IsArray(), "Must not be an array");
    }

    Block(const TypeInfo* typeInfo, int32_t count) noexcept : objectInfo_(typeInfo), arrayHeader() {
        RuntimeAssert(!typeInfo->IsArray(), "Must be an array");
    }

    ~Block() = default;

    Block* next_ = nullptr;
    ObjectInfo objectInfo_;
    union {
        ObjHeader objHeader;
        ArrayHeader arrayHeader;
    };
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

ArrayHeader* mm::Allocator::AllocateArray(ThreadData* threadData, const TypeInfo* typeInfo, int32_t count) noexcept {
    auto& block = Block::CreateArray(root_, typeInfo, count);
    root_ = &block;
    return block.GetArrayHeader();
}

mm::ObjectInfo& mm::Allocator::GetObjectInfo(Block& block) noexcept {
    return block.GetObjectInfo();
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
