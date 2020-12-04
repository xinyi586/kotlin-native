/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_HEAP_OBJECT_H
#define RUNTIME_MM_HEAP_OBJECT_H

#include "Memory.h"
#include "Utils.hpp"

namespace kotlin {
namespace mm {

// MM-specific object info. Common between object and array.
class HeapObject : private Pinned {
public:
private:
};

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_OBJECT_INFO_H
