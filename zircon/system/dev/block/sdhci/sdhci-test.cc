// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sdhci.h"

#include <atomic>
#include <memory>
#include <optional>

#include <lib/fake_ddk/fake_ddk.h>
#include <mock/ddktl/protocol/sdhci.h>
#include <zxtest/zxtest.h>

namespace sdhci {

class TestSdhci : public Sdhci {
 public:
  TestSdhci(zx_device_t* parent, ddk::MmioBuffer regs_mmio_buffer,
            const ddk::SdhciProtocolClient sdhci)
      : Sdhci(parent, std::move(regs_mmio_buffer), {}, {}, sdhci) {}

  zx_status_t SdmmcRequest(sdmmc_req_t* req) {
    blocks_remaining_ = req->blockcount;
    current_block_ = 0;
    return Sdhci::SdmmcRequest(req);
  }

  void DdkUnbindNew(ddk::UnbindTxn txn) {
    run_thread_ = false;
    Sdhci::DdkUnbindNew(std::move(txn));
  }

 protected:
  zx_status_t WaitForReset(const SoftwareReset mask) const override { return ZX_OK; }

  zx_status_t WaitForInterrupt() override {
    auto status = InterruptStatus::Get().FromValue(0).WriteTo(&regs_mmio_buffer_);

    while (run_thread_) {
      switch (GetRequestStatus()) {
        case RequestStatus::COMMAND:
          status.set_command_complete(1).WriteTo(&regs_mmio_buffer_);
          return ZX_OK;
        case RequestStatus::TRANSFER_DATA_DMA:
          status.set_transfer_complete(1).WriteTo(&regs_mmio_buffer_);
          return ZX_OK;
        case RequestStatus::READ_DATA_PIO:
          if (++current_block_ == blocks_remaining_) {
            status.set_buffer_read_ready(1).set_transfer_complete(1).WriteTo(&regs_mmio_buffer_);
          } else {
            status.set_buffer_read_ready(1).WriteTo(&regs_mmio_buffer_);
          }
          return ZX_OK;
        case RequestStatus::WRITE_DATA_PIO:
          if (++current_block_ == blocks_remaining_) {
            status.set_buffer_write_ready(1).set_transfer_complete(1).WriteTo(&regs_mmio_buffer_);
          } else {
            status.set_buffer_write_ready(1).WriteTo(&regs_mmio_buffer_);
          }
          return ZX_OK;
        default:
          break;
      }
    }

    return ZX_ERR_CANCELED;
  }

 private:
  std::atomic<bool> run_thread_ = true;
  std::atomic<uint16_t> blocks_remaining_ = 0;
  std::atomic<uint16_t> current_block_ = 0;
};

class SdhciTest : public zxtest::Test {
 public:
  SdhciTest()
      : registers_(new uint8_t[kRegisterSetSize]),
        mmio_(
            {
                .vaddr = registers_.get(),
                .offset = 0,
                .size = kRegisterSetSize,
                .vmo = ZX_HANDLE_INVALID,
            },
            0) {}

  void SetUp() override { ASSERT_TRUE(registers_); }

 protected:
  void CreateDut() {
    memset(registers_.get(), 0, kRegisterSetSize);

    dut_.emplace(fake_ddk::kFakeParent, ddk::MmioView(mmio_),
                 ddk::SdhciProtocolClient(mock_sdhci_.GetProto()));

    HostControllerVersion::Get()
        .FromValue(0)
        .set_specification_version(HostControllerVersion::kSpecificationVersion300)
        .WriteTo(&mmio_);
    ClockControl::Get().FromValue(0).set_internal_clock_stable(1).WriteTo(&mmio_);
  }

  std::unique_ptr<uint8_t[]> registers_;
  ddk::MockSdhci mock_sdhci_;
  zx::interrupt irq_;
  std::optional<TestSdhci> dut_;
  ddk::MmioView mmio_;
};

TEST_F(SdhciTest, DdkLifecycle) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  EXPECT_OK(dut_->Init());

