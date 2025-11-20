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
} MossVk__ValidationLayers;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

inline static MossVk__ValidationLayers mossVk__getValidationLayers (void)
{
  static const char *const validationLayerNames[] = {"VK_LAYER_KHRONOS_validation"};

  static const MossVk__ValidationLayers validationLayers = {
    .names = validationLayerNames,
    .count = sizeof (validationLayerNames) / sizeof (validationLayerNames[ 0 ]),
  };

  return validationLayers;
}

/*
  @brief Checks if validation layers are available.
*/
inline static bool mossVk__checkValidationLayerSupport (void)
{
  uint32_t availableLayerCount;
  vkEnumerateInstanceLayerProperties (&availableLayerCount, NULL);

  if (availableLayerCount == 0)
  {
    moss__error ("No validation layers available. ");
    return false;
  }

  assert (availableLayerCount < 16);

  VkLayerProperties availableLayers[ availableLayerCount ];
  vkEnumerateInstanceLayerProperties (&availableLayerCount, availableLayers);

  const MossVk__ValidationLayers requiredValidationLayers =
    mossVk__getValidationLayers ( );

  for (uint32_t i = 0; i < requiredValidationLayers.count; ++i)
  {
    bool              layerFound      = false;
    const char *const targetLayerName = requiredValidationLayers.names[ i ];

    for (uint32_t j = 0; j < availableLayerCount; ++j)
    {
      if (strcmp (targetLayerName, availableLayers[ j ].layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (layerFound == false) { return false; }
  }

  return true;
}

