// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hotsort_vk_target_requirements.h"

#include <string.h>

#include "common/macros.h"
#include "common/vk/shader_info_amd.h"
#include "hotsort_vk.h"
#include "hotsort_vk_target.h"

//
// TARGET PROPERTIES: VULKAN
//
// Yields the extensions and features required by a HotSort target.
//
// If .ext_names is NULL then the respective count will be initialized.
//
// It is an error to provide a count that is too small.
//

//
// FIXME(allanmac): STOP USING float64
//

bool
hotsort_vk_target_get_requirements(struct hotsort_vk_target const * const        target,
                                   struct hotsort_vk_target_requirements * const requirements)
{
  //
  //
  //
  if ((target == NULL) || (requirements == NULL))
    {
      return false;
    }

  //
  // REQUIRED EXTENSIONS
  //
  {
    //
    // compute number of required extensions
    //
    uint32_t ext_count = 0;

    for (uint32_t ii = 0; ii < ARRAY_LENGTH_MACRO(target->config.extensions.bitmap); ii++)
      {
        ext_count += __builtin_popcount(target->config.extensions.bitmap[ii]);
      }

    if (requirements->ext_names == NULL)
      {
        requirements->ext_name_count = ext_count;
      }
    else
      {
        if (requirements->ext_name_count < ext_count)
          {
            return false;
          }
        else
          {
            //
            // FIXME(allanmac): this can be accelerated by exploiting
            // the extension bitmap
            //
            uint32_t ii = 0;

#define HOTSORT_VK_TARGET_EXTENSION_STRING(ext_) "VK_" STRINGIFY_MACRO(ext_)

#undef HOTSORT_VK_TARGET_EXTENSION
#define HOTSORT_VK_TARGET_EXTENSION(ext_)                                                          \
  if (target->config.extensions.named.ext_)                                                        \
    {                                                                                              \
      requirements->ext_names[ii++] = HOTSORT_VK_TARGET_EXTENSION_STRING(ext_);                    \
    }

            HOTSORT_VK_TARGET_EXTENSIONS()

            //
            // FIXME(allanmac): remove this as soon as it's verified
            // that the Intel driver does the right thing
            //
            if (target->config.extensions.named.AMD_shader_info)
              {
                vk_shader_info_amd_statistics_enable();
              }
          }
      }
  }

  //
  // REQUIRED FEATURES
  //
  {
    if (requirements->pdf != NULL)
      {
        //
        // Let's always have this on during debug
        //
#ifndef NDEBUG
        requirements->pdf->robustBufferAccess = true;
#endif

        //
        // Enable target features
        //
#undef HOTSORT_VK_TARGET_FEATURE
#define HOTSORT_VK_TARGET_FEATURE(feature_)                                                        \
  if (target->config.features.named.feature_)                                                      \
    requirements->pdf->feature_ = true;

        HOTSORT_VK_TARGET_FEATURES()
      }
  }

  return true;
}

//
//
//
