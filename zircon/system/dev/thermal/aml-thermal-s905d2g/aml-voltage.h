// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_THERMAL_AML_THERMAL_S905D2G_AML_VOLTAGE_H_
#define ZIRCON_SYSTEM_DEV_THERMAL_AML_THERMAL_S905D2G_AML_VOLTAGE_H_

#include <memory>

#include <ddktl/protocol/composite.h>
#include <soc/aml-common/aml-thermal.h>

#include "aml-pwm.h"
#include "lib/mmio/mmio.h"
#include "zircon/errors.h"
#include "zircon/types.h"

namespace thermal {
// This class represents a voltage regulator
// on the Amlogic board which provides interface
// to set and get current voltage for the CPU.
class AmlVoltageRegulator {
 public:
  DISALLOW_COPY_AND_ASSIGN_ALLOW_MOVE(AmlVoltageRegulator);
  AmlVoltageRegulator() = default;
  zx_status_t Create(zx_device_t* parent, aml_voltage_table_info_t* voltage_table_info);
  // For testing
  zx_status_t Init(ddk::MmioBuffer pwm_AO_D_mmio, ddk::MmioBuffer pwm_A_mmio, uint32_t pid,
                   aml_voltage_table_info_t* voltage_table_info);
  zx_status_t Init(aml_voltage_table_info_t* voltage_table_info);
  uint32_t GetVoltage(fuchsia_hardware_thermal_PowerDomain power_domain) {
    if (power_domain == fuchsia_hardware_thermal_PowerDomain_BIG_CLUSTER_POWER_DOMAIN) {
      return GetBigClusterVoltage();
    }
    return GetLittleClusterVoltage();
  }

  zx_status_t SetVoltage(fuchsia_hardware_thermal_PowerDomain power_domain, uint32_t microvolt) {
    if (power_domain == fuchsia_hardware_thermal_PowerDomain_BIG_CLUSTER_POWER_DOMAIN) {
      return SetBigClusterVoltage(microvolt);
    }
    return SetLittleClusterVoltage(microvolt);
  }

 private:
  uint32_t GetBigClusterVoltage();
  uint32_t GetLittleClusterVoltage();
  zx_status_t SetClusterVoltage(int* current_voltage_index, std::unique_ptr<thermal::AmlPwm>* pwm,
                                uint32_t microvolt);
  zx_status_t SetBigClusterVoltage(uint32_t microvolt) {
    if (pid_ == PDEV_PID_AMLOGIC_S905D2) {
      // Astro
      return SetClusterVoltage(&current_big_cluster_voltage_index_, &pwm_AO_D_, microvolt);
    }
    if (pid_ == PDEV_PID_AMLOGIC_T931) {
      // Sherlock
      return SetClusterVoltage(&current_big_cluster_voltage_index_, &pwm_A_, microvolt);
    }
    zxlogf(ERROR, "aml-cpufreq: unsupported SOC PID %u\n", pid_);
    return ZX_ERR_INVALID_ARGS;
  }
  zx_status_t SetLittleClusterVoltage(uint32_t microvolt) {
    return SetClusterVoltage(&current_little_cluster_voltage_index_, &pwm_AO_D_, microvolt);
  }

  std::unique_ptr<thermal::AmlPwm> pwm_A_;
  std::unique_ptr<thermal::AmlPwm> pwm_AO_D_;
  aml_voltage_table_info_t voltage_table_info_;
  int current_big_cluster_voltage_index_;
  int current_little_cluster_voltage_index_;
  uint32_t pid_;
};
}  // namespace thermal

#endif  // ZIRCON_SYSTEM_DEV_THERMAL_AML_THERMAL_S905D2G_AML_VOLTAGE_H_
