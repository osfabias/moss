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
*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/physical_device.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult moss__createLogicalDevice (MossEngine *const pEngine)
{
  const MossVk__PhysicalDeviceExtensions extensions =
    mossVk__getRequiredDeviceExtensions ( );

  uint32_t                queue_create_info_count = 0;
  VkDeviceQueueCreateInfo queue_create_infos[ 3 ];
  const float             queue_priority = 1.0F;

  // Add graphics queue create info
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = pEngine->queueFamilyIndices.graphicsFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  // Add present queue create info
  if (pEngine->queueFamilyIndices.graphicsFamily !=
      pEngine->queueFamilyIndices.presentFamily)
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = pEngine->queueFamilyIndices.presentFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  // Add transfer queue create info
  if (pEngine->queueFamilyIndices.graphicsFamily !=
      pEngine->queueFamilyIndices.transferFamily)
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = pEngine->queueFamilyIndices.transferFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  VkPhysicalDeviceFeatures device_features = { 0 };

  const VkDeviceCreateInfo create_info = {
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount    = queue_create_info_count,
    .pQueueCreateInfos       = queue_create_infos,
    .enabledExtensionCount   = extensions.count,
    .ppEnabledExtensionNames = extensions.names,
    .pEnabledFeatures        = &device_features,
  };

  const VkResult result =
    vkCreateDevice (pEngine->physicalDevice, &create_info, NULL, &pEngine->device);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create logical device. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

void moss_initBufferSharingMode (MossEngine *const pEngine)
{
  if (pEngine->queueFamilyIndices.graphicsFamily ==
      pEngine->queueFamilyIndices.transferFamily)
  {
    pEngine->bufferSharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    pEngine->sharedQueueFamilyIndexCount = 0;
  }
  else {
    pEngine->bufferSharingMode             = VK_SHARING_MODE_CONCURRENT;
    pEngine->sharedQueueFamilyIndices[ 0 ] = pEngine->queueFamilyIndices.graphicsFamily;
    pEngine->sharedQueueFamilyIndices[ 1 ] = pEngine->queueFamilyIndices.transferFamily;
    pEngine->sharedQueueFamilyIndexCount   = 2;
  }
}
