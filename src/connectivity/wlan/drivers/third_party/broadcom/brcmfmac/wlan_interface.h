// Copyright (c) 2019 The Fuchsia Authors
//
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without
// fee is hereby granted, provided that the above copyright notice and this permission notice
// appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
// SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
#ifndef SRC_CONNECTIVITY_WLAN_DRIVERS_THIRD_PARTY_BROADCOM_BRCMFMAC_WLAN_INTERFACE_H_
#define SRC_CONNECTIVITY_WLAN_DRIVERS_THIRD_PARTY_BROADCOM_BRCMFMAC_WLAN_INTERFACE_H_

#include <zircon/types.h>

#include <memory>

#include <ddk/device.h>

#include "src/connectivity/wlan/drivers/third_party/broadcom/brcmfmac/core.h"

struct wireless_dev;

namespace wlan {
namespace brcmfmac {

class Device;

class WlanInterface {
 public:
  // Static factory function.  The returned instance is unowned, since its lifecycle is managed by
  // the devhost.
  static zx_status_t Create(Device* device, const char* name, wireless_dev* wdev,
                            WlanInterface** out_interface);

  // Accessors.
  zx_device_t* zxdev();
  const zx_device_t* zxdev() const;
  wireless_dev* wdev();
  const wireless_dev* wdev() const;

  // Device operations.
  void DdkAsyncRemove();
  void DdkRelease();

  static zx_status_t Query(brcmf_pub* drvr, wlanphy_impl_info_t* out_info);
  static zx_status_t SetCountry(const wlanphy_country_t* country);

  // ZX_PROTOCOL_WLANIF_IMPL operations.
  zx_status_t Start(const wlanif_impl_ifc_protocol_t* ifc, zx_handle_t* out_sme_channel);
  void Stop();
  void Query(wlanif_query_info_t* info);
  void StartScan(const wlanif_scan_req_t* req);
  void JoinReq(const wlanif_join_req_t* req);
  void AuthReq(const wlanif_auth_req_t* req);
  void AuthResp(const wlanif_auth_resp_t* resp);
  void DeauthReq(const wlanif_deauth_req_t* req);
  void AssocReq(const wlanif_assoc_req_t* req);
  void AssocResp(const wlanif_assoc_resp_t* resp);
  void DisassocReq(const wlanif_disassoc_req_t* req);
  void ResetReq(const wlanif_reset_req_t* req);
  void StartReq(const wlanif_start_req_t* req);
  void StopReq(const wlanif_stop_req_t* req);
  void SetKeysReq(const wlanif_set_keys_req_t* req);
  void DelKeysReq(const wlanif_del_keys_req_t* req);
  void EapolReq(const wlanif_eapol_req_t* req);
  void StatsQueryReq();
  void StartCaptureFrames(const wlanif_start_capture_frames_req_t* req,
                          wlanif_start_capture_frames_resp_t* resp);
  void StopCaptureFrames();
  zx_status_t SetMulticastPromisc(bool enable);
  void DataQueueTx(uint32_t options, ethernet_netbuf_t* netbuf,
                   ethernet_impl_queue_tx_callback completion_cb, void* cookie);

 private:
  WlanInterface();
  ~WlanInterface();
  zx_device_t* zx_device_;
  wireless_dev* wdev_;
  Device* device_;
};
}  // namespace brcmfmac
}  // namespace wlan
#endif  // SRC_CONNECTIVITY_WLAN_DRIVERS_THIRD_PARTY_BROADCOM_BRCMFMAC_WLAN_INTERFACE_H_
