// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_DISPLAY_FAKE_FAKE_DISPLAY_H_
#define ZIRCON_SYSTEM_DEV_DISPLAY_FAKE_FAKE_DISPLAY_H_

#include <lib/zircon-internal/thread_annotations.h>
#include <sys/types.h>
#include <unistd.h>
#include <zircon/compiler.h>
#include <zircon/pixelformat.h>
#include <zircon/types.h>

#include <atomic>
#include <memory>

#include <ddk/debug.h>
#include <ddk/driver.h>
#include <ddk/protocol/display/capture.h>
#include <ddk/protocol/platform/device.h>
#include <ddk/protocol/sysmem.h>
#include <ddktl/device.h>
#include <ddktl/protocol/display/capture.h>
#include <ddktl/protocol/display/controller.h>
#include <fbl/auto_lock.h>
#include <fbl/intrusive_double_list.h>
#include <fbl/mutex.h>

namespace fake_display {

struct ImageInfo : public fbl::DoublyLinkedListable<std::unique_ptr<ImageInfo>> {
  zx_paddr_t paddr;
};

class FakeDisplay;

using DeviceType = ddk::Device<FakeDisplay, ddk::GetProtocolable, ddk::UnbindableNew>;

class FakeDisplay : public DeviceType,
                    public ddk::DisplayControllerImplProtocol<FakeDisplay, ddk::base_protocol>,
                    public ddk::DisplayCaptureImplProtocol<FakeDisplay> {
 public:
  explicit FakeDisplay(zx_device_t* parent)
      : DeviceType(parent), dcimpl_proto_({&display_controller_impl_protocol_ops_, this}) {}

  // This function is called from the c-bind function upon driver matching. If start_vsync is true,
  // a background thread will be started to issue vsync events.
  zx_status_t Bind(bool start_vsync);

  // Required functions needed to implement Display Controller Protocol
  void DisplayControllerImplSetDisplayControllerInterface(
      const display_controller_interface_protocol_t* intf);
  zx_status_t DisplayControllerImplImportVmoImage(image_t* image, zx::vmo vmo, size_t offset);
  zx_status_t DisplayControllerImplImportImage(image_t* image, zx_unowned_handle_t handle,
                                               uint32_t index);
  void DisplayControllerImplReleaseImage(image_t* image);
  uint32_t DisplayControllerImplCheckConfiguration(const display_config_t** display_configs,
                                                   size_t display_count,
                                                   uint32_t** layer_cfg_results,
                                                   size_t* layer_cfg_result_count);
  void DisplayControllerImplApplyConfiguration(const display_config_t** display_config,
                                               size_t display_count);
  uint32_t DisplayControllerImplComputeLinearStride(uint32_t width, zx_pixel_format_t format);
  zx_status_t DisplayControllerImplAllocateVmo(uint64_t size, zx::vmo* vmo_out);
  zx_status_t DisplayControllerImplGetSysmemConnection(zx::channel connection);
  zx_status_t DisplayControllerImplSetBufferCollectionConstraints(const image_t* config,
                                                                  uint32_t collection);
  zx_status_t DisplayControllerImplGetSingleBufferFramebuffer(zx::vmo* out_vmo,
                                                              uint32_t* out_stride) {
    return ZX_ERR_NOT_SUPPORTED;
  }

  void DisplayCaptureImplSetDisplayCaptureInterface(
      const display_capture_interface_protocol_t* intf);

  zx_status_t DisplayCaptureImplImportImageForCapture(zx_unowned_handle_t collection,
                                                      uint32_t index, uint64_t* out_capture_handle);
  zx_status_t DisplayCaptureImplStartCapture(uint64_t capture_handle);
  zx_status_t DisplayCaptureImplReleaseCapture(uint64_t capture_handle);
  bool DisplayCaptureImplIsCaptureCompleted() {
    fbl::AutoLock lock(&display_lock_);
    return (capture_active_id_ == INVALID_ID);
  }

  // Required functions for DeviceType
  void DdkUnbindNew(ddk::UnbindTxn txn);
  void DdkRelease();
  zx_status_t DdkGetProtocol(uint32_t proto_id, void* out_protocol);

  zx_paddr_t GetNextFakePaddr() { return (++fake_image_paddr_ << 2); }

  const display_controller_impl_protocol_t* dcimpl_proto() const { return &dcimpl_proto_; }
  void SendVsync();

 private:
  enum {
    COMPONENT_PDEV,
    COMPONENT_SYSMEM,
    COMPONENT_COUNT,
  };
  zx_device_t* components_[COMPONENT_COUNT];

  zx_status_t SetupDisplayInterface();
  int VSyncThread();
  void PopulateAddedDisplayArgs(added_display_args_t* args);

  display_controller_impl_protocol_t dcimpl_proto_ = {};
  pdev_protocol_t pdev_ = {};
  sysmem_protocol_t sysmem_ = {};

  std::atomic_bool vsync_shutdown_flag_ = false;

  // Thread handles
  thrd_t vsync_thread_;

  // Locks used by the display driver
  fbl::Mutex display_lock_;  // general display state (i.e. display_id)
  fbl::Mutex image_lock_;    // used for accessing imported_images

  // The ID for currently active capture
  uint64_t capture_active_id_ TA_GUARDED(display_lock_);

  // Imported Images
  fbl::DoublyLinkedList<std::unique_ptr<ImageInfo>> imported_images_ TA_GUARDED(image_lock_);
  fbl::DoublyLinkedList<std::unique_ptr<ImageInfo>> imported_captures_ TA_GUARDED(display_lock_);

  uint64_t current_image_ TA_GUARDED(display_lock_);
  bool current_image_valid_ TA_GUARDED(display_lock_);

  // Fake addresses are stored in ImageInfo (which is used when importing an image for
  // for either display or capture.
  zx_paddr_t fake_image_paddr_ = 0;

  // Capture complete is signaled at vsync time. This counter introduces a bit of delay
  // for signal capture complete
  uint64_t capture_complete_signal_count_ = 0;

  // Display controller related data
  ddk::DisplayControllerInterfaceProtocolClient dc_intf_ TA_GUARDED(display_lock_);

  // Display Capture interface protocol
  ddk::DisplayCaptureInterfaceProtocolClient capture_intf_ TA_GUARDED(display_lock_);
};

}  // namespace fake_display

#endif  // ZIRCON_SYSTEM_DEV_DISPLAY_FAKE_FAKE_DISPLAY_H_