  fake_ddk::Bind bind;
  dut_->DdkAdd("sdhci");
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  EXPECT_TRUE(bind.Ok());
}

TEST_F(SdhciTest, BaseClockZero) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(0);
  EXPECT_NOT_OK(dut_->Init());
}

TEST_F(SdhciTest, BaseClockFromDriver) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(0xabcdef);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  EXPECT_EQ(dut_->base_clock(), 0xabcdef);
}

TEST_F(SdhciTest, BaseClockFromHardware) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  Capabilities0::Get().FromValue(0).set_base_clock_frequency(104).WriteTo(&mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  EXPECT_EQ(dut_->base_clock(), 104'000'000);
}

TEST_F(SdhciTest, HostInfo) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  Capabilities0::Get()
      .FromValue(0)
      .set_base_clock_frequency(1)
      .set_bus_width_8_support(1)
      .set_voltage_3v3_support(1)
      .set_v3_64_bit_system_address_support(1)
      .WriteTo(&mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  sdmmc_host_info_t host_info = {};
  EXPECT_OK(dut_->SdmmcHostInfo(&host_info));
  EXPECT_EQ(host_info.caps, SDMMC_HOST_CAP_BUS_WIDTH_8 | SDMMC_HOST_CAP_SIXTY_FOUR_BIT |
                                SDMMC_HOST_CAP_VOLTAGE_330 | SDMMC_HOST_CAP_AUTO_CMD12);
  EXPECT_EQ(host_info.prefs, 0);
}

TEST_F(SdhciTest, HostInfoNoDma) {
  mock_sdhci_.ExpectGetQuirks(SDHCI_QUIRK_NO_DMA);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  Capabilities0::Get()
      .FromValue(0)
      .set_base_clock_frequency(1)
      .set_bus_width_8_support(1)
      .set_voltage_3v3_support(1)
      .set_v3_64_bit_system_address_support(1)
      .WriteTo(&mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  sdmmc_host_info_t host_info = {};
  EXPECT_OK(dut_->SdmmcHostInfo(&host_info));
  EXPECT_EQ(host_info.caps,
            SDMMC_HOST_CAP_BUS_WIDTH_8 | SDMMC_HOST_CAP_VOLTAGE_330 | SDMMC_HOST_CAP_AUTO_CMD12);
  EXPECT_EQ(host_info.prefs, 0);
}

TEST_F(SdhciTest, HostInfoNoTuning) {
  mock_sdhci_.ExpectGetQuirks(SDHCI_QUIRK_NON_STANDARD_TUNING);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  Capabilities0::Get().FromValue(0).set_base_clock_frequency(1).WriteTo(&mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  sdmmc_host_info_t host_info = {};
  EXPECT_OK(dut_->SdmmcHostInfo(&host_info));
  EXPECT_EQ(host_info.caps, SDMMC_HOST_CAP_AUTO_CMD12);
  EXPECT_EQ(host_info.prefs, SDMMC_HOST_PREFS_DISABLE_HS400 | SDMMC_HOST_PREFS_DISABLE_HS200);
}

TEST_F(SdhciTest, SetSignalVoltage) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  Capabilities0::Get().FromValue(0).set_voltage_3v3_support(1).set_voltage_1v8_support(1).WriteTo(
      &mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  PresentState::Get().FromValue(0).set_dat_3_0(0b0001).WriteTo(&mmio_);

  PowerControl::Get()
      .FromValue(0)
      .set_sd_bus_voltage_vdd1(PowerControl::kBusVoltage1V8)
      .set_sd_bus_power_vdd1(1)
      .WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcSetSignalVoltage(SDMMC_VOLTAGE_V180));
  EXPECT_TRUE(HostControl2::Get().ReadFrom(&mmio_).voltage_1v8_signalling_enable());

  PowerControl::Get()
      .FromValue(0)
      .set_sd_bus_voltage_vdd1(PowerControl::kBusVoltage3V3)
      .set_sd_bus_power_vdd1(1)
      .WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcSetSignalVoltage(SDMMC_VOLTAGE_V330));
  EXPECT_FALSE(HostControl2::Get().ReadFrom(&mmio_).voltage_1v8_signalling_enable());
}

