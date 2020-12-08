/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_ALLOCATOR_H
#define RUNTIME_MM_ALLOCATOR_H

#include <mutex>

#include "HeterogeneousMultiSourceQueue.hpp"
#include "Memory.h"
#include "Mutex.hpp"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

class Allocator : private Pinned {
public:
    class ThreadQueue : private MoveOnly {
    public:
        explicit ThreadQueue(Allocator& owner) noexcept : producer_(owner.allocations_) {}

        ObjHeader* AllocateObject(const TypeInfo* typeInfo) noexcept;
        ArrayHeader* AllocateArray(const TypeInfo* typeInfo, uint32_t count) noexcept;

        void Publish() noexcept { producer_.Publish(); }

    private:
        HeterogeneousMultiSourceQueue<SimpleMutex>::Producer producer_;
    };

    using Iterator = HeterogeneousMultiSourceQueue<SimpleMutex>::Iterator;
    using Iterable = HeterogeneousMultiSourceQueue<SimpleMutex>::Iterable;

    static Allocator& Instance() noexcept;

    Iterable Iter() noexcept { return allocations_.Iter(); }

private:
    friend class GlobalData;

    Allocator() noexcept;
    ~Allocator();

    HeterogeneousMultiSourceQueue<SimpleMutex> allocations_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_ALLOCATOR_H
