/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_ALLOCATOR_H
#define RUNTIME_MM_ALLOCATOR_H

#include <mutex>

#include "Memory.h"
#include "Mutex.hpp"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

class Allocator : private Pinned {
public:
    // This class does not know its size at compile-time.
    class Node : private Pinned {
    public:
        ~Node();

        ObjHeader* GetObjHeader() noexcept;
        ArrayHeader* GetArrayHeader() noexcept;

    private:
        friend class Allocator;

        explicit Node(const TypeInfo* typeInfo) noexcept;
        Node(const TypeInfo* typeInfo, uint32_t count) noexcept;

        void* ExtraData() noexcept { return this + 1; }

        std::unique_ptr<Node> next_;
        // There's some more data of an unknown (at compile-time) size here, but it cannot be represented
        // with C++ members.
    };

    class ThreadQueue {
    public:
        explicit ThreadQueue(Allocator& owner) noexcept : owner_(owner) {}

        ObjHeader* AllocateObject(const TypeInfo* typeInfo) noexcept;
        ArrayHeader* AllocateArray(const TypeInfo* typeInfo, uint32_t count) noexcept;

        void Publish() noexcept;

    private:
        void InsertNode(std::unique_ptr<Node> node) noexcept;

        Allocator& owner_; // weak
        std::unique_ptr<Node> root_;
        Node* last_ = nullptr;
    };

    class Iterator {
    public:
        Node& operator*() noexcept { return *node_; }

        Iterator& operator++() noexcept {
            previousNode_ = node_;
            node_ = node_->next_.get();
            return *this;
        }

        bool operator==(const Iterator& rhs) const noexcept { return node_ == rhs.node_; }

        bool operator!=(const Iterator& rhs) const noexcept { return node_ != rhs.node_; }

    private:
        friend class Allocator;

        Iterator(Node* previousNode, Node* node) noexcept : previousNode_(previousNode), node_(node) {}

        Node* previousNode_; // Kept for `Iterable::Erase`.
        Node* node_;
    };

    class Iterable {
    public:
        explicit Iterable(Allocator& owner) noexcept : owner_(owner), guard_(owner_.mutex_) {}

        Iterator begin() noexcept { return Iterator(nullptr, owner_.root_.get()); }
        Iterator end() noexcept { return Iterator(owner_.last_, nullptr); }

        void Erase(const Iterator& iterator) noexcept { owner_.EraseUnsafe(iterator.previousNode_); }

    private:
        Allocator& owner_; // weak
        std::unique_lock<SimpleMutex> guard_;
    };

    static Allocator& Instance() noexcept;

    Iterable Iter() noexcept { return Iterable(*this); }

private:
    friend class GlobalData;

    Allocator() noexcept;
    ~Allocator();

    // Expects `mutex_` to be held by the current thread.
    void EraseUnsafe(Node* previousNode) noexcept;

    std::unique_ptr<Node> root_;
    Node* last_ = nullptr;
    SimpleMutex mutex_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_ALLOCATOR_H