TEST_F(SdhciTest, SetSignalVoltageFail) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  Capabilities0::Get().FromValue(0).set_voltage_1v8_support(1).WriteTo(&mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  PresentState::Get().FromValue(0).set_dat_3_0(0b0001).WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcSetSignalVoltage(SDMMC_VOLTAGE_V180));

  PresentState::Get().FromValue(0).set_dat_3_0(0b0000).WriteTo(&mmio_);
  EXPECT_NOT_OK(dut_->SdmmcSetSignalVoltage(SDMMC_VOLTAGE_V180));
}

TEST_F(SdhciTest, SetSignalVoltageUnsupported) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  EXPECT_NOT_OK(dut_->SdmmcSetSignalVoltage(SDMMC_VOLTAGE_V330));
}

TEST_F(SdhciTest, SetBusWidth) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  Capabilities0::Get().FromValue(0).set_bus_width_8_support(1).WriteTo(&mmio_);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  auto ctrl1 = HostControl1::Get().FromValue(0);

  EXPECT_OK(dut_->SdmmcSetBusWidth(SDMMC_BUS_WIDTH_EIGHT));
  EXPECT_TRUE(ctrl1.ReadFrom(&mmio_).extended_data_transfer_width());
  EXPECT_FALSE(ctrl1.ReadFrom(&mmio_).data_transfer_width_4bit());

  EXPECT_OK(dut_->SdmmcSetBusWidth(SDMMC_BUS_WIDTH_ONE));
  EXPECT_FALSE(ctrl1.ReadFrom(&mmio_).extended_data_transfer_width());
  EXPECT_FALSE(ctrl1.ReadFrom(&mmio_).data_transfer_width_4bit());

  EXPECT_OK(dut_->SdmmcSetBusWidth(SDMMC_BUS_WIDTH_FOUR));
  EXPECT_FALSE(ctrl1.ReadFrom(&mmio_).extended_data_transfer_width());
  EXPECT_TRUE(ctrl1.ReadFrom(&mmio_).data_transfer_width_4bit());
}

TEST_F(SdhciTest, SetBusWidthNotSupported) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  EXPECT_NOT_OK(dut_->SdmmcSetBusWidth(SDMMC_BUS_WIDTH_EIGHT));
}

TEST_F(SdhciTest, SetBusFreq) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  auto clock = ClockControl::Get().FromValue(0);

  EXPECT_OK(dut_->SdmmcSetBusFreq(12'500'000));
  EXPECT_EQ(clock.ReadFrom(&mmio_).frequency_select(), 4);
  EXPECT_TRUE(clock.sd_clock_enable());

  EXPECT_OK(dut_->SdmmcSetBusFreq(65'190));
  EXPECT_EQ(clock.ReadFrom(&mmio_).frequency_select(), 767);
  EXPECT_TRUE(clock.sd_clock_enable());

  EXPECT_OK(dut_->SdmmcSetBusFreq(100'000'000));
  EXPECT_EQ(clock.ReadFrom(&mmio_).frequency_select(), 0);
  EXPECT_TRUE(clock.sd_clock_enable());

  EXPECT_OK(dut_->SdmmcSetBusFreq(26'000'000));
  EXPECT_EQ(clock.ReadFrom(&mmio_).frequency_select(), 2);
  EXPECT_TRUE(clock.sd_clock_enable());
}

TEST_F(SdhciTest, SetBusFreqTimeout) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  ClockControl::Get().FromValue(0).set_internal_clock_stable(1).WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcSetBusFreq(12'500'000));

  ClockControl::Get().FromValue(0).WriteTo(&mmio_);
  EXPECT_NOT_OK(dut_->SdmmcSetBusFreq(12'500'000));
}

