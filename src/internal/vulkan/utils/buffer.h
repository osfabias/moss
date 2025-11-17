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
#include <string.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/log.h"
#include "src/internal/memory_utils.h"
#include "src/internal/vulkan/utils/command_buffer.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Info required to create a buffer.
*/
typedef struct
{
  VkPhysicalDevice      physical_device;
  VkDevice              device;
  VkDeviceSize          size;
  VkBufferUsageFlags    usage;
  VkMemoryPropertyFlags memory_properties;
  VkSharingMode         sharing_mode;
  uint32_t              shared_queue_family_index_count;
  const uint32_t       *shared_queue_family_indices;
} Moss__CreateVkBufferInfo;

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
  @brief Info required to fill a buffer with data from host memory.
*/
typedef struct
{
  VkPhysicalDevice physical_device;
  VkDevice         device;
  VkBuffer         destination_buffer;
  VkDeviceSize     buffer_size;
  const void      *source_data;
  VkDeviceSize     data_size;
  VkCommandPool    command_pool;
  VkQueue          transfer_queue;
  VkSharingMode    sharing_mode;
  uint32_t         shared_queue_family_index_count;
  const uint32_t  *shared_queue_family_indices;
} Moss__FillVkBufferInfo;

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


/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Creates a Vulkan buffer and allocates its memory.
  @param info Buffer creation info.
  @param out_buffer Output parameter for the created buffer.
  @param out_buffer_memory Output parameter for the allocated buffer memory.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__create_buffer (
  const Moss__CreateVkBufferInfo *const info,
  VkBuffer                             *out_buffer,
  VkDeviceMemory                       *out_buffer_memory
)
{
  {  // Create buffer
    const VkBufferCreateInfo buffer_info = {
      .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size                  = info->size,
      .usage                 = info->usage,
      .sharingMode           = info->sharing_mode,
      .queueFamilyIndexCount = info->shared_queue_family_index_count,
      .pQueueFamilyIndices   = info->shared_queue_family_indices,
    };

    const VkResult result = vkCreateBuffer (info->device, &buffer_info, NULL, out_buffer);
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to create buffer: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  // Get memory requirements
  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements (info->device, *out_buffer, &memory_requirements);

  // Find suitable memory type
  uint32_t memory_type_index;
  {
    const MossResult result = moss__select_suitable_memory_type (
      info->physical_device,
      memory_requirements.memoryTypeBits,
      info->memory_properties,
      &memory_type_index
    );
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyBuffer (info->device, *out_buffer, NULL);
      moss__error ("Failed to find suitable memory type for buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }


  {  // Allocate memory
    const VkMemoryAllocateInfo alloc_info = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize  = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    const VkResult result =
      vkAllocateMemory (info->device, &alloc_info, NULL, out_buffer_memory);
    if (result != VK_SUCCESS)
    {
      vkDestroyBuffer (info->device, *out_buffer, NULL);
      moss__error ("Failed to allocate buffer memory: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  // Bind memory to buffer
  vkBindBufferMemory (info->device, *out_buffer, *out_buffer_memory, 0);

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Destroys a Vulkan buffer and frees its memory.
  @param device Logical device.
  @param buffer Buffer to destroy.
  @param buffer_memory Memory to free.
*/
inline static void
moss_vk__destroy_buffer (VkDevice device, VkBuffer buffer, VkDeviceMemory buffer_memory)
{
  if (buffer_memory != VK_NULL_HANDLE) { vkFreeMemory (device, buffer_memory, NULL); }

  if (buffer != VK_NULL_HANDLE) { vkDestroyBuffer (device, buffer, NULL); }
}

/*
  @brief Copies data from one Vulkan buffer to another.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__copy_buffer (const Moss__CopyVkBufferInfo *const info)
{
  VkCommandBuffer command_buffer;
  {  // Begin one time vk command buffer
    const Moss__BeginOneTimeVkCommandBufferInfo begin_info = {
      .device       = info->device,
      .command_pool = info->command_pool,
    };
    command_buffer = moss_vk__begin_one_time_command_buffer (&begin_info);
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
    const MossResult result = moss_vk__end_one_time_command_buffer (&end_info);

    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time Vulkan command buffer.");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}


/*
  @brief Fills a buffer with data from host memory using a staging buffer.
  @param info Buffer fill info.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__fill_buffer (const Moss__FillVkBufferInfo *const info)
{
  // Create staging buffer
  VkBuffer       staging_buffer;
  VkDeviceMemory staging_buffer_memory;

  const Moss__CreateVkBufferInfo staging_create_info = {
    .physical_device = info->physical_device,
    .device          = info->device,
    .size            = info->data_size,
    .usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    .memory_properties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    .sharing_mode                    = info->sharing_mode,
    .shared_queue_family_index_count = info->shared_queue_family_index_count,
    .shared_queue_family_indices     = info->shared_queue_family_indices,
  };

  MossResult result = moss_vk__create_buffer (
    &staging_create_info,
    &staging_buffer,
    &staging_buffer_memory
  );
  if (result != MOSS_RESULT_SUCCESS)
  {
    moss__error ("Failed to create staging buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  // Map staging buffer memory and copy data
  void *mapped_memory;
  vkMapMemory (
    info->device,
    staging_buffer_memory,
    0,
    info->data_size,
    0,
    &mapped_memory
  );
  memcpy (mapped_memory, info->source_data, info->data_size);
  vkUnmapMemory (info->device, staging_buffer_memory);

  // Copy from staging buffer to destination buffer
  const Moss__CopyVkBufferInfo copy_info = {
    .device             = info->device,
    .destination_buffer = info->destination_buffer,
    .source_buffer      = staging_buffer,
    .size               = info->data_size,
    .command_pool       = info->command_pool,
    .transfer_queue     = info->transfer_queue,
  };

  result = moss_vk__copy_buffer (&copy_info);
  if (result != MOSS_RESULT_SUCCESS)
  {
    moss_vk__destroy_buffer (info->device, staging_buffer, staging_buffer_memory);
    moss__error ("Failed to copy buffer data.\n");
    return MOSS_RESULT_ERROR;
  }

  // Cleanup staging buffer
  moss_vk__destroy_buffer (info->device, staging_buffer, staging_buffer_memory);

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Copies data from Vulkan buffer to image.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult
moss_vk__copy_buffer_to_image (const Moss__CopyVkBufferToImageInfo *const info)
{
  VkCommandBuffer command_buffer;
  {  // Begin one time command buffer
    const Moss__BeginOneTimeVkCommandBufferInfo begin_info = {
      .device       = info->device,
      .command_pool = info->command_pool,
    };
    command_buffer = moss_vk__begin_one_time_command_buffer (&begin_info);

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

    .imageOffset = (VkOffset3D) {           0,            0, 0 },
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
    const MossResult result = moss_vk__end_one_time_command_buffer (&end_info);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}
