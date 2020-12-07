/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#ifndef RUNTIME_MM_THREAD_STATE_H
#define RUNTIME_MM_THREAD_STATE_H

#include <Common.h>
#include <Utils.hpp>

namespace kotlin {
namespace mm {

enum ThreadState {
    kRunnable, kNative
};

// Switches the state of the current thread to `newState` and returns the previous thread state.
ALWAYS_INLINE RUNTIME_NOTHROW ThreadState SwitchThreadState(ThreadState newState);

ALWAYS_INLINE void AssertThreadState(ThreadState expected);

class ThreadStateGuard final : private Pinned {
public:
    explicit ThreadStateGuard(ThreadState state);
    ~ThreadStateGuard();
private:
    ThreadState oldState_;
};

} // namespace mm
} // namespace kotlin

extern "C" {

ALWAYS_INLINE RUNTIME_NOTHROW void Kotlin_mm_switchThreadStateNative();
ALWAYS_INLINE RUNTIME_NOTHROW void Kotlin_mm_switchThreadStateRunnable();

} // extern "C"

#endif // RUNTIME_MM_THREAD_STATE_H