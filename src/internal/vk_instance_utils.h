/*
  Copyright 2025 Osfabias

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  @file src/internal/vk_instance_utils.c
  @brief Required Vulkan instance creation utility functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "vulkan/vulkan_core.h"
#include <stdint.h>

/*
  @brief Vulkan instance extensions.
*/
typedef struct
{
  const char *const *names;
  const uint32_t     count;
} Moss__VkInstanceExtensions;

/*
  @brief Returns a list of required Vulkan instance extensions.
  @return Instance extensions struct.
*/
Moss__VkInstanceExtensions moss__get_required_vk_instance_extensions (void)
{
#ifdef __APPLE__
  static const char *const extension_names[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    "VK_EXT_metal_surface",
  };

  static const uint32_t extension_count =
    sizeof (extension_names) / sizeof (extension_names[ 0 ]);
#else
#  error "Vulkan instance extensions are not specified for the current target platform."
#endif

  static const Moss__VkInstanceExtensions extensions = {
    .names = extension_names,
    .count = extension_count,
  };

  return extensions;
}

/*
  @brief Returns required Vulkan instance flags.
  @return Vulkan instance flags value.
*/
uint32_t moss__get_required_vk_instance_flags (void)
{
#ifdef __APPLE__
  return VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
  return 0;
#endif
}
