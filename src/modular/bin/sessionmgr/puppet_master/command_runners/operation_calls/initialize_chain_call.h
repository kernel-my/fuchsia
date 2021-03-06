// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_MODULAR_BIN_SESSIONMGR_PUPPET_MASTER_COMMAND_RUNNERS_OPERATION_CALLS_INITIALIZE_CHAIN_CALL_H_
#define SRC_MODULAR_BIN_SESSIONMGR_PUPPET_MASTER_COMMAND_RUNNERS_OPERATION_CALLS_INITIALIZE_CHAIN_CALL_H_

#include <fuchsia/modular/cpp/fidl.h>

#include "src/modular/bin/sessionmgr/storage/story_storage.h"
#include "src/modular/lib/async/cpp/operation.h"

namespace modular {

void AddInitializeChainOperation(
    OperationContainer* operation_container, StoryStorage* story_storage,
    std::vector<std::string> module_path,
    fuchsia::modular::CreateModuleParameterMapInfoPtr create_parameter_map_info,
    fit::function<void(fuchsia::modular::ExecuteResult, fuchsia::modular::ModuleParameterMapPtr)>
        result_call);

}  // namespace modular

#endif  // SRC_MODULAR_BIN_SESSIONMGR_PUPPET_MASTER_COMMAND_RUNNERS_OPERATION_CALLS_INITIALIZE_CHAIN_CALL_H_
