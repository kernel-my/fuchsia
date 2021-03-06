// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZIRCON_SYSTEM_DEV_BLOCK_CORE_MANAGER_H_
#define ZIRCON_SYSTEM_DEV_BLOCK_CORE_MANAGER_H_

#include <lib/zx/fifo.h>
#include <lib/zx/vmo.h>
#include <threads.h>
#include <zircon/types.h>

#include <atomic>

#include <ddktl/protocol/block.h>

#include "server.h"

// Manager controls the state of a background thread (or threads) servicing Fifo
// requests.
class Manager {
 public:
  DISALLOW_COPY_ASSIGN_AND_MOVE(Manager);

  Manager();
  ~Manager();

  // Launches the Fifo server in a background thread.
  //
  // Returns an error if the block server cannot be created.
  // Returns an error if the Fifo server is already running.
  zx_status_t StartServer(ddk::BlockProtocolClient* protocol, zx::fifo* out_fifo);

  // Ensures the FIFO server has terminated.
  //
  // When this function returns, it is guaranteed that the next call to |StartServer()|
  // won't see an already running Fifo server.
  zx_status_t CloseFifoServer();

  // Attaches a VMO to the currently executing server, if one is running.
  //
  // Returns an error if a server is not currently running.
  zx_status_t AttachVmo(zx::vmo vmo, vmoid_t* out_vmoid);

 private:
  enum class ThreadState : uint32_t {
    // No server is currently executing.
    None,
    // The server is executing right now.
    Running,
    // The server has finished executing, and is ready to be joined.
    Joinable,
  };

  // Queries if the Fifo Server is running, possibly cleaning up the old server's
  // thread if one exists.
  bool IsFifoServerRunning();

  // Joins the completed server thread and clean up all resources it may have used.
  void JoinServer();

  // Frees the Fifo server, cleaning up "server_" and setting the thread state to none.
  //
  // Precondition: No background thread is executing.
  void FreeServer();

  // Runs the server until |blockserver_shutdown| is invoked on |server_|, or the client
  // closes their end of the Fifo.
  static int RunServer(void* arg);

  ThreadState GetState() const { return state_.load(); }

  void SetState(ThreadState state) { state_.store(state); }

  thrd_t thread_;
  std::atomic<ThreadState> state_ = ThreadState::None;
  Server* server_ = nullptr;
};

#endif  // ZIRCON_SYSTEM_DEV_BLOCK_CORE_MANAGER_H_
