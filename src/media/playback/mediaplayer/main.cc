// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/media/cpp/fidl.h>
#include <fuchsia/media/playback/cpp/fidl.h>
#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/async/cpp/task.h>
#include <lib/svc/cpp/services.h>
#include <lib/sys/cpp/component_context.h>
#include <lib/trace-provider/provider.h>

#include <string>

#include <fs/pseudo_file.h>

#include "src/media/playback/mediaplayer/player_impl.h"

const std::string kIsolateUrl = "fuchsia-pkg://fuchsia.com/mediaplayer#meta/mediaplayer.cmx";
const std::string kIsolateArgument = "--transient";

// Connects to the requested service in a mediaplayer isolate.
template <typename Interface>
void ConnectToIsolate(fidl::InterfaceRequest<Interface> request, fuchsia::sys::Launcher* launcher) {
  fuchsia::sys::LaunchInfo launch_info;
  launch_info.url = kIsolateUrl;
  launch_info.arguments.emplace({kIsolateArgument});
  component::Services services;
  launch_info.directory_request = services.NewRequest();

  fuchsia::sys::ComponentControllerPtr controller;
  launcher->CreateComponent(std::move(launch_info), controller.NewRequest());

  services.ConnectToService(std::move(request), Interface::Name_);

  controller->Detach();
}

int main(int argc, const char** argv) {
  bool transient = false;
  for (int arg_index = 0; arg_index < argc; ++arg_index) {
    if (argv[arg_index] == kIsolateArgument) {
      transient = true;
      break;
    }
  }

  async::Loop loop(&kAsyncLoopConfigAttachToCurrentThread);
  trace::TraceProviderWithFdio trace_provider(loop.dispatcher());

  std::unique_ptr<sys::ComponentContext> component_context = sys::ComponentContext::Create();

  if (transient) {
    std::unique_ptr<media_player::PlayerImpl> player;
    component_context->outgoing()->AddPublicService<fuchsia::media::playback::Player>(
        [component_context = component_context.get(), &player,
         &loop](fidl::InterfaceRequest<fuchsia::media::playback::Player> request) {
          player = media_player::PlayerImpl::Create(
              std::move(request), component_context,
              [&loop]() { async::PostTask(loop.dispatcher(), [&loop]() { loop.Quit(); }); });
        });

    loop.Run();
  } else {
    fuchsia::sys::LauncherPtr launcher;
    fuchsia::sys::EnvironmentPtr environment;
    component_context->svc()->Connect(environment.NewRequest());
    environment->GetLauncher(launcher.NewRequest());

    component_context->outgoing()->AddPublicService<fuchsia::media::playback::Player>(
        [&launcher](fidl::InterfaceRequest<fuchsia::media::playback::Player> request) {
          ConnectToIsolate<fuchsia::media::playback::Player>(std::move(request), launcher.get());
        });

    loop.Run();
  }

  return 0;
}
