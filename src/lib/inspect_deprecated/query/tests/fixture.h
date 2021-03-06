// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_LIB_INSPECT_DEPRECATED_QUERY_TESTS_FIXTURE_H_
#define SRC_LIB_INSPECT_DEPRECATED_QUERY_TESTS_FIXTURE_H_

#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/async/cpp/executor.h>
#include <lib/gtest/real_loop_fixture.h>
#include <zircon/assert.h>

namespace {
class TestFixture : public gtest::RealLoopFixture {
 public:
  TestFixture()
      : promise_loop_(&kAsyncLoopConfigNoAttachToCurrentThread),
        executor_(promise_loop_.dispatcher()) {
    ZX_ASSERT(promise_loop_.StartThread() == ZX_OK);
  }

  ~TestFixture() override {
    promise_loop_.Quit();
    promise_loop_.JoinThreads();
  }

  void SchedulePromise(fit::pending_task promise) { executor_.schedule_task(std::move(promise)); }

 private:
  async::Loop promise_loop_;
  async::Executor executor_;
};
}  // namespace

#endif  // SRC_LIB_INSPECT_DEPRECATED_QUERY_TESTS_FIXTURE_H_
