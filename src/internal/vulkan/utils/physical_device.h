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
  uint32_t graphicsFamily;      /* Graphics queue family index. */
  uint32_t presentFamily;       /* Present queue family index. */
  uint32_t transferFamily;      /* Transfer queue family index. */
  bool     graphicsFamilyFound; /* Whether graphics family index is valid. */
  bool     presentFamilyFound;  /* Whether present family index is valid. */
  bool     transferFamilyFound; /* Whether present family index is valid. */
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
mossVk__getRequiredDeviceExtensions (void)
{
#ifdef __APPLE__
  static const char *const extensionNames[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
  };
#else
#  error \
    "Vulkan physical device extensions aren't specified for the current target platform."
#endif

  static const MossVk__PhysicalDeviceExtensions extensions = {
    .names = extensionNames,
    .count = sizeof (extensionNames) / sizeof (extensionNames[ 0 ]),
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
mossVk__findQueueFamilies (const MossVk__FindQueueFamiliesInfo *const info)
{
  MossVk__QueueFamilyIndices indices = {
    .graphicsFamilyFound = false,
    .presentFamilyFound  = false,
    .transferFamilyFound = false,
  };

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties (info->device, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilies[ queueFamilyCount ];
  vkGetPhysicalDeviceQueueFamilyProperties (
    info->device,
    &queueFamilyCount,
    queueFamilies
  );

  for (uint32_t i = 0; i < queueFamilyCount; ++i)
  {
    VkQueueFlags queueFamilyFlags = queueFamilies[ i ].queueFlags;

    if ((queueFamilyFlags & VK_QUEUE_TRANSFER_BIT) &&
        !(queueFamilyFlags & VK_QUEUE_GRAPHICS_BIT))
    {
      indices.transferFamily       = i;
      indices.transferFamilyFound = true;
    }

    if (queueFamilyFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily       = i;
      indices.graphicsFamilyFound = true;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR (
      info->device,
      i,
      info->surface,
      &presentSupport
    );

    if (presentSupport)
    {
      indices.presentFamily       = i;
      indices.presentFamilyFound = true;
    }

    if (indices.transferFamilyFound && indices.graphicsFamilyFound &&
        indices.presentFamilyFound)
    {
      break;
    }
  }

  // If transfer queue family not found, set it to be the same as the graphics family
  if (!indices.transferFamilyFound)
  {
    indices.transferFamily       = indices.graphicsFamily;
    indices.transferFamilyFound = true;
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
inline static bool mossVk__checkDeviceQueuesSupport (
  const MossVk__CheckDeviceQueuesSupportInfo *const info
)
{
#ifndef NDEBUG
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties (info->device, &deviceProperties);
#endif

    const MossVk__FindQueueFamiliesInfo findInfo = {
      .device  = info->device,
      .surface = info->surface,
    };
    const MossVk__QueueFamilyIndices indices = mossVk__findQueueFamilies (&findInfo);

  if (!indices.presentFamilyFound)
  {
    moss__info (
      "%s device do not support required present queue family.\n",
      deviceProperties.deviceName
    );
    return false;
  }

  if (!indices.graphicsFamilyFound)
  {
    moss__info (
      "%s device do not support required graphics queue family.\n",
      deviceProperties.deviceName
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
inline static bool mossVk__checkDeviceExtensionSupport (
  const MossVk__CheckDeviceExtensionSupportInfo *const info
)
{
#ifndef NDEBUG
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties (info->device, &deviceProperties);
#endif

  const MossVk__PhysicalDeviceExtensions requiredExtensions =
    mossVk__getRequiredDeviceExtensions ( );

  uint32_t availableExtensionCount;
  vkEnumerateDeviceExtensionProperties (
    info->device,
    NULL,
    &availableExtensionCount,
    NULL
  );

  VkExtensionProperties availableExtensions[ availableExtensionCount ];
  vkEnumerateDeviceExtensionProperties (
    info->device,
    NULL,
    &availableExtensionCount,
    availableExtensions
  );

  for (uint32_t i = 0; i < requiredExtensions.count; ++i)
  {
    bool extensionFound = false;
    for (uint32_t j = 0; j < availableExtensionCount; ++j)
    {
      if (strcmp (
            requiredExtensions.names[ i ],
            availableExtensions[ j ].extensionName
          ) == 0)
      {
        extensionFound = true;
        break;
      }
    }

    if (!extensionFound)
    {
      moss__info (
        "%s device doesn't support required \"%s\" extension.\n",
        deviceProperties.deviceName,
        requiredExtensions.names[ i ]
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
inline static bool mossVk__checkDeviceFormatSupport (
  const MossVk__CheckDeviceFormatSupportInfo *const info
)
{
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR (info->device, info->surface, &formatCount, NULL);

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR (
    info->device,
    info->surface,
    &presentModeCount,
    NULL
  );

  return (formatCount > 0) && (presentModeCount > 0);
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
inline static bool mossVk__isPhysicalDeviceSuitable (
  const MossVk__IsPhysicalDeviceSuitableInfo *const info
)
{
    const MossVk__CheckDeviceQueuesSupportInfo queuesInfo = {
      .device  = info->device,
      .surface = info->surface,
    };
    if (!mossVk__checkDeviceQueuesSupport (&queuesInfo)) { return false; }

    const MossVk__CheckDeviceExtensionSupportInfo extensionInfo = {
      .device = info->device,
    };
    if (!mossVk__checkDeviceExtensionSupport (&extensionInfo)) { return false; }

    const MossVk__CheckDeviceFormatSupportInfo formatInfo = {
      .device  = info->device,
      .surface = info->surface,
    };
  if (!mossVk__checkDeviceFormatSupport (&formatInfo)) { return false; }

  return true;
}

/*
  @brief Required info to select physical device.
*/
typedef struct
{
  VkInstance        instance;  /* Vulkan instance. */
  VkSurfaceKHR      surface;   /* Surface to check presentation support. */
  VkPhysicalDevice *outDevice; /* Pointer to store selected physical device. */
} MossVk__SelectPhysicalDeviceInfo;

/*
  @brief Selects a suitable physical device from available devices.
  @param info Required info to select physical device.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static VkResult
mossVk__selectPhysicalDevice (const MossVk__SelectPhysicalDeviceInfo *const info)
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices (info->instance, &deviceCount, NULL);

  if (deviceCount == 0)
  {
    moss__error ("Failed to find GPUs with Vulkan support.\n");
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  assert (deviceCount < 16);

  VkPhysicalDevice devices[ deviceCount ];
  vkEnumeratePhysicalDevices (info->instance, &deviceCount, devices);

  for (uint32_t i = 0; i < deviceCount; ++i)
  {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties (devices[ i ], &deviceProperties);

    const MossVk__IsPhysicalDeviceSuitableInfo suitableInfo = {
      .device  = devices[ i ],
      .surface = info->surface,
    };
    if (mossVk__isPhysicalDeviceSuitable (&suitableInfo))
    {
      *info->outDevice = devices[ i ];

      moss__info ("Selected %s as the target GPU.\n", deviceProperties.deviceName);
      return VK_SUCCESS;
    }
  }

  moss__error ("Failed to find a suitable GPU.\n");
  return VK_ERROR_INITIALIZATION_FAILED;
}
