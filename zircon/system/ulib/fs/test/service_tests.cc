// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/io/llcpp/fidl.h>
#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/fdio/directory.h>
#include <lib/fdio/fd.h>
#include <lib/fdio/fdio.h>

#include <utility>

#include <fs/pseudo_dir.h>
#include <fs/service.h>
#include <fs/synchronous_vfs.h>
#include <unittest/unittest.h>

namespace {

namespace fio = ::llcpp::fuchsia::io;

bool test_service() {
  BEGIN_TEST;

  // set up a service which can only be bound once (to make it easy to
  // simulate an error to test error reporting behavior from the connector)
  zx::channel bound_channel;
  auto svc = fbl::AdoptRef<fs::Service>(new fs::Service([&bound_channel](zx::channel channel) {
    if (bound_channel)
      return ZX_ERR_IO;
    bound_channel = std::move(channel);
    return ZX_OK;
  }));

  fs::VnodeConnectionOptions options_readable;
  options_readable.rights.read = true;

  // open
  fbl::RefPtr<fs::Vnode> redirect;
  EXPECT_EQ(ZX_OK, svc->ValidateOptions(options_readable));
  EXPECT_EQ(ZX_OK, svc->Open(options_readable, &redirect));
  EXPECT_NULL(redirect);

  // get attr
  fs::VnodeAttributes attr;
  EXPECT_EQ(ZX_OK, svc->GetAttributes(&attr));
  EXPECT_EQ(V_TYPE_FILE, attr.mode);
  EXPECT_EQ(1, attr.link_count);

  // make some channels we can use for testing
  zx::channel c1, c2;
  EXPECT_EQ(ZX_OK, zx::channel::create(0u, &c1, &c2));
  zx_handle_t hc1 = c1.get();

  // serve, the connector will return success the first time
  fs::SynchronousVfs vfs;
  EXPECT_EQ(ZX_OK, svc->Serve(&vfs, std::move(c1), options_readable));
  EXPECT_EQ(hc1, bound_channel.get());

  // the connector will return failure because bound_channel is still valid
  // we test that the error is propagated back up through Serve
  EXPECT_EQ(ZX_ERR_IO, svc->Serve(&vfs, std::move(c2), options_readable));
  EXPECT_EQ(hc1, bound_channel.get());

  END_TEST;
}

bool TestServeDirectory() {
  BEGIN_TEST;

  zx::channel client, server;
  EXPECT_EQ(ZX_OK, zx::channel::create(0u, &client, &server));

  // open client
  zx::channel c1, c2;
  EXPECT_EQ(ZX_OK, zx::channel::create(0u, &c1, &c2));
  EXPECT_EQ(ZX_OK, fdio_service_connect_at(client.get(), "abc", c2.release()));

  // close client
  // We test the semantic that a pending open is processed even if the client
  // has been closed.
  client.reset();

  // serve
  async::Loop loop(&kAsyncLoopConfigNoAttachToCurrentThread);
  fs::SynchronousVfs vfs(loop.dispatcher());

  auto directory = fbl::AdoptRef<fs::PseudoDir>(new fs::PseudoDir());
  auto vnode = fbl::AdoptRef<fs::Service>(new fs::Service([&loop](zx::channel channel) {
    loop.Shutdown();
    return ZX_OK;
  }));
  directory->AddEntry("abc", vnode);

  EXPECT_EQ(ZX_OK, vfs.ServeDirectory(directory, std::move(server)));
  EXPECT_EQ(ZX_ERR_BAD_STATE, loop.RunUntilIdle());

  END_TEST;
}

bool TestServiceNodeIsNotDirectory() {
  BEGIN_TEST;

  // Set up the server
  zx::channel client_end, server_end;
  ASSERT_EQ(ZX_OK, zx::channel::create(0u, &client_end, &server_end));

  async::Loop loop(&kAsyncLoopConfigNoAttachToCurrentThread);
  fs::SynchronousVfs vfs(loop.dispatcher());

  auto directory = fbl::AdoptRef<fs::PseudoDir>(new fs::PseudoDir());
  auto vnode = fbl::AdoptRef<fs::Service>(new fs::Service([](zx::channel channel) {
    // Should never reach here, because the directory flag is not allowed.
    EXPECT_TRUE(false, "Should not be able to open the service");
    channel.reset();
    return ZX_OK;
  }));
  directory->AddEntry("abc", vnode);
  ASSERT_EQ(ZX_OK, vfs.ServeDirectory(directory, std::move(server_end)));

  // Call |ValidateOptions| with the directory flag should fail.
  ASSERT_EQ(ZX_ERR_NOT_DIR,
            vnode->ValidateOptions(fs::VnodeConnectionOptions::ReadWrite().set_directory()));

  // Open the service through FIDL with the directory flag, which should fail.
  zx::channel abc_client_end, abc_server_end;
  ASSERT_EQ(ZX_OK, zx::channel::create(0u, &abc_client_end, &abc_server_end));

  loop.StartThread();

  auto open_result =
      fio::Directory::Call::Open(zx::unowned_channel(client_end),
                                 fio::OPEN_FLAG_DESCRIBE | fio::OPEN_FLAG_DIRECTORY |
                                     fio::OPEN_RIGHT_READABLE | fio::OPEN_RIGHT_WRITABLE,
                                 0755, fidl::StringView("abc"), std::move(abc_server_end));
  EXPECT_EQ(open_result.status(), ZX_OK);
  zx_status_t event_status = fio::Node::Call::HandleEvents(zx::unowned_channel(abc_client_end),
                                                           fio::Node::EventHandlers{
      .on_open = [](zx_status_t status, fio::NodeInfo* info) {
        EXPECT_EQ(ZX_ERR_NOT_DIR, status);
        EXPECT_EQ(nullptr, info);
        return ZX_OK;
      },
      .unknown = []() { return ZX_ERR_INVALID_ARGS; }
  });
  // Expect that |on_open| was received
  EXPECT_EQ(ZX_OK, event_status);

  loop.Shutdown();

  END_TEST;
}

}  // namespace

BEGIN_TEST_CASE(service_tests)
RUN_TEST(test_service)
RUN_TEST(TestServeDirectory)
RUN_TEST(TestServiceNodeIsNotDirectory)
END_TEST_CASE(service_tests)
