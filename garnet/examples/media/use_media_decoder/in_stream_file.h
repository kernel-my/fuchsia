// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GARNET_EXAMPLES_MEDIA_USE_MEDIA_DECODER_IN_STREAM_FILE_H_
#define GARNET_EXAMPLES_MEDIA_USE_MEDIA_DECODER_IN_STREAM_FILE_H_

#include <fstream>

#include "in_stream.h"
#include "util.h"

class InStreamFile : public InStream {
 public:
  explicit InStreamFile(async::Loop* fidl_loop, thrd_t fidl_thread,
                        sys::ComponentContext* component_context, std::string input_file_name);
  ~InStreamFile();

 private:
  zx_status_t ReadBytesInternal(uint32_t max_bytes_to_read, uint32_t* bytes_read_out,
                                uint8_t* buffer_out, zx::time deadline) override;

  std::string input_file_name_;
  std::ifstream file_;
};

#endif  // GARNET_EXAMPLES_MEDIA_USE_MEDIA_DECODER_IN_STREAM_FILE_H_
