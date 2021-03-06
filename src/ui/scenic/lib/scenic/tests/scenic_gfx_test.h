// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_SCENIC_LIB_SCENIC_TESTS_SCENIC_GFX_TEST_H_
#define SRC_UI_SCENIC_LIB_SCENIC_TESTS_SCENIC_GFX_TEST_H_

#include "src/ui/lib/escher/impl/command_buffer_sequencer.h"
#include "src/ui/scenic/lib/gfx/displays/display.h"
#include "src/ui/scenic/lib/gfx/engine/engine.h"
#include "src/ui/scenic/lib/scenic/tests/scenic_test.h"

namespace scenic_impl {
namespace test {

// Subclass of ScenicTest for tests requiring Scenic with a gfx system installed
class ScenicGfxTest : public ScenicTest {
 protected:
  void TearDown() override;
  void InitializeScenic(Scenic* scenic) override;

 private:
  std::unique_ptr<escher::impl::CommandBufferSequencer> command_buffer_sequencer_;
  std::unique_ptr<gfx::Engine> engine_;
  std::shared_ptr<gfx::FrameScheduler> frame_scheduler_;
  std::unique_ptr<gfx::Display> display_;
};

}  // namespace test
}  // namespace scenic_impl

#endif  // SRC_UI_SCENIC_LIB_SCENIC_TESTS_SCENIC_GFX_TEST_H_
