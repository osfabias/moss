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

  @file src/internal/vulkan/utils/image.h
  @brief Vulkan image utility functions.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/log.h"
#include "src/internal/vulkan/utils/memory.h"
#include "src/internal/vulkan/utils/command_buffer.h"
#include "vulkan/vulkan_core.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Required info to create Vulkan image.
*/
typedef struct
{
  VkDevice          device;                /* Device to create image on. */
  VkFormat          format;                /* Image format. */
  uint32_t          imageWidth;            /* Image width. */
  uint32_t          imageHeight;           /* Image height. */
  VkImageUsageFlags usage;                 /* Image usage flags. */
  VkSharingMode     sharingMode;           /* Image sharing mode. */
  uint32_t  sharedQueueFamilyIndexCount;   /* Number of shared queue family indices. */
  uint32_t *sharedQueueFamilyIndices;      /* Indices of queue families that will share
                                              image's memory. */
} MossVk__CreateImageInfo;

/*
  @brief Required information to allocate memory for image.
*/
typedef struct
{
  VkPhysicalDevice physicalDevice; /* Physical device to allocate memory on. */
  VkDevice         device;         /* Device where the image created on. */
  VkImage          image;          /* Image to allocate memory for. */
} MossVk__AllocateImageMemoryInfo;

/*
  @brief Required info to transition image layout.
*/
typedef struct
{
  VkDevice      device;       /* Logical device. */
  VkCommandPool commandPool;  /* Command pool to perform operation with. */
  VkQueue       transferQueue; /* Queue to be used as a transfer queue. */
  VkImage       image;        /* Image to transition. */
  VkImageLayout oldLayout;    /* Current image layout. */
  VkImageLayout newLayout;    /* Target image layout. */
} MossVk__TransitionImageLayoutInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Crates Vulkan image.
  @param info Required info to create Vulkan image.
  @param out_image Output parameter for created image handle.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult
mossVk__createImage (const MossVk__CreateImageInfo *const info, VkImage *const outImage)
{
  const VkExtent3D imageExtent = {
    .width  = info->imageWidth,
    .height = info->imageHeight,
    .depth  = 1,
  };

  const VkImageCreateInfo createInfo = {
    .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType             = VK_IMAGE_TYPE_2D,
    .extent                = imageExtent,
    .mipLevels             = 1,
    .arrayLayers           = 1,
    .format                = info->format,
    .tiling                = VK_IMAGE_TILING_OPTIMAL,
    .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    .usage                 = info->usage,
    .sharingMode           = info->sharingMode,
    .queueFamilyIndexCount = info->sharedQueueFamilyIndexCount,
    .pQueueFamilyIndices   = info->sharedQueueFamilyIndices,
    .samples               = VK_SAMPLE_COUNT_1_BIT,
  };

  const VkResult result = vkCreateImage (info->device, &createInfo, NULL, outImage);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create Vulkan image. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Allocates device memory for Vulkan image.
  @param info Required info for allocating image memory.
  @param out_image_memory Output parameter for allocated memory handle.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult mossVk__allocateImageMemory (
  const MossVk__AllocateImageMemoryInfo *const info,
  VkDeviceMemory *const                         outImageMemory
)
{
  // Select suitable memory type
  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements (info->device, info->image, &memoryRequirements);

  uint32_t suitableMemoryType;
  {
    const Moss__SelectSuitableMemoryTypeInfo selectInfo = {
      .physicalDevice = info->physicalDevice,
      .typeFilter     = memoryRequirements.memoryTypeBits,
      .properties     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    if (moss__selectSuitableMemoryType (&selectInfo, &suitableMemoryType) !=
        MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to find suitable memory type.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Allocate memory
    const VkMemoryAllocateInfo allocInfo = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext           = NULL,
      .allocationSize  = memoryRequirements.size,
      .memoryTypeIndex = suitableMemoryType,
    };

    const VkResult result =
      vkAllocateMemory (info->device, &allocInfo, NULL, outImageMemory);
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to allocate memory for the image. Error code: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Bind image memory
    const VkResult result =
      vkBindImageMemory (info->device, info->image, *outImageMemory, 0);
    if (result != VK_SUCCESS)
    {
      vkFreeMemory (info->device, *outImageMemory, NULL);

      moss__error ("Failed to bind image memory to the image. Error code: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}


/*
  @brief Transitions image layout from one to another.
  @param info Required info to perform transition.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult
mossVk__transitionImageLayout (const MossVk__TransitionImageLayoutInfo *const info)
{
  VkCommandBuffer commandBuffer;
  {  // Begin one time command buffer
    const MossVk__BeginOneTimeCommandBufferInfo beginInfo = {
      .device      = info->device,
      .commandPool = info->commandPool,
    };
    const MossResult result =
      mossVk__beginOneTimeCommandBuffer (&beginInfo, &commandBuffer);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to begin one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  VkImageMemoryBarrier barrier = {
    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout           = info->oldLayout,
    .newLayout           = info->newLayout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image               = info->image,
    .srcAccessMask       = 0,
    .dstAccessMask       = 0,
    .subresourceRange    = {
      .aspectMask     = VK_IMAGE_ASPECT_NONE,
      .baseMipLevel   = 0,
      .levelCount     = 1,
      .baseArrayLayer = 0,
      .layerCount     = 1,
    }
  };

  // Determine aspect mask
  if (info->newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  // Determine source and destination
  VkPipelineStageFlags sourceStage      = 0;
  VkPipelineStageFlags destinationStage = 0;

  if (info->oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      info->newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (info->oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           info->newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (info->oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
           info->newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }
  else {
    moss__error (
      "Unsupported image layout transition: %u -> %u.\n",
      info->oldLayout,
      info->newLayout
    );
    return MOSS_RESULT_ERROR;
  }

  vkCmdPipelineBarrier (
    commandBuffer,
    sourceStage,
    destinationStage,
    0,
    0,
    NULL,
    0,
    NULL,
    1,
    &barrier
  );

  {  // End one time command buffer
    const MossVk__EndOneTimeCommandBufferInfo endInfo = {
      .device        = info->device,
      .commandPool   = info->commandPool,
      .commandBuffer = commandBuffer,
      .queue         = info->transferQueue,
    };
    const MossResult result = mossVk__endOneTimeCommandBuffer (&endInfo);

    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}
