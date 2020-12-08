/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_HETEROGENEOUS_SOURCE_QUEUE_H
#define RUNTIME_HETEROGENEOUS_SOURCE_QUEUE_H

#include <mutex>

#include "Alignment.hpp"
#include "Mutex.hpp"
#include "Utils.hpp"

namespace kotlin {

// A queue that is constructed by collecting subqueues from several `Producer`s.
// TODO: Consider merging with `MultiSourceQueue` somehow.
class HeterogeneousMultiSourceQueue : private MoveOnly {
public:
    // This class does not know its size at compile-time.
    class Node : private Pinned {
    public:
        ~Node() = default;

        // TODO: Consider adding destructors for extra data.
        void* ExtraData() noexcept { return this + 1; }

    private:
        friend class HeterogeneousMultiSourceQueue;

        Node() noexcept = default;

        std::unique_ptr<Node> next_;
        // There's some more data of an unknown (at compile-time) size here, but it cannot be represented
        // with C++ members.
    };

    class Producer : private MoveOnly {
    public:
        explicit Producer(HeterogeneousMultiSourceQueue& owner) noexcept : owner_(owner) {}

        ~Producer() { Publish(); }

        Node& Insert(uint32_t extraDataSize, uint32_t extraDataAlignment) noexcept {
            uint32_t allocSize = AlignUp(sizeof(Node) + extraDataSize, extraDataAlignment);
            std::unique_ptr<Node> node(new (konanAllocMemory(allocSize)) Node());
            auto* nodePtr = node.get();
            if (!root_) {
                root_ = std::move(node);
                last_ = nodePtr;
                return *nodePtr;
            }

            last_->next_ = std::move(node);
            last_ = nodePtr;
            return *nodePtr;
        }

        // Merge `this` queue with owning `HeterogeneousMultiSourceQueue`.
        // `this` will have empty queue after the call.
        // This call is performed without heap allocations. TODO: Test that no allocations are happening.
        void Publish() noexcept {
            if (!root_) {
                RuntimeAssert(last_ == nullptr, "Unsynchronized root_ and last_");
                return;
            }

            std::lock_guard<SimpleMutex>(owner_.mutex_);

            if (!owner_.root_) {
                RuntimeAssert(owner_.last_ == nullptr, "Unsynchronized root_ and last_");
                owner_.root_ = std::move(root_);
                owner_.last_ = last_;
                last_ = nullptr;
                return;
            }

            RuntimeAssert(owner_.last_->next_ == nullptr, "last cannot have next");
            owner_.last_->next_ = std::move(root_);
            owner_.last_ = last_;
            last_ = nullptr;
        }

    private:
        friend class HeterogeneousMultiSourceQueue;

        HeterogeneousMultiSourceQueue& owner_; // weak
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
        friend class HeterogeneousMultiSourceQueue;

        Iterator(Node* previousNode, Node* node) noexcept : previousNode_(previousNode), node_(node) {}

        Node* previousNode_; // Kept for `Iterable::Erase`.
        Node* node_;
    };

    class Iterable : private MoveOnly {
    public:
        explicit Iterable(HeterogeneousMultiSourceQueue& owner) noexcept : owner_(owner), guard_(owner_.mutex_) {}

        Iterator begin() noexcept { return Iterator(nullptr, owner_.root_.get()); }
        Iterator end() noexcept { return Iterator(owner_.last_, nullptr); }

        void Erase(const Iterator& iterator) noexcept { owner_.EraseUnsafe(iterator.previousNode_); }

    private:
        HeterogeneousMultiSourceQueue& owner_; // weak
        std::unique_lock<SimpleMutex> guard_;
    };

    // Lock `HeterogeneousMultiSourceQueue` for safe iteration.
    Iterable Iter() noexcept { return Iterable(*this); }

private:
    // Expects `mutex_` to be held by the current thread.
    void EraseUnsafe(Node* previousNode) noexcept {
        if (previousNode == nullptr) {
            // Deleting the root.
            auto newRoot = std::move(root_->next_);
            root_ = std::move(newRoot);
            if (!root_) {
                last_ = nullptr;
            }
            return;
        }

        auto node = std::move(previousNode->next_);
        previousNode->next_ = std::move(node->next_);
        if (!previousNode->next_) {
            last_ = previousNode;
        }
    }

    std::unique_ptr<Node> root_;
    Node* last_ = nullptr;
    SimpleMutex mutex_;
};

} // namespace kotlin

#endif // RUNTIME_HETEROGENEOUS_SOURCE_QUEUE_H
