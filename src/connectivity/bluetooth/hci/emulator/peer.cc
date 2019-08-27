// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peer.h"

#include <fuchsia/bluetooth/test/cpp/fidl.h>

#include "src/connectivity/bluetooth/core/bt-host/testing/fake_peer.h"
#include "src/connectivity/bluetooth/hci/emulator/log.h"

namespace fbt = fuchsia::bluetooth;
namespace ftest = fuchsia::bluetooth::test;

namespace bt_hci_emulator {
namespace {

bt::DeviceAddress::Type LeAddressTypeFromFidl(fbt::AddressType type) {
  return (type == fbt::AddressType::RANDOM) ? bt::DeviceAddress::Type::kLERandom
                                            : bt::DeviceAddress::Type::kLEPublic;
}

bt::DeviceAddress LeAddressFromFidl(const fbt::Address& address) {
  return bt::DeviceAddress(LeAddressTypeFromFidl(address.type), address.bytes);
}

}  // namespace

// static
Peer::Result Peer::NewLowEnergy(ftest::LowEnergyPeerParameters parameters,
                                fidl::InterfaceRequest<fuchsia::bluetooth::test::Peer> request,
                                fbl::RefPtr<bt::testing::FakeController> fake_controller) {
  ZX_DEBUG_ASSERT(request);
  ZX_DEBUG_ASSERT(fake_controller);

  if (!parameters.has_address()) {
    logf(ERROR, "A fake peer address is mandatory!\n");
    return fit::error(ftest::EmulatorPeerError::PARAMETERS_INVALID);
  }

  bt::BufferView adv, scan_response;
  if (parameters.has_advertisement()) {
    adv = bt::BufferView(parameters.advertisement().data);
  }
  if (parameters.has_scan_response()) {
    scan_response = bt::BufferView(parameters.scan_response().data);
  }

  auto address = LeAddressFromFidl(parameters.address());
  bool connectable = parameters.has_connectable() && parameters.connectable();
  bool scannable = scan_response.size() != 0u;

  // TODO(armansito): We should consider splitting bt::testing::FakePeer into separate types for
  // BR/EDR and LE transport emulation logic.
  auto peer = std::make_unique<bt::testing::FakePeer>(address, connectable, scannable);
  peer->SetAdvertisingData(adv);
  if (scannable) {
    peer->SetScanResponse(/*should_batch_reports=*/false, scan_response);
  }

  if (!fake_controller->AddPeer(std::move(peer))) {
    logf(ERROR, "A fake LE peer with given address already exists: %s\n",
         address.ToString().c_str());
    return fit::error(ftest::EmulatorPeerError::ADDRESS_REPEATED);
  }

  return fit::ok(
      std::unique_ptr<Peer>(new Peer(address, std::move(request), std::move(fake_controller))));
}

// static
Peer::Result Peer::NewBredr(ftest::BredrPeerParameters parameters,
                            fidl::InterfaceRequest<fuchsia::bluetooth::test::Peer> request,
                            fbl::RefPtr<bt::testing::FakeController> fake_controller) {
  ZX_DEBUG_ASSERT(request);
  ZX_DEBUG_ASSERT(fake_controller);

  if (!parameters.has_address()) {
    logf(ERROR, "A fake peer address is mandatory!\n");
    return fit::error(ftest::EmulatorPeerError::PARAMETERS_INVALID);
  }

  auto address = bt::DeviceAddress(bt::DeviceAddress::Type::kBREDR, parameters.address().bytes);
  bool connectable = parameters.has_connectable() && parameters.connectable();

  // TODO(armansito): We should consider splitting bt::testing::FakePeer into separate types for
  // BR/EDR and LE transport emulation logic.
  auto peer = std::make_unique<bt::testing::FakePeer>(address, connectable, false);
  if (parameters.has_device_class()) {
    peer->set_class_of_device(bt::DeviceClass(parameters.device_class().value));
  }

  if (!fake_controller->AddPeer(std::move(peer))) {
    logf(ERROR, "A fake BR/EDR peer with given address already exists: %s\n",
         address.ToString().c_str());
    return fit::error(ftest::EmulatorPeerError::ADDRESS_REPEATED);
  }

  return fit::ok(
      std::unique_ptr<Peer>(new Peer(address, std::move(request), std::move(fake_controller))));
}

Peer::Peer(bt::DeviceAddress address, fidl::InterfaceRequest<ftest::Peer> request,
           fbl::RefPtr<bt::testing::FakeController> fake_controller)
    : address_(address), fake_controller_(fake_controller), binding_(this, std::move(request)) {
  ZX_DEBUG_ASSERT(fake_controller_);
  ZX_DEBUG_ASSERT(binding_.is_bound());

  binding_.set_error_handler(fit::bind_member(this, &Peer::OnChannelClosed));
}

Peer::~Peer() { CleanUp(); }

void Peer::AssignConnectionStatus(ftest::HciError status, AssignConnectionStatusCallback callback) {
  logf(TRACE, "Peer.AssignConnectionStatus\n");

  auto peer = fake_controller_->FindPeer(address_);
  if (peer) {
    peer->set_connect_response(static_cast<bt::hci::StatusCode>(status));
  }

  callback();
}

void Peer::OnChannelClosed(zx_status_t status) {
  logf(TRACE, "Peer channel closed\n");
  if (closed_callback_) {
    closed_callback_();
  }
}

void Peer::CleanUp() { fake_controller_->RemovePeer(address_); }

}  // namespace bt_hci_emulator
