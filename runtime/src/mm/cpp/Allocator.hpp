/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_ALLOCATOR_H
#define RUNTIME_MM_ALLOCATOR_H

#include "Memory.h"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

class ObjectInfo;
class ThreadData;

class Allocator : private Pinned {
public:
    class Block;

    class Iterator {
    public:
        Block& operator*() noexcept { return *block_; }

        Iterator& operator++() noexcept {
            previousBlock_ = block_;
            block_ = NextBlock(*block_);
            return *this;
        }

        bool operator==(const Iterator& rhs) const noexcept { return block_ == rhs.block_; }

        bool operator!=(const Iterator& rhs) const noexcept { return block_ != rhs.block_; }

    private:
        friend class Allocator;

        // Only valid for the first block and for `nullptr`
        explicit Iterator(Block* block) noexcept : block_(block) {}

        Block* previousBlock_ = nullptr;
        Block* block_;
    };

    class Iterable : private MoveOnly {
    public:
        explicit Iterable(Allocator& owner) : owner_(owner) {}

        Iterator begin() noexcept { return Iterator(owner_.FirstBlock()); }
        Iterator end() noexcept { return Iterator(nullptr); }

        void Erase(Iterator& iterator) noexcept { return owner_.Erase(iterator.previousBlock_, *iterator.block_); }

    private:
        Allocator& owner_;
    };

    static Allocator& Instance() noexcept;

    ObjHeader* AllocateObject(ThreadData* threadData, const TypeInfo* typeInfo) noexcept;

    ArrayHeader* AllocateArray(ThreadData* threadData, const TypeInfo* typeInfo, int32_t count) noexcept;

    ObjectInfo& GetObjectInfo(Block& block) noexcept;

    Iterable Iter() noexcept { return Iterable(*this); }

private:
    friend class GlobalData;

    Block* FirstBlock() noexcept;
    static Block* NextBlock(Block& block) noexcept;
    void Erase(Block* previousBlock, Block& block) noexcept;

    Allocator() noexcept;
    ~Allocator();

    Block* root_ = nullptr;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_ALLOCATOR_H
