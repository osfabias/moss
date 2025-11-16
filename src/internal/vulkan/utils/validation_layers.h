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

  @file src/internal/vulkan/utils/validation_layers.h
  @brief Vulkan validation layers utility functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "src/internal/log.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Vulkan validation layers.
*/
typedef struct
{
  const char *const *names; /* Layer names. */
  const uint32_t     count; /* Layer count. */
} Moss__VkValidationLayers;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

inline static Moss__VkValidationLayers moss__get_vk_validation_layers (void)
{
  static const char *const validation_layer_names[] = {"VK_LAYER_KHRONOS_validation"};

  static const Moss__VkValidationLayers validation_layers = {
    .names = validation_layer_names,
    .count = sizeof (validation_layer_names) / sizeof (validation_layer_names[ 0 ]),
  };

  return validation_layers;
}

/*
  @brief Checks if validation layers are available.
*/
inline static bool moss__check_vk_validation_layer_support (void)
{
  uint32_t available_layer_count;
  vkEnumerateInstanceLayerProperties (&available_layer_count, NULL);

  if (available_layer_count == 0)
  {
    moss__error ("No validation layers available. ");
    return false;
  }

  assert (available_layer_count < 16);

  VkLayerProperties available_layers[ available_layer_count ];
  vkEnumerateInstanceLayerProperties (&available_layer_count, available_layers);

  const Moss__VkValidationLayers required_validation_layers =
    moss__get_vk_validation_layers ( );

  for (uint32_t i = 0; i < required_validation_layers.count; ++i)
  {
    bool              layer_found       = false;
    const char *const target_layer_name = required_validation_layers.names[ i ];

    for (uint32_t j = 0; j < available_layer_count; ++j)
    {
      if (strcmp (target_layer_name, available_layers[ j ].layerName) == 0)
      {
        layer_found = true;
        break;
      }
    }

    if (layer_found == false) { return false; }
  }

  return true;
}

