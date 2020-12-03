/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_OBJECT_INFO_H
#define RUNTIME_MM_OBJECT_INFO_H

#include "Memory.h"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

// MM-specific object info. Common between object and array.
// NOTE: Do not add any other superclasses, as it may break object layout (it must match `TypeInfo`).
// `Pinned` is ok, because it's an empty class, and empty base optimization is required to work.
class ObjectInfo : private Pinned {
public:
    // TODO: Hide MetaObjHeader conversions inside mm/Memory.cpp. This will require using
    // an abstraction over `ObjHeader` and `ArrayHeader`.
    MetaObjHeader* ToMetaObjHeader() noexcept { return reinterpret_cast<MetaObjHeader*>(this); }

    static ObjectInfo* FromMetaObjHeader(MetaObjHeader* metaObject) noexcept { return reinterpret_cast<ObjectInfo*>(metaObject); }

    explicit ObjectInfo(TypeInfo* typeInfo) noexcept : typeInfo_(typeInfo) {}

    TypeInfo* Type() const { return typeInfo_; }

private:
    // Must be the first to match `TypeInfo` layout.
    TypeInfo* typeInfo_;
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_OBJECT_INFO_H
