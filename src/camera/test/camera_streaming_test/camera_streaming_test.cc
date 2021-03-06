// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <button_checker.h>
#include <fcntl.h>
#include <fuchsia/camera/test/cpp/fidl.h>
#include <fuchsia/camera2/cpp/fidl.h>
#include <lib/devmgr-integration-test/fixture.h>  // For RecursiveWaitForFile
#include <lib/fdio/fdio.h>
#include <lib/zx/vmar.h>
#include <unistd.h>
#include <zircon/status.h>
#include <zircon/types.h>

#include <array>
#include <atomic>
#include <string>

#include <fbl/unique_fd.h>
#include <openssl/sha.h>

#include "garnet/public/lib/gtest/real_loop_fixture.h"

// fx run-test camera_full_on_device_test -t camera_streaming_test

class CameraStreamingTest : public gtest::RealLoopFixture {
 protected:
  CameraStreamingTest() {}
  ~CameraStreamingTest() override {}
  virtual void SetUp() override;
  void BindIspTester(fuchsia::camera::test::IspTesterSyncPtr& ptr);
};

// Returns the formatted sha512 string of a buffer.
std::string Hash(const void* data, size_t size) {
  static const char* table = "0123456789abcdef";
  uint8_t md[SHA512_DIGEST_LENGTH]{};
  SHA512(reinterpret_cast<const uint8_t*>(data), size, md);
  std::string ret(2 * sizeof(md) + 1, 0);
  for (uint32_t i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
    ret[2 * i] = table[(md[i] >> 4) & 0xF];
    ret[2 * i + 1] = table[md[i] & 0xF];
  }
  return ret;
}

void CameraStreamingTest::SetUp() {
  if (!VerifyDeviceUnmuted()) {
    GTEST_SKIP();
  }
}

// Connect to the ISP test device.
void CameraStreamingTest::BindIspTester(fuchsia::camera::test::IspTesterSyncPtr& ptr) {
  static const char* kIspTesterDir = "/dev/class/isp-device-test";

  int result = open(kIspTesterDir, O_RDONLY);
  ASSERT_GE(result, 0) << "Error opening " << kIspTesterDir;
  fbl::unique_fd dir_fd(result);

  fbl::unique_fd fd;
  ASSERT_EQ(devmgr_integration_test::RecursiveWaitForFile(dir_fd, "000", &fd), ZX_OK);

  zx::channel channel;
  ASSERT_EQ(fdio_get_service_handle(fd.get(), channel.reset_and_get_address()), ZX_OK);

  ptr.Bind(std::move(channel));
}

// Validate the contents of the stream coming from the ISP.
TEST_F(CameraStreamingTest, CheckStreamFromIsp) {
  // Pick something large enough that it's likely larger than any internal ring buffers, but small
  // enough that the test completes relatively quickly.
  static const uint32_t kFramesToCheck = 42;

  // Connect to the tester.
  fuchsia::camera::test::IspTesterSyncPtr tester;
  ASSERT_NO_FATAL_FAILURE(BindIspTester(tester));
  ASSERT_TRUE(tester.is_bound());

  // Request a stream.
  fuchsia::camera2::StreamPtr stream;
  fuchsia::sysmem::BufferCollectionInfo_2 buffers;
  fuchsia::sysmem::ImageFormat_2 format;
  ASSERT_EQ(tester->CreateStream(stream.NewRequest(), &buffers, &format), ZX_OK);

  std::atomic_bool stream_alive = true;
  stream.set_error_handler([&](zx_status_t status) {
    ADD_FAILURE_AT(__FILE__, __LINE__) << "Stream disconnected: " << zx_status_get_string(status);
    stream_alive = false;
  });

  // Populate a set of known hashes to constant-value frame data.
  std::map<std::string, uint8_t> known_hashes;
  {
    // Try known transients 0x00 and 0xFF, as well as other likely transients near values k*2^N.
    static constexpr const std::array<uint8_t, 10> kValuesToCheck{0x00, 0xFF, 0x01, 0xFE, 0x7F,
                                                                  0x80, 0x3F, 0x40, 0xBF, 0xC0};
    std::vector<uint8_t> known_frame(buffers.settings.buffer_settings.size_bytes);
    for (size_t i = 0; i < kValuesToCheck.size(); ++i) {
      auto value = kValuesToCheck[i];
      std::cout << "\rCalculating hash for fixed value " << static_cast<uint32_t>(value) << " ("
                << i + 1 << "/" << kValuesToCheck.size() << ")";
      std::cout.flush();
      memset(known_frame.data(), value, known_frame.size());
      known_hashes[Hash(known_frame.data(), known_frame.size())] = value;
    }
    std::cout << std::endl;
  }

  // Register a frame event handler.
  std::map<std::string, uint32_t> frame_hashes;
  std::vector<bool> buffer_owned(buffers.buffer_count, false);
  std::atomic_uint32_t frames_received{0};
  stream.events().OnFrameAvailable = [&](fuchsia::camera2::FrameAvailableInfo event) {
    if (++frames_received > kFramesToCheck) {
      // If we've reached the target number of frames, just release the frame and return.
      stream->ReleaseFrame(event.buffer_id);
      return;
    }

    // Check ownership validity of the buffer.
    ASSERT_LT(event.buffer_id, buffers.buffer_count);
    EXPECT_FALSE(buffer_owned[event.buffer_id])
        << "Server sent frame " << event.buffer_id << " again without the client releasing it.";
    buffer_owned[event.buffer_id] = true;

    // Map and hash the entire contents of the buffer.
    uintptr_t mapped_addr = 0;
    ASSERT_EQ(zx::vmar::root_self()->map(0, buffers.buffers[event.buffer_id].vmo, 0,
                                         buffers.settings.buffer_settings.size_bytes,
                                         ZX_VM_PERM_READ, &mapped_addr),
              ZX_OK);
    std::cout << "\rCalculating hash for frame " << frames_received << "/" << kFramesToCheck;
    std::cout.flush();
    auto hash =
        Hash(reinterpret_cast<void*>(mapped_addr), buffers.settings.buffer_settings.size_bytes);
    ASSERT_EQ(
        zx::vmar::root_self()->unmap(mapped_addr, buffers.settings.buffer_settings.size_bytes),
        ZX_OK);

    // Verify the hash does not match a prior or known hash. Even with a static scene, thermal
    // noise should prevent any perfectly identical frames. As a result, this check should only
    // fail if the frames are not actually coming from the sensor, or are being recycled
    // incorrectly.
    auto it = known_hashes.find(hash);
    if (it != known_hashes.end()) {
      ADD_FAILURE_AT(__FILE__, __LINE__)
          << "Frame " << frames_received
          << " does not contain valid image data - it is just the constant byte value "
          << it->second;
    } else {
      auto it = frame_hashes.find(hash);
      if (it == frame_hashes.end()) {
        frame_hashes.emplace(hash, frames_received);
      } else {
        ADD_FAILURE_AT(__FILE__, __LINE__)
            << "Duplicate frame - the contents of frames " << it->second << " and "
            << frames_received << " both hash to 0x" << hash;
      }
    }
    buffer_owned[event.buffer_id] = false;
    stream->ReleaseFrame(event.buffer_id);
  };

  // Start the stream.
  stream->Start();

  // Begin the message loop, exiting when a certain number of frames are received, or the stream
  // connection dies.
  RunLoopUntil([&]() { return !stream_alive || frames_received >= kFramesToCheck; });
  std::cout << std::endl;
  ASSERT_TRUE(stream_alive);

  // Stop the stream.
  stream->Stop();
  RunLoopUntilIdle();
}
