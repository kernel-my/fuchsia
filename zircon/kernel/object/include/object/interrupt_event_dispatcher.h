// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#ifndef ZIRCON_KERNEL_OBJECT_INCLUDE_OBJECT_INTERRUPT_EVENT_DISPATCHER_H_
#define ZIRCON_KERNEL_OBJECT_INCLUDE_OBJECT_INTERRUPT_EVENT_DISPATCHER_H_

#include <sys/types.h>
#include <zircon/types.h>

#include <fbl/canary.h>
#include <fbl/intrusive_single_list.h>
#include <kernel/mp.h>
#include <ktl/unique_ptr.h>
#include <object/handle.h>
#include <object/interrupt_dispatcher.h>

class InterruptEventDispatcher final : public InterruptDispatcher {
 public:
  static zx_status_t Create(KernelHandle<InterruptDispatcher>* handle, zx_rights_t* rights,
                            uint32_t vector, uint32_t options);

  ~InterruptEventDispatcher() final;

  InterruptEventDispatcher(const InterruptDispatcher&) = delete;
  InterruptEventDispatcher& operator=(const InterruptDispatcher&) = delete;

  zx_status_t BindVcpu(fbl::RefPtr<VcpuDispatcher> vcpu_dispatcher) final;

 private:
  explicit InterruptEventDispatcher(uint32_t vector);

  void MaskInterrupt() final;
  void UnmaskInterrupt() final;
  void UnregisterInterruptHandler() final;
  bool HasVcpu() const final;

  zx_status_t RegisterInterruptHandler();
  static interrupt_eoi IrqHandler(void* ctx);
  static interrupt_eoi VcpuIrqHandler(void* ctx);
  void VcpuInterruptHandler();

  const uint32_t vector_;

  struct VcpuDispatcherNode
      : public fbl::SinglyLinkedListable<ktl::unique_ptr<VcpuDispatcherNode>> {
    VcpuDispatcherNode(fbl::RefPtr<VcpuDispatcher> vcpu) : vcpu_(vcpu) {}
    fbl::RefPtr<VcpuDispatcher> vcpu_;
  };
  fbl::SinglyLinkedList<ktl::unique_ptr<VcpuDispatcherNode>> vcpus_;
};

#endif  // ZIRCON_KERNEL_OBJECT_INCLUDE_OBJECT_INTERRUPT_EVENT_DISPATCHER_H_
