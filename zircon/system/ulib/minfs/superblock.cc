// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/cksum.h>
#include <lib/fzl/owned-vmo-mapper.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
#include <utility>

#include <bitmap/raw-bitmap.h>
#include <minfs/superblock.h>

namespace minfs {

SuperblockManager::SuperblockManager(const Superblock* info, fzl::OwnedVmoMapper mapper)
    : mapping_(std::move(mapper)) {}

SuperblockManager::~SuperblockManager() = default;

// Static.
zx_status_t SuperblockManager::Create(block_client::BlockDevice* device, const Superblock* info,
                                      uint32_t max_blocks, IntegrityCheck checks,
                                      std::unique_ptr<SuperblockManager>* out) {
  zx_status_t status = ZX_OK;
  if (checks == IntegrityCheck::kAll) {
    status = CheckSuperblock(info, device, max_blocks);
    if (status != ZX_OK) {
      FS_TRACE_ERROR("SuperblockManager::Create failed to check info: %d\n", status);
      return status;
    }
  }

  fzl::OwnedVmoMapper mapper;
  // Create the info vmo
  if ((status = mapper.CreateAndMap(kMinfsBlockSize, "minfs-superblock")) != ZX_OK) {
    return status;
  }

  fuchsia_hardware_block_VmoID info_vmoid;
  if ((status = device->BlockAttachVmo(mapper.vmo(), &info_vmoid)) != ZX_OK) {
    return status;
  }
  memcpy(mapper.start(), info, sizeof(Superblock));

  auto sb = std::unique_ptr<SuperblockManager>(new SuperblockManager(info, std::move(mapper)));
  *out = std::move(sb);
  return ZX_OK;
}

void SuperblockManager::Write(PendingWork* transaction, UpdateBackupSuperblock write_backup) {
  UpdateChecksum(MutableInfo());
  auto data = mapping_.vmo().get();

  storage::Operation op = {
      .type = storage::OperationType::kWrite,
      .vmo_offset = 0,
      .dev_offset = kSuperblockStart,
      .length = 1,
  };
  transaction->EnqueueMetadata(data, std::move(op));

  if (write_backup == UpdateBackupSuperblock::kUpdate) {
    blk_t superblock_dev_offset = kNonFvmSuperblockBackup;

    if (MutableInfo()->flags & kMinfsFlagFVM) {
      superblock_dev_offset = kFvmSuperblockBackup;
    }

    storage::Operation op = {
        .type = storage::OperationType::kWrite,
        .vmo_offset = 0,
        .dev_offset = superblock_dev_offset,
        .length = 1,
    };
    transaction->EnqueueMetadata(data, std::move(op));
  }
}

}  // namespace minfs
