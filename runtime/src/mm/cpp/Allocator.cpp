/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "Allocator.hpp"

#include "Alloc.h"
#include "GlobalData.hpp"
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

// No need to clear extra data.
mm::Allocator::Node::~Node() = default;

ObjHeader* mm::Allocator::Node::GetObjHeader() noexcept {
    auto* object = static_cast<ObjHeader*>(ExtraData());
    RuntimeAssert(!object->type_info()->IsArray(), "Must not be an array");
    return object;
}

ArrayHeader* mm::Allocator::Node::GetArrayHeader() noexcept {
    auto* array = static_cast<ArrayHeader*>(ExtraData());
    RuntimeAssert(array->type_info()->IsArray(), "Must be an array");
    return array;
}

mm::Allocator::Node::Node(const TypeInfo* typeInfo) noexcept {
    RuntimeAssert(!typeInfo->IsArray(), "Must not be an array");
    auto* object = static_cast<ObjHeader*>(ExtraData());
    object->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
}

mm::Allocator::Node::Node(const TypeInfo* typeInfo, uint32_t count) noexcept {
    RuntimeAssert(typeInfo->IsArray(), "Must be an array");
    auto* array = static_cast<ArrayHeader*>(ExtraData());
    array->typeInfoOrMeta_ = const_cast<TypeInfo*>(typeInfo);
    array->count_ = count;
}

ObjHeader* mm::Allocator::ThreadQueue::AllocateObject(const TypeInfo* typeInfo) noexcept {
    RuntimeAssert(!typeInfo->IsArray(), "Must not be an array");
    uint32_t allocSize = sizeof(Node) + typeInfo->instanceSize_;
    std::unique_ptr<Node> node(konanConstructSizedInstance<Node>(AlignUp(allocSize, kObjectAlignment)));
    auto* object = node->GetObjHeader();
    InsertNode(std::move(node));
    return object;
}

ArrayHeader* mm::Allocator::ThreadQueue::AllocateArray(const TypeInfo* typeInfo, uint32_t count) noexcept {
    RuntimeAssert(typeInfo->IsArray(), "Must be an array");
    // Note: array body is aligned, but for size computation it is enough to align the sum.
    uint32_t allocSize = sizeof(Node) + AlignUp(sizeof(ArrayHeader) - typeInfo->instanceSize_ * count, kObjectAlignment);
    std::unique_ptr<Node> node(konanConstructSizedInstance<Node>(AlignUp(allocSize, kObjectAlignment)));
    auto* array = node->GetArrayHeader();
    InsertNode(std::move(node));
    return array;
}

void mm::Allocator::ThreadQueue::Publish() noexcept {
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

void mm::Allocator::ThreadQueue::InsertNode(std::unique_ptr<Node> node) noexcept {
    auto* nodePtr = node.get();
    if (!root_) {
        root_ = std::move(node);
        last_ = nodePtr;
        return;
    }

    last_->next_ = std::move(node);
    last_ = nodePtr;
}

// static
mm::Allocator& mm::Allocator::Instance() noexcept {
    return GlobalData::Instance().allocator();
}

mm::Allocator::Allocator() noexcept = default;
mm::Allocator::~Allocator() = default;

void mm::Allocator::EraseUnsafe(Node* previousNode) noexcept {
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
