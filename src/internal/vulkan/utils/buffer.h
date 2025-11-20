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
  VkSharingMode      sharingMode;           /* Sharing mode. */
  uint32_t sharedQueueFamilyIndexCount;     /* Number of queue family indices that will
                                               share the buffer. */
  const uint32_t *sharedQueueFamilyIndices; /* Array of queue family indices that will
                                               share the buffer. */
} MossVk__CreateBufferInfo;

/*
  @brief Info required to allocate memory for a buffer.
*/
typedef struct
{
  VkPhysicalDevice physicalDevice;  /* Physical device to query memory properties on. */
  VkDevice         device;          /* Logical device to allocate memory on. */
  VkBuffer         buffer;          /* Buffer to allocate memory for. */
  VkMemoryPropertyFlags memoryProperties; /* Memory property flags. */
} MossVk__AllocateBufferMemoryInfo;

/*
  @brief Info required to perform copy operation on two buffers.
*/
typedef struct
{
  VkDevice      device;          /* Logical device to perform copy operation on. */
  VkBuffer      destinationBuffer; /* Destination buffer. */
  VkBuffer      sourceBuffer;    /* Source buffer. */
  VkDeviceSize  size;            /* Number of bytes to copy. */
  VkCommandPool commandPool;     /* Command pool to allocate command buffer from. */
  VkQueue       transferQueue;   /* Queue to submit command buffer to. */
} MossVk__CopyBufferInfo;

/*
  @brief Info required to fill a buffer with data from host memory.
*/
typedef struct
{
  VkPhysicalDevice
           physicalDevice;    /* Physical device to allocate staging buffer memory on. */
  VkDevice device;            /* Logical device to create staging buffer on. */
  VkBuffer destinationBuffer; /* Destination buffer. */
  VkDeviceSize    bufferSize; /* Size of the destination buffer. */
  const void     *sourceData; /* Source data in host memory. */
  VkDeviceSize    dataSize;   /* Size of source data in bytes. */
  VkCommandPool   commandPool;   /* Command pool to allocate command buffer from. */
  VkQueue         transferQueue; /* Queue to submit command buffer to. */
  VkSharingMode   sharingMode;   /* Sharing mode. */
  uint32_t        sharedQueueFamilyIndexCount; /* Number of queue family indices. */
  const uint32_t *sharedQueueFamilyIndices;    /* Array of queue family indices. */
} MossVk__FillBufferInfo;

/*
  @brief Required info to perform copy operation from buffer to image.
*/
typedef struct
{
  VkDevice      device;       /* Logical device to perform copy operation on. */
  VkCommandPool commandPool;  /* Command pool to allocate command buffer from. */
  VkQueue       transferQueue; /* Queue to submit command buffer to. */
  VkBuffer      buffer;       /* Source buffer. */
  VkImage       image;        /* Destination image. */
  uint32_t      width;        /* Image width in pixels. */
  uint32_t      height;       /* Image height in pixels. */
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
inline static MossResult mossVk__createBuffer (
  const MossVk__CreateBufferInfo *const info,
  VkBuffer *const                       outBuffer
)
{
  const VkBufferCreateInfo bufferInfo = {
    .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size                  = info->size,
    .usage                 = info->usage,
    .sharingMode           = info->sharingMode,
    .queueFamilyIndexCount = info->sharedQueueFamilyIndexCount,
    .pQueueFamilyIndices   = info->sharedQueueFamilyIndices,
  };

  const VkResult result = vkCreateBuffer (info->device, &bufferInfo, NULL, outBuffer);
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
inline static MossResult mossVk__allocateBufferMemory (
  const MossVk__AllocateBufferMemoryInfo *const info,
  VkDeviceMemory *const                         outBufferMemory
)
{
  // Get memory requirements
  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements (info->device, info->buffer, &memoryRequirements);

  // Find suitable memory type
  uint32_t memoryTypeIndex;
  {
    const Moss__SelectSuitableMemoryTypeInfo selectInfo = {
      .physicalDevice = info->physicalDevice,
      .typeFilter     = memoryRequirements.memoryTypeBits,
      .properties     = info->memoryProperties,
    };
    const MossResult result = moss__selectSuitableMemoryType (&selectInfo, &memoryTypeIndex);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to find suitable memory type for buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  // Allocate memory
  {
    const VkMemoryAllocateInfo allocInfo = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize  = memoryRequirements.size,
      .memoryTypeIndex = memoryTypeIndex,
    };

    const VkResult result =
      vkAllocateMemory (info->device, &allocInfo, NULL, outBufferMemory);
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to allocate buffer memory: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  // Bind memory to buffer
  vkBindBufferMemory (info->device, info->buffer, *outBufferMemory, 0);

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Copies data from one Vulkan buffer to another.
  @param info Required info to perform copy.
  @return Return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult mossVk__copyBuffer (const MossVk__CopyBufferInfo *const info)
{
  VkCommandBuffer commandBuffer;
  {  // Begin one time vk command buffer
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

  // Add copy command
  const VkBufferCopy copyRegion = {
    .size      = info->size,
    .srcOffset = 0,
    .dstOffset = 0,
  };
  vkCmdCopyBuffer (
    commandBuffer,
    info->sourceBuffer,
    info->destinationBuffer,
    1,
    &copyRegion
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
inline static MossResult mossVk__fillBuffer (const MossVk__FillBufferInfo *const info)
{
  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  {  // Create staging buffer
    const MossVk__CreateBufferInfo stagingCreateInfo = {
      .device                    = info->device,
      .size                      = info->dataSize,
      .usage                     = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode               = info->sharingMode,
      .sharedQueueFamilyIndexCount = info->sharedQueueFamilyIndexCount,
      .sharedQueueFamilyIndices   = info->sharedQueueFamilyIndices,
    };

    const MossResult result =
      mossVk__createBuffer (&stagingCreateInfo, &stagingBuffer);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create staging buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Allocate buffer memory
    const MossVk__AllocateBufferMemoryInfo allocInfo = {
      .physicalDevice = info->physicalDevice,
      .device         = info->device,
      .buffer         = stagingBuffer,
      .memoryProperties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    const MossResult result =
      mossVk__allocateBufferMemory (&allocInfo, &stagingBufferMemory);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyBuffer (info->device, stagingBuffer, NULL);
      moss__error ("Failed to allocate staging buffer memory.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  // Map staging buffer memory and copy data
  void *mappedMemory;
  vkMapMemory (
    info->device,
    stagingBufferMemory,
    0,
    info->dataSize,
    0,
    &mappedMemory
  );
  memcpy (mappedMemory, info->sourceData, info->dataSize);
  vkUnmapMemory (info->device, stagingBufferMemory);

  {  // Copy from staging buffer to destination buffer
    const MossVk__CopyBufferInfo copyInfo = {
      .device          = info->device,
      .destinationBuffer = info->destinationBuffer,
      .sourceBuffer    = stagingBuffer,
      .size            = info->dataSize,
      .commandPool     = info->commandPool,
      .transferQueue   = info->transferQueue,
    };

    const MossResult result = mossVk__copyBuffer (&copyInfo);
    if (result != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (info->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (info->device, stagingBuffer, NULL);
      }
      moss__error ("Failed to copy buffer data.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Cleanup staging buffer
    if (stagingBufferMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory (info->device, stagingBufferMemory, NULL);
    }
    if (stagingBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer (info->device, stagingBuffer, NULL);
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
mossVk__copyBufferToImage (const MossVk__CopyBufferToImageInfo *const info)
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
    commandBuffer,
    info->buffer,
    info->image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
  );

  {  // End one time Vulkan command buffer
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
