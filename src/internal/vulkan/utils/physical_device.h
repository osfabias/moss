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

  @file src/internal/vulkan/utils/physical_device.h
  @brief Vulkan physical device selection utility functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "src/internal/log.h"
#include "vulkan/vulkan_core.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Queue family indices.
  @details Stores the indices of queue families that are required for rendering.
*/
typedef struct
{
  uint32_t graphics_family;       /* Graphics queue family index. */
  uint32_t present_family;        /* Present queue family index. */
  uint32_t transfer_family;       /* Transfer queue family index. */
  bool     graphics_family_found; /* Whether graphics family index is valid. */
  bool     present_family_found;  /* Whether present family index is valid. */
  bool     transfer_family_found; /* Whether present family index is valid. */
} MossVk__QueueFamilyIndices;

/*
  @brief Vulkan physical device extensions.
*/
typedef struct
{
  const char *const *names; /* Extension names. */
  const uint32_t     count; /* Extension count. */
} MossVk__PhysicalDeviceExtensions;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

inline static MossVk__PhysicalDeviceExtensions
moss_vk__get_required_device_extensions (void)
{
#ifdef __APPLE__
  static const char *const extension_names[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
  };
#else
#  error \
    "Vulkan physical device extensions aren't specified for the current target platform."
#endif

  static const MossVk__PhysicalDeviceExtensions extensions = {
    .names = extension_names,
    .count = sizeof (extension_names) / sizeof (extension_names[ 0 ]),
  };

  return extensions;
}

/*
  @brief Required info to find queue families.
*/
typedef struct
{
  VkPhysicalDevice device;  /* Physical device to query. */
  VkSurfaceKHR     surface; /* Surface to check presentation support. */
} MossVk__FindQueueFamiliesInfo;

