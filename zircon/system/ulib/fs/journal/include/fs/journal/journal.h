// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FS_JOURNAL_JOURNAL_H_
#define FS_JOURNAL_JOURNAL_H_

#include <lib/fit/barrier.h>
#include <lib/fit/promise.h>
#include <lib/fit/sequencer.h>
#include <zircon/status.h>
#include <zircon/types.h>

#include <algorithm>

#include <fbl/vector.h>
#include <fs/journal/background_executor.h>
#include <fs/journal/format.h>
#include <fs/journal/internal/journal_writer.h>
#include <fs/journal/superblock.h>
#include <fs/transaction/block_transaction.h>
#include <storage/buffer/blocking-ring-buffer.h>
#include <storage/buffer/ring-buffer.h>
#include <storage/operation/buffered-operation.h>

namespace fs {

// This class implements an interface for filesystems to write back data to the underlying
// device. It provides methods for the following functionality:
// - Writing data to the underlying device
// - Writing metadata to the underlying device (journaled or unjournaled)
// - Revoking metadata from the journal
//
// The journal operates on asynchronous objects: it returns promises corresponding to
// each operation, which may be chained together by the caller, and which may be completed
// by scheduling these promises on the journal's executor via |journal.schedule_task|.
//
// EXAMPLE USAGE
//
//      Journal journal(...);
//      auto data_promise = journal.WriteData(vnode_data);
//      auto metadata_promise = journal.WriteMetadata(vnode_metadata);
//      journal.schedule_task(data_promise.and_then(metadata_promise));
//
//      // A few moments later...
//
//      journal.schedule_task(Sync().and_then([]() {
//          printf("Operation completed successfully!");
//      }));
//
// This class is thread-safe.
class Journal final : public fit::executor {
 public:
  using Promise = fit::promise<void, zx_status_t>;

  // Constructs a Journal with journaling enabled. This is the traditional constructor
  // of Journals, where data and metadata are treated separately.
  //
  // |journal_superblock| represents the journal info block.
  // |journal_buffer| must be the size of the entries (not including the info block).
  // |journal_start_block| must point to the start of the journal info block.
  Journal(fs::TransactionHandler* transaction_handler, JournalSuperblock journal_superblock,
          std::unique_ptr<storage::BlockingRingBuffer> journal_buffer,
          std::unique_ptr<storage::BlockingRingBuffer> writeback_buffer,
          uint64_t journal_start_block);

  // Constructs a journal where metadata and data are both treated as data, effectively
  // disabling the journal.
  Journal(fs::TransactionHandler* transaction_handler,
          std::unique_ptr<storage::BlockingRingBuffer> writeback_buffer);

  ~Journal() final;

  // Transmits operations containing pure data, which may be subject to different atomicity
  // guarantees than metadata updates.
  //
  // Multiple requests to WriteData are not ordered. If ordering is desired, it should
  // be added using a |fit::sequencer| object, or by chaining the data writeback promise
  // along an object which is ordered.
  Promise WriteData(fbl::Vector<storage::UnbufferedOperation> operations);

  // Transmits operations contains metadata, which must be updated atomically with respect
  // to power failures if journaling is enabled.
  //
  // Multiple requests to WriteMetadata are ordered. They are ordered by the invocation
  // of the |WriteMetadata| method, not by the completion of the returned promise.
  Promise WriteMetadata(fbl::Vector<storage::UnbufferedOperation> operations);

  // Transmits operations containing trim requests, which must be ordered with respect
  // to metadata writes.
  //
  // Requests to TrimData are ordered with respect to WriteMetadata by the invocation
  // of the respective method, not by the completion of the returned promise.
  Promise TrimData(fbl::Vector<storage::BufferedOperation> operations);

  // Returns a promise which identifies that all previous promises returned from the journal
  // have completed (succeeded, failed, or abandoned).
  // Additionally, prompt the internal journal writer to update the info block, if it
  // isn't already up-to-date.
  //
  // This promise completes when the promises from all prior invocations of:
  // - WriteData
  // - WriteMetadata
  // - Sync
  // Have completed (either successfully or with an error).
  Promise Sync();

  // Schedules a promise to the journals background thread executor.
  void schedule_task(fit::pending_task task) final { executor_.schedule_task(std::move(task)); }

 private:
  std::unique_ptr<storage::BlockingRingBuffer> journal_buffer_;
  std::unique_ptr<storage::BlockingRingBuffer> writeback_buffer_;

  // To implement |Sync()|, the journal must track all pending work, with the ability
  // to react once all prior work (up to a point) has finished execution.
  // This barrier enables a journal to generate new promises identifying when all prior
  // tasks transmitted to |executor_| have completed.
  fit::barrier barrier_;

  // The journal must enforce the requirement that metadata operations are completed in
  // the order they are enqueued. To fulfill this requirement, a sequencer guarantees
  // ordering of internal promise structures before they are handed to |executor_|.
  fit::sequencer metadata_sequencer_;

  internal::JournalWriter writer_;

  // Intentionally place the executor at the end of the journal. This ensures that
  // during destruction, the executor can complete pending tasks operation on the writeback
  // buffers before the writeback buffers are destroyed.
  BackgroundExecutor executor_;
};

}  // namespace fs

#endif  // FS_JOURNAL_JOURNAL_H_
