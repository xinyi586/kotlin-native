/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include "ThreadData.hpp"
#include "ThreadState.hpp"

using namespace kotlin;

namespace {

// TODO: Can we make it constexpr? Is it worthwhile at all?
bool isStateSwitchAllowed(mm::ThreadState oldState, mm::ThreadState newState) {
    switch (oldState) {
        case mm::kRunnable:
            return newState == mm::kNative;
        case mm::kNative:
            return newState == mm::kRunnable;
    }
}

// TODO: Is there a more general way to get an enum name in runtime?
std::string stateToString(mm::ThreadState state) {
    switch (state) {
        case mm::kRunnable:
            return "RUNNABLE";
        case mm::kNative:
            return "NATIVE";
    }
}

std::string unexpectedStateMessage(mm::ThreadState expected, mm::ThreadState actual) {
    return std::string("Unexpected thread state. Expected: ") + stateToString(expected)
        + ". Actual: " + stateToString(actual);
}

std::string illegalStateSwitchMessage(mm::ThreadState oldState, mm::ThreadState newState) {
    return std::string("Illegal thread state switch. Old state: ") + stateToString(oldState)
        + ". New state: " + stateToString(newState);
}

} // namespace

// Switches the state of the current thread to `newState` and returns the previous state.
ALWAYS_INLINE RUNTIME_NOTHROW mm::ThreadState mm::SwitchThreadState(mm::ThreadState newState) {
    auto threadData = ThreadRegistry::Instance().CurrentThreadData();
    auto oldState = threadData->state();
    // TODO(perf): Mesaure the impact of this assert in debug and opt modes.
    RuntimeAssert(isStateSwitchAllowed(oldState, newState),
                  illegalStateSwitchMessage(oldState, newState).c_str());
    threadData->setState(newState);
    return oldState;
}

ALWAYS_INLINE void mm::AssertThreadState(mm::ThreadState expected) {
    auto actual = ThreadRegistry::Instance().CurrentThreadData()->state();
    RuntimeAssert(actual == expected, unexpectedStateMessage(expected, actual).c_str());
}

mm::ThreadStateGuard::ThreadStateGuard(ThreadState state) {
    oldState_ = SwitchThreadState(state);
}

mm::ThreadStateGuard::~ThreadStateGuard() {
    SwitchThreadState(oldState_);
}

extern "C" ALWAYS_INLINE RUNTIME_NOTHROW void Kotlin_mm_switchThreadStateNative() {
    mm::SwitchThreadState(mm::kNative);
}

extern "C" ALWAYS_INLINE RUNTIME_NOTHROW void Kotlin_mm_switchThreadStateRunnable() {
    mm::SwitchThreadState(mm::kRunnable);
}

