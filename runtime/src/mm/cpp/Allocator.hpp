/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_ALLOCATOR_H
#define RUNTIME_MM_ALLOCATOR_H

#include "Alignment.hpp"
#include "HeapObject.hpp"
#include "Memory.h"
#include "Porting.h"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

class ThreadData;

class Allocator : private Pinned {
public:
    class Block final : private Pinned {
    public:
        explicit Block(Block* next) noexcept : next_(next) {}
        ~Block() = default;

    private:
        friend class Allocator;

        Block* next_ = nullptr;
    };

    class Iterator {
    public:
        Block& operator*() noexcept { return *block_; }

        Iterator& operator++() noexcept {
            previousBlock_ = block_;
            block_ = block_->next_;
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

        Iterator begin() noexcept { return Iterator(owner_.root_); }
        Iterator end() noexcept { return Iterator(nullptr); }

        void Erase(Iterator& iterator) noexcept { return owner_.Erase(iterator.previousBlock_, *iterator.block_); }

    private:
        Allocator& owner_;
    };

    static Allocator& Instance() noexcept;

    template <typename... Args>
    HeapObject& Allocate(Args... args) noexcept {
        auto allocationSize = sizeof(Block) + HeapObject::Sizeof(std::forward<Args>(args)...);
        void* location = konan::calloc(1, AlignUp(allocationSize, kObjectAlignment));
        auto* block = new (location) Block(root_);
        auto& heapObject = HeapObject::Create(GetHeapObjectPlace(block), std::forward<Args>(args)...);
        root_ = block;
        return heapObject;
    }

    HeapObject& GetHeapObject(Block& block) noexcept;

    Iterable Iter() noexcept { return Iterable(*this); }

private:
    friend class GlobalData;

    HeapObject* GetHeapObjectPlace(Block* blockPlace) noexcept;
    void Erase(Block* previousBlock, Block& block) noexcept;

    Allocator() noexcept;
    ~Allocator();

    // TODO: Use a thread-safe list.
    Block* root_ = nullptr;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_ALLOCATOR_H