/*
  @brief Finds queue families for a physical device.
  @param info Required info to find queue families.
  @return Queue family indices structure.
*/
inline static MossVk__QueueFamilyIndices
moss_vk__find_queue_families (const MossVk__FindQueueFamiliesInfo *const info)
{
  MossVk__QueueFamilyIndices indices = {
    .graphics_family_found = false,
    .present_family_found  = false,
    .transfer_family_found = false,
  };

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (info->device, &queue_family_count, NULL);

  VkQueueFamilyProperties queue_families[ queue_family_count ];
  vkGetPhysicalDeviceQueueFamilyProperties (
    info->device,
    &queue_family_count,
    queue_families
  );

  for (uint32_t i = 0; i < queue_family_count; ++i)
  {
    VkQueueFlags queue_family_flags = queue_families[ i ].queueFlags;

    if ((queue_family_flags & VK_QUEUE_TRANSFER_BIT) &&
        !(queue_family_flags & VK_QUEUE_GRAPHICS_BIT))
    {
      indices.transfer_family       = i;
      indices.transfer_family_found = true;
    }

    if (queue_family_flags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphics_family       = i;
      indices.graphics_family_found = true;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR (
      info->device,
      i,
      info->surface,
      &present_support
    );

    if (present_support)
    {
      indices.present_family       = i;
      indices.present_family_found = true;
    }

    if (indices.transfer_family_found && indices.graphics_family_found &&
        indices.present_family_found)
    {
      break;
    }
  }

  // If transfer queue family not found, set it to be the same as the graphics family
  if (!indices.transfer_family_found)
  {
    indices.transfer_family       = indices.graphics_family;
    indices.transfer_family_found = true;
  }

  return indices;
}


/*
  @brief Required info to check device queue support.
*/
typedef struct
{
  VkPhysicalDevice device;  /* Physical device to check. */
  VkSurfaceKHR     surface; /* Surface to check presentation support. */
} MossVk__CheckDeviceQueuesSupportInfo;

/*
  @brief Checks if device supports required queues.
  @param info Required info to check device queue support.
  @return True if all required queues are supported, otherwise false.
*/
inline static bool moss_vk__check_device_queues_support (
  const MossVk__CheckDeviceQueuesSupportInfo *const info
)
{
#ifndef NDEBUG
  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties (info->device, &device_properties);
#endif

    const MossVk__FindQueueFamiliesInfo find_info = {
      .device  = info->device,
      .surface = info->surface,
    };
    const MossVk__QueueFamilyIndices indices = moss_vk__find_queue_families (&find_info);

  if (!indices.present_family_found)
  {
    moss__info (
      "%s device do not support required present queue family.\n",
      device_properties.deviceName
    );
    return false;
  }

  if (!indices.graphics_family_found)
  {
    moss__info (
      "%s device do not support required graphics queue family.\n",
      device_properties.deviceName
    );
    return false;
  }

  return true;
}

/*
  @brief Required info to check device extension support.
*/
typedef struct
{
  VkPhysicalDevice device; /* Physical device to check. */
} MossVk__CheckDeviceExtensionSupportInfo;

/*
  @brief Checks if device supports required extensions.
  @param info Required info to check device extension support.
  @return True if all required extensions are supported, otherwise false.
*/
inline static bool moss_vk__check_device_extension_support (
  const MossVk__CheckDeviceExtensionSupportInfo *const info
)
{
#ifndef NDEBUG
  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties (info->device, &device_properties);
#endif

  const MossVk__PhysicalDeviceExtensions required_extensions =
    moss_vk__get_required_device_extensions ( );

  uint32_t available_extension_count;
  vkEnumerateDeviceExtensionProperties (
    info->device,
    NULL,
    &available_extension_count,
    NULL
  );

  VkExtensionProperties available_extensions[ available_extension_count ];
  vkEnumerateDeviceExtensionProperties (
    info->device,
    NULL,
    &available_extension_count,
    available_extensions
  );

  for (uint32_t i = 0; i < required_extensions.count; ++i)
  {
    bool extension_found = false;
    for (uint32_t j = 0; j < available_extension_count; ++j)
    {
      if (strcmp (
            required_extensions.names[ i ],
            available_extensions[ j ].extensionName
          ) == 0)
      {
        extension_found = true;
        break;
      }
    }

    if (!extension_found)
    {
      moss__info (
        "%s device doesn't support required \"%s\" extension.\n",
        device_properties.deviceName,
        required_extensions.names[ i ]
      );
      return false;
    }
  }

  return true;
}


/*
  @brief Required info to check device format support.
*/
typedef struct
{
  VkPhysicalDevice device;  /* Physical device to check. */
  VkSurfaceKHR     surface; /* Surface to check format support. */
} MossVk__CheckDeviceFormatSupportInfo;

/*
  @brief Checks if device supports required formats.
  @param info Required info to check device format support.
  @return True if all required formats are supported, otherwise false.
*/
inline static bool moss_vk__check_device_format_support (
  const MossVk__CheckDeviceFormatSupportInfo *const info
)
{
  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR (info->device, info->surface, &format_count, NULL);

  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR (
    info->device,
    info->surface,
    &present_mode_count,
    NULL
  );

  return (format_count > 0) && (present_mode_count > 0);
}

/*
  @brief Required info to check if physical device is suitable.
*/
typedef struct
{
  VkPhysicalDevice device;  /* Physical device to check. */
  VkSurfaceKHR     surface; /* Surface to check presentation support. */
} MossVk__IsPhysicalDeviceSuitableInfo;

/*
  @brief Checks if a physical device is suitable for our needs.
  @param info Required info to check if physical device is suitable.
  @return True if device is suitable, false otherwise.
*/
inline static bool moss_vk__is_physical_device_suitable (
  const MossVk__IsPhysicalDeviceSuitableInfo *const info
)
{
    const MossVk__CheckDeviceQueuesSupportInfo queues_info = {
      .device  = info->device,
      .surface = info->surface,
    };
    if (!moss_vk__check_device_queues_support (&queues_info)) { return false; }

    const MossVk__CheckDeviceExtensionSupportInfo extension_info = {
      .device = info->device,
    };
    if (!moss_vk__check_device_extension_support (&extension_info)) { return false; }

    const MossVk__CheckDeviceFormatSupportInfo format_info = {
      .device  = info->device,
      .surface = info->surface,
    };
  if (!moss_vk__check_device_format_support (&format_info)) { return false; }

  return true;
}

/*
  @brief Required info to select physical device.
*/
typedef struct
{
  VkInstance        instance;   /* Vulkan instance. */
  VkSurfaceKHR      surface;    /* Surface to check presentation support. */
  VkPhysicalDevice *out_device; /* Pointer to store selected physical device. */
} MossVk__SelectPhysicalDeviceInfo;

/*
  @brief Selects a suitable physical device from available devices.
  @param info Required info to select physical device.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static VkResult
moss_vk__select_physical_device (const MossVk__SelectPhysicalDeviceInfo *const info)
{
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices (info->instance, &device_count, NULL);

  if (device_count == 0)
  {
    moss__error ("Failed to find GPUs with Vulkan support.\n");
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  assert (device_count < 16);

  VkPhysicalDevice devices[ device_count ];
  vkEnumeratePhysicalDevices (info->instance, &device_count, devices);

  for (uint32_t i = 0; i < device_count; ++i)
  {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties (devices[ i ], &device_properties);

    const MossVk__IsPhysicalDeviceSuitableInfo suitable_info = {
      .device  = devices[ i ],
      .surface = info->surface,
    };
    if (moss_vk__is_physical_device_suitable (&suitable_info))
    {
      *info->out_device = devices[ i ];

      moss__info ("Selected %s as the target GPU.\n", device_properties.deviceName);
      return VK_SUCCESS;
    }
  }

  moss__error ("Failed to find a suitable GPU.\n");
  return VK_ERROR_INITIALIZATION_FAILED;
}
