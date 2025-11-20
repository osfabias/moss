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
#include "src/internal/vulkan/utils/image.h"
#include "src/internal/vulkan/utils/image_view.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult moss__createDepthResources (MossEngine *const pEngine)
{
  // TODO: add moss_vk__select_format function that will select format from
  //       the list of desired formats ordered by priority.
  const VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;


  {  // Create depth image
    const MossVk__CreateImageInfo info = {
      .device                      = pEngine->device,
      .format                      = depthImageFormat,
      .imageWidth                  = pEngine->swapchainExtent.width,
      .imageHeight                 = pEngine->swapchainExtent.height,
      .usage                       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .sharingMode                 = pEngine->bufferSharingMode,
      .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
      .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
    };

    {
      const MossResult result = mossVk__createImage (&info, &pEngine->depthImage);
      if (result != MOSS_RESULT_SUCCESS)
      {
        moss__error ("Failed to create depth image.\n");
        return MOSS_RESULT_ERROR;
      }
    }
  }


  {  // Allocate memory for texture image
    const MossVk__AllocateImageMemoryInfo info = {
      .physicalDevice = pEngine->physicalDevice,
      .device         = pEngine->device,
      .image          = pEngine->depthImage,
    };

    {
      const MossResult result =
        mossVk__allocateImageMemory (&info, &pEngine->depthImageMemory);
      if (result != MOSS_RESULT_SUCCESS)
      {
        vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);

        moss__error ("Failed to allocate memory for the depth image.\n");
        return MOSS_RESULT_ERROR;
      }
    }
  }

  {  // Create depth image view
    const MossVk__ImageViewCreateInfo info = {
      .device = pEngine->device,
      .image  = pEngine->depthImage,
      .format = depthImageFormat,
      .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
    };

    const MossResult result = mossVk__createImageView (&info, &pEngine->depthImageView);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkFreeMemory (pEngine->device, pEngine->depthImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);

      moss__error ("Failed to create depth image view.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Transition depth image layout
    const MossVk__TransitionImageLayoutInfo info = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .image         = pEngine->depthImage,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const MossResult result = mossVk__transitionImageLayout (&info);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyImageView (pEngine->device, pEngine->depthImageView, NULL);
      vkFreeMemory (pEngine->device, pEngine->depthImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);

      moss__error ("Failed to transition depth image layout.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}

void moss__cleanupDepthResources (MossEngine *const pEngine)
{
  if (pEngine->depthImageView != VK_NULL_HANDLE)
  {
    vkDestroyImageView (pEngine->device, pEngine->depthImageView, NULL);
    pEngine->depthImageView = VK_NULL_HANDLE;
  }

  if (pEngine->depthImageMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory (pEngine->device, pEngine->depthImageMemory, NULL);
    pEngine->depthImageMemory = VK_NULL_HANDLE;
  }

  if (pEngine->depthImage != VK_NULL_HANDLE)
  {
    vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);
    pEngine->depthImage = VK_NULL_HANDLE;
  }
}

