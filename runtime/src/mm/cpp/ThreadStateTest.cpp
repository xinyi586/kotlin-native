/*
 * Copyright 2010-2020 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */

#include <thread>

#include "gtest/gtest.h"

#include "ThreadData.hpp"
#include "ThreadRegistry.hpp"
#include "ThreadState.hpp"

using namespace kotlin;

TEST(ThreadStateTest, ThreadStateSwitch) {
    std::thread t([]() {
        mm::ThreadRegistry::Instance().RegisterCurrentThread();

        auto* threadData = mm::ThreadRegistry::Instance().CurrentThreadData();
        auto initialState = threadData->state();
        EXPECT_EQ(mm::ThreadState::kRunnable, initialState);

        mm::ThreadState oldState = mm::SwitchThreadState(mm::kNative);
        EXPECT_EQ(initialState, oldState);
        EXPECT_EQ(mm::kNative, threadData->state());

        // Check functions exported for the compiler too.
        Kotlin_mm_switchThreadStateRunnable();
        EXPECT_EQ(mm::kRunnable, threadData->state());

        Kotlin_mm_switchThreadStateNative();
        EXPECT_EQ(mm::kNative, threadData->state());
    });
    t.join();
}

TEST(ThreadStateTest, ThreadStateGuard) {
    std::thread t([]() {
        mm::ThreadRegistry::Instance().RegisterCurrentThread();
        auto* threadData = mm::ThreadRegistry::Instance().CurrentThreadData();
        auto initialState = threadData->state();
        EXPECT_EQ(mm::ThreadState::kRunnable, initialState);
        {
            mm::ThreadStateGuard guard(mm::ThreadState::kNative);
            EXPECT_EQ(mm::ThreadState::kNative, threadData->state());
        }
        EXPECT_EQ(initialState, threadData->state());
    });
    t.join();
}
