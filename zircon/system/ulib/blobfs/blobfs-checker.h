// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_ULIB_BLOBFS_BLOBFS_CHECKER_H_
#define ZIRCON_SYSTEM_ULIB_BLOBFS_BLOBFS_CHECKER_H_

#ifdef __Fuchsia__
#include "blobfs.h"
#else
#include <blobfs/host.h>
#endif

#include <memory>

namespace blobfs {

class BlobfsChecker {
 public:
  BlobfsChecker(std::unique_ptr<Blobfs> blobfs);

  zx_status_t Initialize(bool apply_journal);
  void TraverseInodeBitmap();
  void TraverseBlockBitmap();
  zx_status_t CheckAllocatedCounts() const;

 private:
  DISALLOW_COPY_ASSIGN_AND_MOVE(BlobfsChecker);
  std::unique_ptr<Blobfs> blobfs_;
  uint32_t alloc_inodes_ = 0;
  uint32_t alloc_blocks_ = 0;
  uint32_t error_blobs_ = 0;
  uint32_t inode_blocks_ = 0;
};

#ifdef __Fuchsia__
// Validate that the contents of the superblock matches the results claimed in the underlying
// volume manager.
//
// If the results are inconsistent, update the FVM's allocation accordingly.
zx_status_t CheckFvmConsistency(const Superblock* info, BlockDevice* device);
#endif  // __Fuchsia__

}  // namespace blobfs

#endif  // ZIRCON_SYSTEM_ULIB_BLOBFS_BLOBFS_CHECKER_H_
