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

  @file src/internal/vulkan/utils/memory.h
  @brief Utility functions for GPU memory management.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "moss/result.h"
#include <stdint.h>

#include <vulkan/vulkan.h>

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Required info to select suitable memory type.
*/
typedef struct
{
  VkPhysicalDevice      physicalDevice; /* Physical device to search memory type on. */
  uint32_t              typeFilter;     /* Memory type filter. */
  VkMemoryPropertyFlags properties;     /* Required memory properties. */
} Moss__SelectSuitableMemoryTypeInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Searches for the suitable memory type that satisfies passed filter and
  properties.
  @param info Required info to select suitable memory type.
  @param out_memory_type Output parameter for found memory type.
  @return MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__selectSuitableMemoryType (
  const Moss__SelectSuitableMemoryTypeInfo *const info,
  uint32_t *const                                 outMemoryType
)
{
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties (info->physicalDevice, &memoryProperties);

  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
  {
    const VkMemoryType memoryType = memoryProperties.memoryTypes[ i ];

    if (info->typeFilter & (1 << i) &&
        (memoryType.propertyFlags & info->properties) == info->properties)
    {
      *outMemoryType = i;
      return MOSS_RESULT_SUCCESS;
    }
  }

  return MOSS_RESULT_ERROR;
}

