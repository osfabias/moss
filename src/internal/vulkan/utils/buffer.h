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
#include "src/internal/vulkan/utils/memory.h"
#include "src/internal/vulkan/utils/command_buffer.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Info required to create a buffer.
*/
typedef struct
{
  VkDevice           device;                /* Logical device to create the buffer on. */
  VkDeviceSize       size;                  /* Buffer size in bytes. */
  VkBufferUsageFlags usage;                 /* Buffer usage flags. */
  VkSharingMode      sharing_mode;          /* Sharing mode. */
  uint32_t shared_queue_family_index_count; /* Number of queue family indices that will
                                               share the buffer. */
  const uint32_t *shared_queue_family_indices; /* Array of queue family indices that will
                                                  share the buffer. */
} MossVk__CreateBufferInfo;

/*
  @brief Info required to allocate memory for a buffer.
*/
typedef struct
{
  VkPhysicalDevice physical_device; /* Physical device to query memory properties on. */
  VkDevice         device;          /* Logical device to allocate memory on. */
  VkBuffer         buffer;          /* Buffer to allocate memory for. */
  VkMemoryPropertyFlags memory_properties; /* Memory property flags. */
} MossVk__AllocateBufferMemoryInfo;

/*
  @brief Info required to perform copy operation on two buffers.
*/
typedef struct
{
  VkDevice      device;             /* Logical device to perform copy operation on. */
  VkBuffer      destination_buffer; /* Destination buffer. */
  VkBuffer      source_buffer;      /* Source buffer. */
  VkDeviceSize  size;               /* Number of bytes to copy. */
  VkCommandPool command_pool;       /* Command pool to allocate command buffer from. */
  VkQueue       transfer_queue;     /* Queue to submit command buffer to. */
} MossVk__CopyBufferInfo;

/*
  @brief Info required to fill a buffer with data from host memory.
*/
typedef struct
{
  VkPhysicalDevice
           physical_device;    /* Physical device to allocate staging buffer memory on. */
  VkDevice device;             /* Logical device to create staging buffer on. */
  VkBuffer destination_buffer; /* Destination buffer. */
  VkDeviceSize    buffer_size; /* Size of the destination buffer. */
  const void     *source_data; /* Source data in host memory. */
  VkDeviceSize    data_size;   /* Size of source data in bytes. */
  VkCommandPool   command_pool;   /* Command pool to allocate command buffer from. */
  VkQueue         transfer_queue; /* Queue to submit command buffer to. */
  VkSharingMode   sharing_mode;   /* Sharing mode. */
  uint32_t        shared_queue_family_index_count; /* Number of queue family indices. */
  const uint32_t *shared_queue_family_indices;     /* Array of queue family indices. */
} MossVk__FillBufferInfo;

/*
  @brief Required info to perform copy operation from buffer to image.
*/
typedef struct
{
  VkDevice      device;         /* Logical device to perform copy operation on. */
  VkCommandPool command_pool;   /* Command pool to allocate command buffer from. */
  VkQueue       transfer_queue; /* Queue to submit command buffer to. */
  VkBuffer      buffer;         /* Source buffer. */
  VkImage       image;          /* Destination image. */
  uint32_t      width;          /* Image width in pixels. */
  uint32_t      height;         /* Image height in pixels. */
} MossVk__CopyBufferToImageInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Creates a Vulkan buffer.
  @param info Buffer creation info.
  @param out_buffer Output parameter for the created buffer.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__create_buffer (
  const MossVk__CreateBufferInfo *const info,
  VkBuffer *const                       out_buffer
)
{
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

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Allocates and binds memory for a Vulkan buffer.
  @param info Buffer memory allocation info.
  @param out_buffer_memory Output parameter for allocated memory handle.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__allocate_buffer_memory (
  const MossVk__AllocateBufferMemoryInfo *const info,
  VkDeviceMemory *const                         out_buffer_memory
)
{
  // Get memory requirements
  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements (info->device, info->buffer, &memory_requirements);

  // Find suitable memory type
  uint32_t memory_type_index;
  {
    const Moss__SelectSuitableMemoryTypeInfo select_info = {
      .physical_device = info->physical_device,
      .type_filter     = memory_requirements.memoryTypeBits,
      .properties      = info->memory_properties,
    };
    const MossResult result = moss__select_suitable_memory_type (&select_info, &memory_type_index);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to find suitable memory type for buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  // Allocate memory
  {
    const VkMemoryAllocateInfo alloc_info = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize  = memory_requirements.size,
      .memoryTypeIndex = memory_type_index,
    };

    const VkResult result =
      vkAllocateMemory (info->device, &alloc_info, NULL, out_buffer_memory);
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to allocate buffer memory: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  // Bind memory to buffer
  vkBindBufferMemory (info->device, info->buffer, *out_buffer_memory, 0);

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Copies data from one Vulkan buffer to another.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__copy_buffer (const MossVk__CopyBufferInfo *const info)
{
  VkCommandBuffer command_buffer;
  {  // Begin one time vk command buffer
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
    const MossVk__EndOneTimeCommandBufferInfo end_info = {
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
inline static MossResult moss_vk__fill_buffer (const MossVk__FillBufferInfo *const info)
{
  VkBuffer       staging_buffer;
  VkDeviceMemory staging_buffer_memory;

  {  // Create staging buffer
    const MossVk__CreateBufferInfo staging_create_info = {
      .device                          = info->device,
      .size                            = info->data_size,
      .usage                           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharing_mode                    = info->sharing_mode,
      .shared_queue_family_index_count = info->shared_queue_family_index_count,
      .shared_queue_family_indices     = info->shared_queue_family_indices,
    };

    const MossResult result =
      moss_vk__create_buffer (&staging_create_info, &staging_buffer);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create staging buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Allocate buffer memory
    const MossVk__AllocateBufferMemoryInfo alloc_info = {
      .physical_device = info->physical_device,
      .device          = info->device,
      .buffer          = staging_buffer,
      .memory_properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    const MossResult result =
      moss_vk__allocate_buffer_memory (&alloc_info, &staging_buffer_memory);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyBuffer (info->device, staging_buffer, NULL);
      moss__error ("Failed to allocate staging buffer memory.\n");
      return MOSS_RESULT_ERROR;
    }
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

  {  // Copy from staging buffer to destination buffer
    const MossVk__CopyBufferInfo copy_info = {
      .device             = info->device,
      .destination_buffer = info->destination_buffer,
      .source_buffer      = staging_buffer,
      .size               = info->data_size,
      .command_pool       = info->command_pool,
      .transfer_queue     = info->transfer_queue,
    };

    const MossResult result = moss_vk__copy_buffer (&copy_info);
    if (result != MOSS_RESULT_SUCCESS)
    {
      if (staging_buffer_memory != VK_NULL_HANDLE)
      {
        vkFreeMemory (info->device, staging_buffer_memory, NULL);
      }
      if (staging_buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (info->device, staging_buffer, NULL);
      }
      moss__error ("Failed to copy buffer data.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Cleanup staging buffer
    if (staging_buffer_memory != VK_NULL_HANDLE)
    {
      vkFreeMemory (info->device, staging_buffer_memory, NULL);
    }
    if (staging_buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer (info->device, staging_buffer, NULL);
    }
  }

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Copies data from Vulkan buffer to image.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult
moss_vk__copy_buffer_to_image (const MossVk__CopyBufferToImageInfo *const info)
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