TEST_F(SdhciTest, SetBusFreqInternalClockEnable) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  EXPECT_OK(dut_->Init());
  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));

  ClockControl::Get()
      .FromValue(0)
      .set_internal_clock_stable(1)
      .set_internal_clock_enable(0)
      .WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcSetBusFreq(12'500'000));
  EXPECT_TRUE(ClockControl::Get().ReadFrom(&mmio_).internal_clock_enable());
}

TEST_F(SdhciTest, SetTiming) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  EXPECT_OK(dut_->SdmmcSetTiming(SDMMC_TIMING_HS));
  EXPECT_TRUE(HostControl1::Get().ReadFrom(&mmio_).high_speed_enable());
  EXPECT_EQ(HostControl2::Get().ReadFrom(&mmio_).uhs_mode_select(), HostControl2::kUhsModeSdr25);

  EXPECT_OK(dut_->SdmmcSetTiming(SDMMC_TIMING_LEGACY));
  EXPECT_FALSE(HostControl1::Get().ReadFrom(&mmio_).high_speed_enable());
  EXPECT_EQ(HostControl2::Get().ReadFrom(&mmio_).uhs_mode_select(), HostControl2::kUhsModeSdr12);

  EXPECT_OK(dut_->SdmmcSetTiming(SDMMC_TIMING_HSDDR));
  EXPECT_TRUE(HostControl1::Get().ReadFrom(&mmio_).high_speed_enable());
  EXPECT_EQ(HostControl2::Get().ReadFrom(&mmio_).uhs_mode_select(), HostControl2::kUhsModeDdr50);

  EXPECT_OK(dut_->SdmmcSetTiming(SDMMC_TIMING_SDR25));
  EXPECT_TRUE(HostControl1::Get().ReadFrom(&mmio_).high_speed_enable());
  EXPECT_EQ(HostControl2::Get().ReadFrom(&mmio_).uhs_mode_select(), HostControl2::kUhsModeSdr25);

  EXPECT_OK(dut_->SdmmcSetTiming(SDMMC_TIMING_SDR12));
  EXPECT_TRUE(HostControl1::Get().ReadFrom(&mmio_).high_speed_enable());
  EXPECT_EQ(HostControl2::Get().ReadFrom(&mmio_).uhs_mode_select(), HostControl2::kUhsModeSdr12);

  EXPECT_OK(dut_->SdmmcSetTiming(SDMMC_TIMING_HS400));
  EXPECT_TRUE(HostControl1::Get().ReadFrom(&mmio_).high_speed_enable());
  EXPECT_EQ(HostControl2::Get().ReadFrom(&mmio_).uhs_mode_select(), HostControl2::kUhsModeHs400);
}

TEST_F(SdhciTest, HwReset) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectHwReset();
  dut_->SdmmcHwReset();
  ASSERT_NO_FATAL_FAILURES(mock_sdhci_.VerifyAndClear());
}

