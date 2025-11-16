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

  @file src/internal/vulkan/utils/buffer.h
  @brief Vulkan buffers utility functions.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/log.h"
#include "src/internal/vulkan/utils/command_buffer.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Info required to perform copy operation on two buffers.
*/
typedef struct
{
  VkDevice      device;             /* Logical device. */
  VkBuffer      destination_buffer; /* Destination buffer to copy data to. */
  VkBuffer      source_buffer;      /* Source buffer to copy data from. */
  VkDeviceSize  size;               /* Amount of bytes to copy. */
  VkCommandPool command_pool;       /* Command pool to perfom operation with. */
  VkQueue       transfer_queue;     /* Queue to be used as a transfer queue. */
} Moss__CopyVkBufferInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Copies data from one Vulkan buffer to another.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss__copy_vk_buffer (const Moss__CopyVkBufferInfo *const info)
{
  VkCommandBuffer command_buffer;
  {  // Begin one time vk command buffer
    const Moss__BeginOneTimeVkCommandBufferInfo begin_info = {
      .device       = info->device,
      .command_pool = info->command_pool,
    };
    command_buffer = moss__begin_one_time_vk_command_buffer (&begin_info);
    if (command_buffer == VK_NULL_HANDLE)
    {
      moss__error ("Failed to begin one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  // Add copy command
  const VkBufferCopy copy_region = {
    .size      = info->size,
    .srcOffset = 0,
    .dstOffset = 0,
  };
  vkCmdCopyBuffer (
    command_buffer,
    info->source_buffer,
    info->destination_buffer,
    1,
    &copy_region
  );

  {  // End one time command buffer
    const Moss__EndOneTimeVkCommandBufferInfo end_info = {
      .device         = info->device,
      .command_pool   = info->command_pool,
      .command_buffer = command_buffer,
      .queue          = info->transfer_queue,
    };
    const MossResult result = moss__end_one_time_vk_command_buffer (&end_info);

    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time Vulkan command buffer.");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Required info to perform copy operation from buffer to image.
*/
typedef struct
{
  VkDevice      device;         /* Logical device. */
  VkCommandPool command_pool;   /* Command pool to perform operation with. */
  VkQueue       transfer_queue; /* Queue to be used as a transfer queue. */
  VkBuffer      buffer;         /* Source buffer to copy data from. */
  VkImage       image;          /* Destination image to copy data to. */
  uint32_t      width;          /* Image width. */
  uint32_t      height;         /* Image height. */
} Moss__CopyVkBufferToImageInfo;

/*
  @brief Copies data from Vulkan buffer to image.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult
moss__copy_buffer_to_image (const Moss__CopyVkBufferToImageInfo *const info)
{
  VkCommandBuffer command_buffer;
  {  // Begin one time command buffer
    const Moss__BeginOneTimeVkCommandBufferInfo begin_info = {
      .device       = info->device,
      .command_pool = info->command_pool,
    };
    command_buffer = moss__begin_one_time_vk_command_buffer (&begin_info);

    if (command_buffer == VK_NULL_HANDLE)
    {
      moss__error ("Failed to begin one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  const VkBufferImageCopy region = {
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,

    .imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel       = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount     = 1,

    .imageOffset = (VkOffset3D) { 0, 0, 0 },
    .imageExtent = (VkExtent3D) { info->width, info->height, 1 },
  };

  vkCmdCopyBufferToImage (
    command_buffer,
    info->buffer,
    info->image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  {  // End one time Vulkan command buffer
    const Moss__EndOneTimeVkCommandBufferInfo end_info = {
      .device         = info->device,
      .command_pool   = info->command_pool,
      .command_buffer = command_buffer,
      .queue          = info->transfer_queue,
    };
    const MossResult result = moss__end_one_time_vk_command_buffer (&end_info);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}
