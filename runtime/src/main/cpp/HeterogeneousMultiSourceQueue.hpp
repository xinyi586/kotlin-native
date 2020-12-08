/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_HETEROGENEOUS_SOURCE_QUEUE_H
#define RUNTIME_HETEROGENEOUS_SOURCE_QUEUE_H

#include <memory>
#include <mutex>

#include "Alignment.hpp"
#include "KAssert.h"
#include "Utils.hpp"

namespace kotlin {

// A queue that is constructed by collecting subqueues from several `Producer`s.
// TODO: Consider merging with `MultiSourceQueue` somehow.
template <typename Mutex, typename Allocator = std::allocator<void>>
class HeterogeneousMultiSourceQueue : private MoveOnly {
public:
    class Node;

private:
    class NodeDeleter;
    using NodeOwner = std::unique_ptr<Node, NodeDeleter>;

public:
    // This class does not know its size at compile-time.
    class Node : private Pinned {
    public:
        // Note: Do not construct the `Node` directly.
        Node() noexcept = default;
        ~Node() = default;

        // TODO: Consider adding destructors for the data.
        void* Data() noexcept { return this + 1; }

    private:
        friend class HeterogeneousMultiSourceQueue;

        NodeOwner next_ = nullptr;
        // There's some more data of an unknown (at compile-time) size here, but it cannot be represented
        // with C++ members.
    };

    class Producer : private MoveOnly {
    public:
        explicit Producer(HeterogeneousMultiSourceQueue& owner) noexcept : Producer(owner, Allocator()) {}
        Producer(HeterogeneousMultiSourceQueue& owner, const Allocator& allocator) noexcept : owner_(owner), allocator_(allocator) {}
        Producer(HeterogeneousMultiSourceQueue& owner, Allocator&& allocator) noexcept : owner_(owner), allocator_(std::move(allocator)) {}

        ~Producer() { Publish(); }

        Node& Insert(size_t dataSize, size_t dataAlignment) noexcept {
            auto node = MakeNode(dataSize, dataAlignment);
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

            std::lock_guard<Mutex>(owner_.mutex_);

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

        NodeOwner MakeNode(size_t dataSize, size_t dataAlignment) noexcept {
            size_t allocSize = AlignUp(sizeof(Node) + dataSize, dataAlignment);
            auto* nodePtr = static_cast<Node*>(std::allocator_traits<Allocator>::allocate(allocator_, allocSize));
            std::allocator_traits<Allocator>::construct(allocator_, nodePtr);
            NodeOwner node(nodePtr, NodeDeleter(allocator_, allocSize));
            return node;
        }

        HeterogeneousMultiSourceQueue& owner_; // weak
        Allocator allocator_;
        NodeOwner root_;
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
        std::unique_lock<Mutex> guard_;
    };

    // Lock `HeterogeneousMultiSourceQueue` for safe iteration.
    Iterable Iter() noexcept { return Iterable(*this); }

private:
    class NodeDeleter {
    public:
        NodeDeleter() noexcept = default;

        NodeDeleter(const Allocator& allocator, size_t size) noexcept : custom_(true), allocator_(allocator), size_(size) {
            RuntimeAssert(allocator_ == allocator, "NodeDeleter allocator must be able to delete what was allocated");
        }

        void operator()(Node* node) noexcept {
            if (custom_) {
                std::allocator_traits<Allocator>::destroy(allocator_, node);
                std::allocator_traits<Allocator>::deallocate(allocator_, node, size_);
            } else {
                delete node;
            }
        }

    private:
        bool custom_ = false;
        Allocator allocator_;
        size_t size_;
    };

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

    NodeOwner root_;
    Node* last_ = nullptr;
    Mutex mutex_;
};

} // namespace kotlin

#endif // RUNTIME_HETEROGENEOUS_SOURCE_QUEUE_H
