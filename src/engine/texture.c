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
#include <string.h>

#include <vulkan/vulkan.h>

#include <stb/stb_image.h>

#include "moss/result.h"

#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/buffer.h"
#include "src/internal/vulkan/utils/image.h"
#include "src/internal/vulkan/utils/image_view.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult moss__createTextureImage (MossEngine *const pEngine)
{
  // Load texture
  int textureWidth, textureHeight, texture_channels;  // NOLINT

  const stbi_uc *const pPixels = stbi_load (
    "textures/atlas.png",
    &textureWidth,
    &textureHeight,
    &texture_channels,
    STBI_rgb_alpha
  );
  if (pPixels == NULL)
  {
    moss__error ("Failed to load texture.");
    return MOSS_RESULT_ERROR;
  }

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  {  // Create staging buffer
    {
      const MossVk__CreateBufferInfo createInfo = {
        .device                      = pEngine->device,
        .size                        = (VkDeviceSize)(textureWidth * textureHeight * 4),
        .usage                       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode                 = pEngine->bufferSharingMode,
        .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
        .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
      };
      const MossResult result = mossVk__createBuffer (&createInfo, &stagingBuffer);
      if (result != MOSS_RESULT_SUCCESS)
      {
        moss__error ("Failed to create staging buffer.");
        return MOSS_RESULT_ERROR;
      }
    }

    {
      const MossVk__AllocateBufferMemoryInfo allocInfo = {
        .physicalDevice = pEngine->physicalDevice,
        .device         = pEngine->device,
        .buffer         = stagingBuffer,
        .memoryProperties =
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      };
      const MossResult result =
        mossVk__allocateBufferMemory (&allocInfo, &stagingBufferMemory);
      if (result != MOSS_RESULT_SUCCESS)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
        moss__error ("Failed to allocate staging buffer memory.");
        return MOSS_RESULT_ERROR;
      }
    }
  }

  // Copy pixels into the buffer
  void *pData;
  vkMapMemory (
    pEngine->device,
    stagingBufferMemory,
    0,
    (VkDeviceSize)(textureWidth * textureHeight * 4),
    0,
    &pData
  );
  memcpy (pData, pPixels, (size_t)(textureWidth * textureHeight * 4));
  vkUnmapMemory (pEngine->device, stagingBufferMemory);

  // Free pixels
  stbi_image_free ((void *)pPixels);

  // Create texture image
  const MossVk__CreateImageInfo createInfo = {
    .device      = pEngine->device,
    .format      = VK_FORMAT_R8G8B8A8_SRGB,
    .imageWidth  = textureWidth,
    .imageHeight = textureHeight,
    .usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = pEngine->bufferSharingMode,
    .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
    .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
  };

  {
    const MossResult result = mossVk__createImage (&createInfo, &pEngine->textureImage);
    if (result != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      moss__error ("Failed to create texture image.\n");
      return MOSS_RESULT_ERROR;
    }
  }


  // Allocate memory for texture image
  const MossVk__AllocateImageMemoryInfo allocInfo = {
    .physicalDevice = pEngine->physicalDevice,
    .device         = pEngine->device,
    .image          = pEngine->textureImage,
  };

  {
    const MossResult result =
      mossVk__allocateImageMemory (&allocInfo, &pEngine->textureImageMemory);
    if (result != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      moss__error ("Failed to allocate memory for the texture image.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__TransitionImageLayoutInfo transitionInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .image         = pEngine->textureImage,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    };
    if (mossVk__transitionImageLayout (&transitionInfo) != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__CopyBufferToImageInfo copyInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .buffer        = stagingBuffer,
      .image         = pEngine->textureImage,
      .width         = textureWidth,
      .height        = textureHeight,
    };
    if (mossVk__copyBufferToImage (&copyInfo) != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__TransitionImageLayoutInfo transitionInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .image         = pEngine->textureImage,
      .oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    if (mossVk__transitionImageLayout (&transitionInfo) != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    if (stagingBufferMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
    }
    if (stagingBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
    }
  }

  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createTextureImageView (MossEngine *const pEngine)
{
  const MossVk__ImageViewCreateInfo info = {
    .device = pEngine->device,
    .image  = pEngine->textureImage,
    .format = VK_FORMAT_R8G8B8A8_SRGB,
    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
  };
  const MossResult result = mossVk__createImageView (&info, &pEngine->textureImageView);
  if (result != MOSS_RESULT_SUCCESS) { return MOSS_RESULT_ERROR; }
  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createTextureSampler (MossEngine *const pEngine)
{
  const VkSamplerCreateInfo create_info = {
    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter               = VK_FILTER_NEAREST,
    .minFilter               = VK_FILTER_NEAREST,
    .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable        = VK_FALSE,
    .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
    .compareEnable           = VK_FALSE,
    .compareOp               = VK_COMPARE_OP_ALWAYS,
    .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias              = 0.0F,
    .minLod                  = 0.0F,
    .maxLod                  = 0.0F,
  };

  const VkResult result =
    vkCreateSampler (pEngine->device, &create_info, NULL, &pEngine->sampler);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create sampler: %d.", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

