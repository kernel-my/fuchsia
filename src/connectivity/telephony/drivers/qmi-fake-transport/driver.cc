// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <zircon/status.h>

#include <cstdio>
#include <future>
#include <memory>
#include <thread>

#include <ddk/binding.h>
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/protocol/test.h>

#include "fake-device.h"

zx_status_t qmi_fake_bind(void* ctx, zx_device_t* device) {
  auto dev = std::make_unique<qmi_fake::Device>(device);
  auto status = dev->Bind();
  std::printf("%s\n", __func__);
  if (status != ZX_OK) {
    std::printf("qmi_fake_bind: could not bind: %d\n", status);
  } else {
    dev.release();
  }

  return status;
}

static constexpr zx_driver_ops_t qmi_fake_driver_ops = []() {
  zx_driver_ops_t ops = {};
  ops.version = DRIVER_OPS_VERSION;
  ops.bind = qmi_fake_bind;
  return ops;
}();

// clang-format off
ZIRCON_DRIVER_BEGIN(qmi_fake, qmi_fake_driver_ops, "zircon", "0.1", 1)
  BI_MATCH_IF(EQ, BIND_PROTOCOL, ZX_PROTOCOL_TEST_PARENT),
ZIRCON_DRIVER_END(qmi_fake)
