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

  @file src/internal/vulkan/utils/swapchain.h
  @brief Vulkan swap chain utility functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "src/internal/log.h"
#include "vulkan/vulkan_core.h"

/* Max number of available Vulkan formats. */
#define MAX_VULKAN_FORMAT_COUNT (uint32_t)(265)

/* Max number of available Vulkan present modes . */
#define MAX_VULKAN_PRESENT_MODE_COUNT (uint32_t)(265)

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Swap chain support details.
  @details Contains information about swap chain capabilities.
*/
typedef struct
{
  /* Surface capabilities. */
  VkSurfaceCapabilitiesKHR capabilities;

  /* Available surface formats. */
  VkSurfaceFormatKHR formats[ MAX_VULKAN_FORMAT_COUNT ];

  /* Number of formats. */
  uint32_t formatCount;

  /* Available present modes. */
  VkPresentModeKHR presentModes[ MAX_VULKAN_PRESENT_MODE_COUNT ];

  /* Number of present modes. */
  uint32_t presentModeCount;
} MossVk__SwapChainSupportDetails;

/*
  @brief Required info to query swapchain support.
*/
typedef struct
{
  VkPhysicalDevice device;  /* Physical device. */
  VkSurfaceKHR     surface; /* Surface to query. */
} MossVk__QuerySwapchainSupportInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Query swap chain support details for a physical device.
  @param info Required info to query swapchain support.
  @return Swap chain support details. Caller must free formats and present_modes arrays.
*/
inline static MossVk__SwapChainSupportDetails
mossVk__querySwapchainSupport (const MossVk__QuerySwapchainSupportInfo *const info)
{
  MossVk__SwapChainSupportDetails details = {0};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR (
    info->device,
    info->surface,
    &details.capabilities
  );

  vkGetPhysicalDeviceSurfaceFormatsKHR (
    info->device,
    info->surface,
    &details.formatCount,
    NULL
  );

  if (details.formatCount <= MAX_VULKAN_FORMAT_COUNT)
  {
    vkGetPhysicalDeviceSurfaceFormatsKHR (
      info->device,
      info->surface,
      &details.formatCount,
      details.formats
    );
  }
  else
  {
    moss__error (
      "Format count exceeded the limit (%d > %d). No formats saved.",
      details.formatCount,
      MAX_VULKAN_FORMAT_COUNT
    );
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR (
    info->device,
    info->surface,
    &details.presentModeCount,
    NULL
  );

  if (details.presentModeCount <= MAX_VULKAN_PRESENT_MODE_COUNT)
  {
    vkGetPhysicalDeviceSurfacePresentModesKHR (
      info->device,
      info->surface,
      &details.presentModeCount,
      details.presentModes
    );
  }
  else
  {
    moss__error (
      "Present mode count exceeded the limit (%d > %d). No formats saved.",
      details.presentModeCount,
      MAX_VULKAN_PRESENT_MODE_COUNT
    );
  }

  return details;
}

/*
  @brief Choose swap surface format.
  @param available_formats Available formats array.
  @param format_count Number of available formats.
  @return Selected surface format.
*/
inline static VkSurfaceFormatKHR mossVk__chooseSwapSurfaceFormat (
  const VkSurfaceFormatKHR *availableFormats,
  uint32_t                  formatCount
)
{
  for (uint32_t i = 0; i < formatCount; ++i)
  {
    if (availableFormats[ i ].format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormats[ i ].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormats[ i ];
    }
  }

  return availableFormats[ 0 ];
}

/*
  @brief Choose swap present mode.
  @param available_present_modes Available present modes array.
  @param present_mode_count Number of available present modes.
  @return Selected present mode.
*/
inline static VkPresentModeKHR mossVk__chooseSwapPresentMode (
  const VkPresentModeKHR *availablePresentModes,
  uint32_t                presentModeCount
)
{
  for (uint32_t i = 0; i < presentModeCount; ++i)
  {
    if (availablePresentModes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentModes[ i ];
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

/*
  @brief Choose swap extent.
  @param capabilities Surface capabilities.
  @param width Desired width.
  @param height Desired height.
  @return Selected extent.
*/
inline static VkExtent2D mossVk__chooseSwapExtent (
  const VkSurfaceCapabilitiesKHR *capabilities,
  uint32_t                        width,
  uint32_t                        height
)
{
  if (capabilities->currentExtent.width != UINT32_MAX)
  {
    return capabilities->currentExtent;
  }

  VkExtent2D actualExtent = {width, height};

  if (actualExtent.width < capabilities->minImageExtent.width)
  {
    actualExtent.width = capabilities->minImageExtent.width;
  }
  if (actualExtent.width > capabilities->maxImageExtent.width)
  {
    actualExtent.width = capabilities->maxImageExtent.width;
  }

  if (actualExtent.height < capabilities->minImageExtent.height)
  {
    actualExtent.height = capabilities->minImageExtent.height;
  }
  if (actualExtent.height > capabilities->maxImageExtent.height)
  {
    actualExtent.height = capabilities->maxImageExtent.height;
  }

  return actualExtent;
}

