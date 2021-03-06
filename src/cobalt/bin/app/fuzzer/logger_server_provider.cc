// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/cobalt/cpp/fidl.h>
#include <lib/fidl/cpp/fuzzing/server_provider.h>

#include <chrono>
#include <future>

#include "fuchsia/net/oldhttp/cpp/fidl.h"
#include "lib/async/default.h"
#include "lib/sys/cpp/component_context.h"
#include "src/cobalt/bin/app/logger_impl.h"
#include "src/cobalt/bin/app/timer_manager.h"
#include "src/cobalt/bin/utils/fuchsia_http_client.h"
#include "third_party/cobalt/src/lib/clearcut/uploader.h"
#include "third_party/cobalt/src/lib/util/posix_file_system.h"
#include "third_party/cobalt/src/logger/event_aggregator.h"
#include "third_party/cobalt/src/logger/observation_writer.h"
#include "third_party/cobalt/src/logger/project_context_factory.h"
#include "third_party/cobalt/src/observation_store/memory_observation_store.h"
#include "third_party/cobalt/src/observation_store/observation_store.h"
#include "third_party/cobalt/src/system_data/client_secret.h"
#include "third_party/cobalt/src/uploader/shipping_manager.h"
#include "third_party/cobalt/src/uploader/upload_scheduler.h"

// Source of cobalt::logger::kConfig
#include "third_party/cobalt/src/logger/internal_metrics_config.cb.h"

namespace {

::fidl::fuzzing::ServerProvider<::fuchsia::cobalt::Logger, ::cobalt::LoggerImpl>*
    fuzzer_server_provider;

cobalt::logger::Encoder encoder(cobalt::encoder::ClientSecret::GenerateNewSecret(), nullptr);
auto observation_store =
    std::make_unique<cobalt::observation_store::MemoryObservationStore>(100, 100, 1000);

class NoOpHTTPClient : public cobalt::lib::clearcut::HTTPClient {
  std::future<statusor::StatusOr<cobalt::lib::clearcut::HTTPResponse>> Post(
      cobalt::lib::clearcut::HTTPRequest request,
      std::chrono::steady_clock::time_point deadline) override {
    return std::async(std::launch::async,
                      []() mutable -> statusor::StatusOr<cobalt::lib::clearcut::HTTPResponse> {
                        return cobalt::util::Status::CANCELLED;
                      });
  }
};

cobalt::encoder::ClearcutV1ShippingManager clearcut_shipping_manager(
    cobalt::encoder::UploadScheduler(std::chrono::seconds(10), std::chrono::seconds(10)),
    observation_store.get(),
    std::make_unique<cobalt::lib::clearcut::ClearcutUploader>("http://test.com",
                                                              std::make_unique<NoOpHTTPClient>()));

std::unique_ptr<cobalt::util::EncryptedMessageMaker> encrypt_to_analyzer =
    cobalt::util::EncryptedMessageMaker::MakeUnencrypted();

cobalt::logger::ObservationWriter observation_writer(observation_store.get(),
                                                     &clearcut_shipping_manager,
                                                     encrypt_to_analyzer.get());

cobalt::util::ConsistentProtoStore local_aggregate_proto_store(
    "/tmp/local_agg", std::make_unique<cobalt::util::PosixFileSystem>());
cobalt::util::ConsistentProtoStore obs_history_proto_store(
    "/tmp/obs_hist", std::make_unique<cobalt::util::PosixFileSystem>());

cobalt::logger::EventAggregator event_aggregator(&encoder, &observation_writer,
                                                 &local_aggregate_proto_store,
                                                 &obs_history_proto_store, 4);

cobalt::TimerManager manager(nullptr);

}  // namespace

// See https://fuchsia.dev/fuchsia-src/development/workflows/libfuzzer_fidl for explanations and
// documentations for these functions.
extern "C" {
zx_status_t fuzzer_init() {
  if (fuzzer_server_provider == nullptr) {
    fuzzer_server_provider =
        new ::fidl::fuzzing::ServerProvider<::fuchsia::cobalt::Logger, ::cobalt::LoggerImpl>(
            ::fidl::fuzzing::ServerProviderDispatcherMode::kFromCaller);
  }

  auto factory = cobalt::logger::ProjectContextFactory::CreateFromCobaltRegistryBase64(
      // Generated by //third_party/cobalt/src/logger:internal_metrics_config
      cobalt::logger::kConfig);
  return fuzzer_server_provider->Init(std::make_unique<cobalt::logger::Logger>(
                                          factory->TakeSingleProjectContext(), &encoder,
                                          &event_aggregator, &observation_writer, nullptr, nullptr),
                                      &manager);
}

zx_status_t fuzzer_connect(zx_handle_t channel_handle, async_dispatcher_t* dispatcher) {
  manager.UpdateDispatcher(dispatcher);
  return fuzzer_server_provider->Connect(channel_handle, dispatcher);
}

zx_status_t fuzzer_disconnect(zx_handle_t channel_handle, async_dispatcher_t* dispatcher) {
  manager.UpdateDispatcher(nullptr);
  return fuzzer_server_provider->Disconnect(channel_handle, dispatcher);
}

zx_status_t fuzzer_clean_up() { return fuzzer_server_provider->CleanUp(); }
}
