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

#include "src/internal/config.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/image_view.h"
#include "src/internal/vulkan/utils/swapchain.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult
moss__createSwapchain (MossEngine *const pEngine, const VkExtent2D extent)
{
  const MossVk__QuerySwapchainSupportInfo queryInfo = {
    .device  = pEngine->physicalDevice,
    .surface = pEngine->surface,
  };
  const MossVk__SwapChainSupportDetails swapchainSupport =
    mossVk__querySwapchainSupport (&queryInfo);

  const VkSurfaceFormatKHR surfaceFormat = mossVk__chooseSwapSurfaceFormat (
    swapchainSupport.formats,
    swapchainSupport.formatCount
  );
  const VkPresentModeKHR presentMode = mossVk__chooseSwapPresentMode (
    swapchainSupport.presentModes,
    swapchainSupport.presentModeCount
  );
  const VkExtent2D chosenExtent = mossVk__chooseSwapExtent (
    &swapchainSupport.capabilities,
    extent.width,
    extent.height
  );

  VkSwapchainCreateInfoKHR createInfo = {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = pEngine->surface,
    .minImageCount    = swapchainSupport.capabilities.minImageCount,
    .imageFormat      = surfaceFormat.format,
    .imageColorSpace  = surfaceFormat.colorSpace,
    .imageExtent      = chosenExtent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform     = swapchainSupport.capabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = presentMode,
    .clipped          = VK_TRUE,
    .oldSwapchain     = VK_NULL_HANDLE,
  };

  uint32_t queueFamilyIndices[] = {
    pEngine->queueFamilyIndices.graphicsFamily,
    pEngine->queueFamilyIndices.presentFamily,
  };

  if (pEngine->queueFamilyIndices.graphicsFamily !=
      pEngine->queueFamilyIndices.presentFamily)
  {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = NULL;
  }

  const VkResult result =
    vkCreateSwapchainKHR (pEngine->device, &createInfo, NULL, &pEngine->swapchain);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create swap chain. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  vkGetSwapchainImagesKHR (
    pEngine->device,
    pEngine->swapchain,
    &pEngine->swapchainImageCount,
    NULL
  );

  if (pEngine->swapchainImageCount > MAX_SWAPCHAIN_IMAGE_COUNT)
  {
    moss__error (
      "Real swapchain image count is bigger than expected. (%d > %zu)",
      pEngine->swapchainImageCount,
      MAX_SWAPCHAIN_IMAGE_COUNT
    );
    return MOSS_RESULT_ERROR;
  }

  vkGetSwapchainImagesKHR (
    pEngine->device,
    pEngine->swapchain,
    &pEngine->swapchainImageCount,
    pEngine->swapchainImages
  );

  pEngine->swapchainImageFormat = surfaceFormat.format;
  pEngine->swapchainExtent      = extent;

  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createSwapchainImageViews (MossEngine *const pEngine)
{
  MossVk__ImageViewCreateInfo info = {
    .device = pEngine->device,
    .image  = VK_NULL_HANDLE,
    .format = pEngine->swapchainImageFormat,
    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
  };

  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    info.image = pEngine->swapchainImages[ i ];
    const MossResult result =
      mossVk__createImageView (&info, &pEngine->presentFramebufferImageViews[ i ]);
    if (result != MOSS_RESULT_SUCCESS) { return MOSS_RESULT_ERROR; }
  }

  return MOSS_RESULT_SUCCESS;
}

void moss__cleanupSwapchainFramebuffers (MossEngine *const pEngine)
{
  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    if (pEngine->presentFramebuffers[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyFramebuffer (pEngine->device, pEngine->presentFramebuffers[ i ], NULL);
    pEngine->presentFramebuffers[ i ] = VK_NULL_HANDLE;
  }
}

void moss__cleanupSwapchainImageViews (MossEngine *const pEngine)
{
  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    if (pEngine->presentFramebufferImageViews[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyImageView (
      pEngine->device,
      pEngine->presentFramebufferImageViews[ i ],
      NULL
    );
    pEngine->presentFramebufferImageViews[ i ] = VK_NULL_HANDLE;
  }
}

void moss__cleanupSwapchainHandle (MossEngine *const pEngine)
{
  if (pEngine->swapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR (pEngine->device, pEngine->swapchain, NULL);
    pEngine->swapchain = VK_NULL_HANDLE;
  }
}

void moss__cleanupSwapchain (MossEngine *const pEngine)
{
  moss__cleanupSwapchainFramebuffers (pEngine);
  moss__cleanupDepthResources (pEngine);
  moss__cleanupSwapchainImageViews (pEngine);
  moss__cleanupSwapchainHandle (pEngine);

  pEngine->swapchainImageCount  = 0;
  pEngine->swapchainImageFormat = 0;
  pEngine->swapchainExtent      = (VkExtent2D) { .width = 0, .height = 0 };
}

MossResult
moss__recreateSwapchain (MossEngine *const pEngine, const VkExtent2D extent)
{
  vkDeviceWaitIdle (pEngine->device);

  moss__cleanupSwapchain (pEngine);

  if (moss__createSwapchain (pEngine, extent) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__createSwapchainImageViews (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__createDepthResources (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__createPresentFramebuffers (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

