/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_ALIGNMENT_H
#define RUNTIME_MM_ALIGNMENT_H

#include "Types.h"

namespace kotlin {
namespace mm {

constexpr uint32_t kObjectAlignment = 8;
static_assert(kObjectAlignment % alignof(KLong) == 0, "");
static_assert(kObjectAlignment % alignof(KDouble) == 0, "");

inline uint32_t AlignUp(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

} // namespace mm
} // namespace kotlin

#endif // RUNTIME_MM_ALIGNMENT_H
