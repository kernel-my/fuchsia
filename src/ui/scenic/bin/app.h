// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_SCENIC_BIN_APP_H_
#define SRC_UI_SCENIC_BIN_APP_H_

#include <lib/async/cpp/executor.h>
#include <lib/fit/function.h>

#include <memory>

#include "src/lib/fsl/io/device_watcher.h"
#include "src/ui/lib/escher/escher.h"
#include "src/ui/scenic/lib/gfx/engine/engine.h"
#include "src/ui/scenic/lib/gfx/engine/frame_scheduler.h"
#include "src/ui/scenic/lib/scenic/scenic.h"

namespace scenic_impl {

class App {
 public:
  explicit App(std::unique_ptr<sys::ComponentContext> app_context,
               inspect_deprecated::Node inspect_node, fit::closure quit_callback);

 private:
  void InitializeServices(escher::EscherUniquePtr escher, gfx::Display* display);

  async::Executor executor_;
  gfx::Sysmem sysmem_;
  gfx::DisplayManager display_manager_;
  escher::EscherUniquePtr escher_;
  std::shared_ptr<gfx::FrameScheduler> frame_scheduler_;
  std::unique_ptr<sys::ComponentContext> app_context_;
  std::optional<gfx::Engine> engine_;
  Scenic scenic_;
  std::unique_ptr<fsl::DeviceWatcher> device_watcher_;
};

}  // namespace scenic_impl

#endif  // SRC_UI_SCENIC_BIN_APP_H_
