// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/lib/fxl/logging.h"
#include "src/ui/lib/escher/defaults/default_shader_program_factory.h"
#include "src/ui/lib/escher/escher.h"
#include "src/ui/lib/escher/escher_process_init.h"
#include "src/ui/lib/escher/forward_declarations.h"
#include "src/ui/lib/escher/fs/hack_filesystem.h"
#include "src/ui/lib/escher/impl/glsl_compiler.h"
#include "src/ui/lib/escher/paper/paper_renderer_config.h"
#include "src/ui/lib/escher/paper/paper_renderer_static_config.h"
#include "src/ui/lib/escher/shaders/util/spirv_file_util.h"
#include "src/ui/lib/escher/vk/shader_program.h"

#include "third_party/shaderc/libshaderc/include/shaderc/shaderc.hpp"

namespace escher {

// Compiles all of the provided shader modules and writes out their spirv
// to disk in the source tree.
bool CompileAndWriteShader(HackFilesystemPtr filesystem, ShaderProgramData program_data) {
  std::string abs_root = *filesystem->base_path() + "/shaders/spirv/";

  static auto compiler = std::make_unique<shaderc::Compiler>();

  // Loop over all the shader stages.
  for (const auto& iter : program_data.source_files) {
    // Skip if path is empty.
    if (iter.second.length() == 0) {
      continue;
    }

    FXL_LOG(INFO) << "Processing shader " << iter.second;

    auto shader = fxl::MakeRefCounted<ShaderModuleTemplate>(vk::Device(), compiler.get(),
                                                            iter.first, iter.second, filesystem);

    std::vector<uint32_t> spirv;
    if (!shader->CompileVariantToSpirv(program_data.args, &spirv)) {
      FXL_LOG(ERROR) << "could not compile shader " << iter.second;
      return false;
    }

    if (!shader_util::WriteSpirvToDisk(spirv, program_data.args, abs_root, iter.second)) {
      FXL_LOG(ERROR) << "could not write shader " << iter.second << " to disk.";
      return false;
    }
  }
  return true;
}

}  // namespace escher

// Program entry point.
int main(int argc, const char** argv) {
  // Register all the shader files, along with include files, that are used by Escher.
  auto filesystem = escher::HackFilesystem::New();

  // The binary for this is expected to be in ./out/default/host_x64.
  bool success = filesystem->InitializeWithRealFiles(escher::kPaperRendererShaderPaths,
                                                     "./../../../../src/ui/lib/escher/");
  FXL_CHECK(success);
  FXL_CHECK(filesystem->base_path());

  // Ambient light program.
  if (!CompileAndWriteShader(filesystem, escher::kAmbientLightProgramData)) {
    return EXIT_FAILURE;
  }

  // No lighting program.
  if (!CompileAndWriteShader(filesystem, escher::kNoLightingProgramData)) {
    return EXIT_FAILURE;
  }

  if (!CompileAndWriteShader(filesystem, escher::kPointLightProgramData)) {
    return EXIT_FAILURE;
  }

  if (!CompileAndWriteShader(filesystem, escher::kPointLightFalloffProgramData)) {
    return EXIT_FAILURE;
  }

  if (!CompileAndWriteShader(filesystem, escher::kShadowVolumeGeometryProgramData)) {
    return EXIT_FAILURE;
  }

  if (!CompileAndWriteShader(filesystem, escher::kShadowVolumeGeometryDebugProgramData)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