TEST_F(SdhciTest, RequestCommandOnly) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  EXPECT_OK(dut_->Init());

  sdmmc_req_t request = {
      .cmd_idx = SDMMC_SEND_STATUS,
      .cmd_flags = SDMMC_SEND_STATUS_FLAGS,
      .arg = 0x7b7d9fbd,
      .blockcount = 0,
      .blocksize = 0,
      .use_dma = false,
      .dma_vmo = ZX_HANDLE_INVALID,
      .virt_buffer = nullptr,
      .virt_size = 0,
      .buf_offset = 0,
      .pmt = ZX_HANDLE_INVALID,
      .probe_tuning_cmd = 0,
      .response = {},
      .status = ZX_ERR_BAD_STATE,
  };

  Response::Get(0).FromValue(0xf3bbf2c0).WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcRequest(&request));

  auto command = Command::Get().FromValue(0);

  EXPECT_EQ(Argument::Get().ReadFrom(&mmio_).reg_value(), 0x7b7d9fbd);
  EXPECT_EQ(command.ReadFrom(&mmio_).command_index(), SDMMC_SEND_STATUS);
  EXPECT_EQ(command.command_type(), Command::kCommandTypeNormal);
  EXPECT_FALSE(command.data_present());
  EXPECT_TRUE(command.command_index_check());
  EXPECT_TRUE(command.command_crc_check());
  EXPECT_EQ(command.response_type(), Command::kResponseType48Bits);

  EXPECT_OK(request.status);
  EXPECT_EQ(request.response[0], 0xf3bbf2c0);

  request = {
      .cmd_idx = SDMMC_SEND_CSD,
      .cmd_flags = SDMMC_SEND_CSD_FLAGS,
      .arg = 0x9c1dc1ed,
      .blockcount = 0,
      .blocksize = 0,
      .use_dma = false,
      .dma_vmo = ZX_HANDLE_INVALID,
      .virt_buffer = nullptr,
      .virt_size = 0,
      .buf_offset = 0,
      .pmt = ZX_HANDLE_INVALID,
      .probe_tuning_cmd = 0,
      .response = {},
      .status = ZX_ERR_BAD_STATE,
  };

  Response::Get(0).FromValue(0x9f93b17d).WriteTo(&mmio_);
  Response::Get(1).FromValue(0x89aaba9e).WriteTo(&mmio_);
  Response::Get(2).FromValue(0xc14b059e).WriteTo(&mmio_);
  Response::Get(3).FromValue(0x7329a9e3).WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcRequest(&request));

  EXPECT_EQ(Argument::Get().ReadFrom(&mmio_).reg_value(), 0x9c1dc1ed);
  EXPECT_EQ(command.ReadFrom(&mmio_).command_index(), SDMMC_SEND_CSD);
  EXPECT_EQ(command.command_type(), Command::kCommandTypeNormal);
  EXPECT_FALSE(command.data_present());
  EXPECT_TRUE(command.command_crc_check());
  EXPECT_EQ(command.response_type(), Command::kResponseType136Bits);

  EXPECT_OK(request.status);
  EXPECT_EQ(request.response[0], 0x9f93b17d);
  EXPECT_EQ(request.response[1], 0x89aaba9e);
  EXPECT_EQ(request.response[2], 0xc14b059e);
  EXPECT_EQ(request.response[3], 0x7329a9e3);

  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));
}

