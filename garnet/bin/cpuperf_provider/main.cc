// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>

#include <trace-provider/provider.h>

#include "garnet/bin/cpuperf_provider/app.h"
#include "garnet/lib/perfmon/controller.h"
#include "src/lib/fxl/command_line.h"
#include "src/lib/fxl/log_settings_command_line.h"
#include "src/lib/fxl/logging.h"

int main(int argc, const char** argv) {
  auto command_line = fxl::CommandLineFromArgcArgv(argc, argv);
  if (!fxl::SetLogSettingsFromCommandLine(command_line))
    return 1;

  if (!perfmon::Controller::IsSupported()) {
    FXL_LOG(INFO) << "Exiting, perfmon device not supported";
    return 0;
  }

  FXL_VLOG(2) << argv[0] << ": starting";

  async::Loop loop(&kAsyncLoopConfigAttachToCurrentThread);
  trace::TraceProviderWithFdio trace_provider(loop.dispatcher(), "cpuperf_provider");

  cpuperf_provider::App app(command_line);
  loop.Run();

  FXL_VLOG(2) << argv[0] << ": exiting";

  return 0;
}
