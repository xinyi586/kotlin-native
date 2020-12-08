/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_ALLOCATOR_H
#define RUNTIME_MM_ALLOCATOR_H

#include <mutex>

#include "Alloc.h"
#include "HeterogeneousMultiSourceQueue.hpp"
#include "Memory.h"
#include "Mutex.hpp"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

namespace internal {

class KotlinObjectAllocator {
public:
    using value_type = void;
    using is_always_equal = std::true_type;

    void* allocate(size_t size) noexcept {
        // TODO: Call into GC, if failed to allocate.
        return konanAllocMemory(size);
    }

    void deallocate(void* ptr, size_t size) noexcept { konanFreeMemory(ptr); }

    bool operator==(const KotlinObjectAllocator& rhs) const { return true; }

    bool operator!=(const KotlinObjectAllocator& rhs) const { return false; }
};

} // namespace internal

class Allocator : private Pinned {
public:
    using Storage = HeterogeneousMultiSourceQueue<SimpleMutex, internal::KotlinObjectAllocator>;

    class ThreadQueue : private MoveOnly {
    public:
        explicit ThreadQueue(Allocator& owner) noexcept : producer_(owner.allocations_) {}

        ObjHeader* AllocateObject(const TypeInfo* typeInfo) noexcept;
        ArrayHeader* AllocateArray(const TypeInfo* typeInfo, uint32_t count) noexcept;

        void Publish() noexcept { producer_.Publish(); }

    private:
        Storage::Producer producer_;
    };

    using Iterator = Storage::Iterator;
    using Iterable = Storage::Iterable;

    static Allocator& Instance() noexcept;

    Iterable Iter() noexcept { return allocations_.Iter(); }

private:
    friend class GlobalData;

    Allocator() noexcept;
    ~Allocator();

    Storage allocations_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_ALLOCATOR_H