TEST_F(SdhciTest, RequestWithData) {
  mock_sdhci_.ExpectGetQuirks(0);
  ASSERT_NO_FATAL_FAILURES(CreateDut());

  mock_sdhci_.ExpectGetBaseClock(100'000'000);
  EXPECT_OK(dut_->Init());

  uint32_t buffer[16] = {
      // clang-format off
      0x178096fb, 0x27328a47, 0x3267ce33, 0x8fccdf57,
      0x84d24349, 0x68fd8e47, 0x6b7363a3, 0x5f9fb9b1,
      0xfa0263f0, 0x467731aa, 0xf1a95135, 0xe9e7ba6b,
      0x2112719a, 0x7ee23bad, 0xb4285417, 0x6db4a2d1,
      // clang-format on
  };

  sdmmc_req_t request = {
      .cmd_idx = SDMMC_WRITE_MULTIPLE_BLOCK,
      .cmd_flags = SDMMC_WRITE_MULTIPLE_BLOCK_FLAGS,
      .arg = 0xfc4e6f56,
      .blockcount = 4,
      .blocksize = 16,
      .use_dma = false,
      .dma_vmo = ZX_HANDLE_INVALID,
      .virt_buffer = buffer,
      .virt_size = 0,
      .buf_offset = 0,
      .pmt = ZX_HANDLE_INVALID,
      .probe_tuning_cmd = 0,
      .response = {},
      .status = ZX_ERR_BAD_STATE,
  };

  Response::Get(0).FromValue(0x4ea3f1f3).WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcRequest(&request));

  auto transfer_mode = TransferMode::Get().FromValue(0);
  auto command = Command::Get().FromValue(0);

  EXPECT_EQ(BlockSize::Get().ReadFrom(&mmio_).reg_value(), 16);
  EXPECT_EQ(BlockCount::Get().ReadFrom(&mmio_).reg_value(), 4);
  EXPECT_EQ(Argument::Get().ReadFrom(&mmio_).reg_value(), 0xfc4e6f56);

  EXPECT_TRUE(transfer_mode.ReadFrom(&mmio_).multi_block());
  EXPECT_FALSE(transfer_mode.read());
  EXPECT_EQ(transfer_mode.auto_cmd_enable(), TransferMode::kAutoCmd12);
  EXPECT_TRUE(transfer_mode.block_count_enable());
  EXPECT_FALSE(transfer_mode.dma_enable());

  EXPECT_EQ(command.ReadFrom(&mmio_).command_index(), SDMMC_WRITE_MULTIPLE_BLOCK);
  EXPECT_EQ(command.command_type(), Command::kCommandTypeNormal);
  EXPECT_TRUE(command.data_present());
  EXPECT_TRUE(command.command_index_check());
  EXPECT_TRUE(command.command_crc_check());
  EXPECT_EQ(command.response_type(), Command::kResponseType48Bits);

  EXPECT_EQ(BufferData::Get().ReadFrom(&mmio_).reg_value(), 0x6db4a2d1);

  EXPECT_OK(request.status);
  EXPECT_EQ(request.response[0], 0x4ea3f1f3);

  request = {
      .cmd_idx = SDMMC_READ_MULTIPLE_BLOCK,
      .cmd_flags = SDMMC_READ_MULTIPLE_BLOCK_FLAGS,
      .arg = 0x55c1c22c,
      .blockcount = 4,
      .blocksize = 16,
      .use_dma = false,
      .dma_vmo = ZX_HANDLE_INVALID,
      .virt_buffer = buffer,
      .virt_size = 0,
      .buf_offset = 0,
      .pmt = ZX_HANDLE_INVALID,
      .probe_tuning_cmd = 0,
      .response = {},
      .status = ZX_ERR_BAD_STATE,
  };

  Response::Get(0).FromValue(0xa5387c19).WriteTo(&mmio_);
  BufferData::Get().FromValue(0xe99dd637).WriteTo(&mmio_);
  EXPECT_OK(dut_->SdmmcRequest(&request));

  EXPECT_EQ(BlockSize::Get().ReadFrom(&mmio_).reg_value(), 16);
  EXPECT_EQ(BlockCount::Get().ReadFrom(&mmio_).reg_value(), 4);
  EXPECT_EQ(Argument::Get().ReadFrom(&mmio_).reg_value(), 0x55c1c22c);

  EXPECT_TRUE(transfer_mode.ReadFrom(&mmio_).multi_block());
  EXPECT_TRUE(transfer_mode.read());
  EXPECT_EQ(transfer_mode.auto_cmd_enable(), TransferMode::kAutoCmd12);
  EXPECT_TRUE(transfer_mode.block_count_enable());
  EXPECT_FALSE(transfer_mode.dma_enable());

  EXPECT_EQ(command.ReadFrom(&mmio_).command_index(), SDMMC_READ_MULTIPLE_BLOCK);
  EXPECT_EQ(command.command_type(), Command::kCommandTypeNormal);
  EXPECT_TRUE(command.data_present());
  EXPECT_TRUE(command.command_index_check());
  EXPECT_TRUE(command.command_crc_check());
  EXPECT_EQ(command.response_type(), Command::kResponseType48Bits);

  EXPECT_OK(request.status);
  EXPECT_EQ(request.response[0], 0xa5387c19);

  for (uint32_t i = 0; i < 16; i++) {
    EXPECT_EQ(buffer[i], 0xe99dd637);
  }

  dut_->DdkUnbindNew(ddk::UnbindTxn(fake_ddk::kFakeDevice));
}

}  // namespace sdhci
