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
#include "src/internal/memory_utils.h"
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
  VkDevice          device;                  /* Device to create image on. */
  VkFormat          format;                  /* Image format. */
  uint32_t          image_width;             /* Image width. */
  uint32_t          image_height;            /* Image height. */
  VkImageUsageFlags usage;                   /* Image usage flags. */
  VkSharingMode     sharing_mode;            /* Image sharing mode. */
  uint32_t  shared_queue_family_index_count; /* Number of shared queue family indices. */
  uint32_t *shared_queue_family_indices;     /* Indices of queue families that will share
                                                image's memory. */
} MossVk__CreateImageInfo;

/*
  @brief Required information to allocate memory for image.
*/
typedef struct
{
  VkPhysicalDevice physical_device; /* Physical device to allocate memory on. */
  VkDevice         device;          /* Device where the image created on. */
  VkImage          image;           /* Image to allocate memory for. */
} MossVk__AllocateImageMemoryInfo;

/*
  @brief Required info to transition image layout.
*/
typedef struct
{
  VkDevice      device;         /* Logical device. */
  VkCommandPool command_pool;   /* Command pool to perform operation with. */
  VkQueue       transfer_queue; /* Queue to be used as a transfer queue. */
  VkImage       image;          /* Image to transition. */
  VkImageLayout old_layout;     /* Current image layout. */
  VkImageLayout new_layout;     /* Target image layout. */
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
moss_vk__create_image (const MossVk__CreateImageInfo *const info, VkImage *const out_image)
{
  const VkExtent3D image_extent = {
    .width  = info->image_width,
    .height = info->image_height,
    .depth  = 1,
  };

  const VkImageCreateInfo create_info = {
    .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType             = VK_IMAGE_TYPE_2D,
    .extent                = image_extent,
    .mipLevels             = 1,
    .arrayLayers           = 1,
    .format                = info->format,
    .tiling                = VK_IMAGE_TILING_OPTIMAL,
    .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    .usage                 = info->usage,
    .sharingMode           = info->sharing_mode,
    .queueFamilyIndexCount = info->shared_queue_family_index_count,
    .pQueueFamilyIndices   = info->shared_queue_family_indices,
    .samples               = VK_SAMPLE_COUNT_1_BIT,
  };

  const VkResult result = vkCreateImage (info->device, &create_info, NULL, out_image);
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
inline static MossResult moss_vk__allocate_image_memory (
  const MossVk__AllocateImageMemoryInfo *const info,
  VkDeviceMemory *const                         out_image_memory
)
{
  // Select suitable memory type
  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements (info->device, info->image, &memory_requirements);

  uint32_t suitable_memory_type;
  {
    const Moss__SelectSuitableMemoryTypeInfo select_info = {
      .physical_device = info->physical_device,
      .type_filter     = memory_requirements.memoryTypeBits,
      .properties      = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    if (moss__select_suitable_memory_type (&select_info, &suitable_memory_type) !=
        MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to find suitable memory type.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Allocate memory
    const VkMemoryAllocateInfo alloc_info = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext           = NULL,
      .allocationSize  = memory_requirements.size,
      .memoryTypeIndex = suitable_memory_type,
    };

    const VkResult result =
      vkAllocateMemory (info->device, &alloc_info, NULL, out_image_memory);
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to allocate memory for the image. Error code: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Bind image memory
    const VkResult result =
      vkBindImageMemory (info->device, info->image, *out_image_memory, 0);
    if (result != VK_SUCCESS)
    {
      vkFreeMemory (info->device, *out_image_memory, NULL);

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
moss_vk__transition_image_layout (const MossVk__TransitionImageLayoutInfo *const info)
{
  VkCommandBuffer command_buffer;
  {  // Begin one time command buffer
    const MossVk__BeginOneTimeCommandBufferInfo begin_info = {
      .device       = info->device,
      .command_pool = info->command_pool,
    };
    const MossResult result =
      moss_vk__begin_one_time_command_buffer (&begin_info, &command_buffer);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to begin one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  VkImageMemoryBarrier barrier = {
    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout           = info->old_layout,
    .newLayout           = info->new_layout,
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
  if (info->new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
  {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  // Determine source and destination
  VkPipelineStageFlags sourceStage      = 0;
  VkPipelineStageFlags destinationStage = 0;

  if (info->old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      info->new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (info->old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           info->new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (info->old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
           info->new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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
      info->old_layout,
      info->new_layout
    );
    return MOSS_RESULT_ERROR;
  }

  vkCmdPipelineBarrier (
    command_buffer,
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
    const MossVk__EndOneTimeCommandBufferInfo end_info = {
      .device         = info->device,
      .command_pool   = info->command_pool,
      .command_buffer = command_buffer,
      .queue          = info->transfer_queue,
    };
    const MossResult result = moss_vk__end_one_time_command_buffer (&end_info);

    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}
