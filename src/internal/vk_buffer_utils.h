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

  @file src/internal/vk_buffer_utils.h
  @brief Vulkan buffers utility functions.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/single_time_cmdbuf.h"

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

/*
  @brief Copies data from one Vulkan buffer to another.
  @param info Required info for operation execution.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss__copy_vk_buffer (const Moss__CopyVkBufferInfo *info)
{
  // Allocate command buffer
  VkCommandBuffer command_buffer =
    moss__begin_single_time_command_buffer (info->device, info->command_pool);

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

  moss__end_single_time_command_buffer (
    info->device,
    info->command_pool,
    command_buffer,
    info->transfer_queue
  );

  return MOSS_RESULT_SUCCESS;
}

inline static void moss__copy_buffer_to_image (
  VkDevice      device,
  VkCommandPool command_pool,
  VkQueue       transfer_queue,
  VkBuffer      buffer,
  VkImage       image,
  uint32_t      width,
  uint32_t      height
)
{
  VkCommandBuffer command_buffer =
    moss__begin_single_time_command_buffer (device, command_pool);

  VkBufferImageCopy region = {
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,

    .imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
    .imageSubresource.mipLevel       = 0,
    .imageSubresource.baseArrayLayer = 0,
    .imageSubresource.layerCount     = 1,

    .imageOffset = (VkOffset3D) {     0,      0, 0 },
    .imageExtent = (VkExtent3D) { width, height, 1 },
  };

  vkCmdCopyBufferToImage (
    command_buffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  moss__end_single_time_command_buffer (
    device,
    command_pool,
    command_buffer,
    transfer_queue
  );
}
