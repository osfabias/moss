/*
  Copyright 2025 Osfabias

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not time this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  @file src/internal/vulkan/utils/command_buffer.h
  @brief Vulkan command buffer utility functions.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/log.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Required info to begin one time Vulkan command buffer creation.
*/
typedef struct
{
  VkDevice      device;       /* Logical device to create command buffer on. */
  VkCommandPool command_pool; /* Command pool to create command buffer in. */
} MossVk__BeginOneTimeCommandBufferInfo;

/*
  @brief Required info to end one time Vulkan command buffer creation.
*/
typedef struct
{
  VkDevice        device;         /* Logical device to end command buffer on. */
  VkCommandPool   command_pool;   /* Command pool to free command buffer from. */
  VkCommandBuffer command_buffer; /* Command buffer to end. */
  VkQueue         queue;          /* Queue to submit command buffer to. */
} MossVk__EndOneTimeCommandBufferInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Begins one time command buffer.
  @param info Begin one time Vulkan command buffer info.
  @param out_command_buffer Output parameter for created command buffer.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__begin_one_time_command_buffer (
  const MossVk__BeginOneTimeCommandBufferInfo *const info,
  VkCommandBuffer *const                             out_command_buffer
)
{
  const VkCommandBufferAllocateInfo allocInfo = {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext              = NULL,
    .commandPool        = info->command_pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  VkCommandBuffer command_buffer;
  {  // Allocate command buffer
    const VkResult result =
      vkAllocateCommandBuffers (info->device, &allocInfo, &command_buffer);
    if (result != VK_SUCCESS)
    {
      moss__error (
        "Failed to allocate command buffer for one time time. Error code: %d.\n",
        result
      );
      return MOSS_RESULT_ERROR;
    }
  }

  static const VkCommandBufferBeginInfo beginInfo = {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext            = NULL,
    .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = NULL,
  };

  {  // Begin command buffer
    const VkResult result = vkBeginCommandBuffer (command_buffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
      vkFreeCommandBuffers (info->device, info->command_pool, 1, &command_buffer);
      moss__error ("Failed to begin one time command buffer. Error code: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  *out_command_buffer = command_buffer;
  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Ends one time command buffer.
  @param info End one time Vulkan command buffer info.
  @return MOSS_RESULT_SUCCESS on success, otherwise MOSS_RESULT_ERROR.
*/
inline static MossResult moss_vk__end_one_time_command_buffer (
  const MossVk__EndOneTimeCommandBufferInfo *const info
)
{
  {  // End command buffer
    const VkResult result = vkEndCommandBuffer (info->command_buffer);
    if (result != VK_SUCCESS)
    {
      moss__error (
        "Failed to end one time command buffer (%p). Error code: %d.\n",
        (void *)info->command_buffer,
        result
      );
      return MOSS_RESULT_ERROR;
    }
  }

  const VkSubmitInfo submitInfo = {
    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext                = NULL,
    .waitSemaphoreCount   = 0,
    .pWaitSemaphores      = NULL,
    .commandBufferCount   = 1,
    .pCommandBuffers      = &info->command_buffer,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores    = NULL,
  };

  {  // Submit queue
    const VkResult result = vkQueueSubmit (info->queue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
      moss__error (
        "Failed to submit one time command buffer (%p). Error code: %d.\n",
        (void *)info->command_buffer,
        result
      );
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Wait queue idle
    const VkResult result = vkQueueWaitIdle (info->queue);
    if (result != VK_SUCCESS)
    {
      moss__error (
        "Failed to wait idle one time command buffer queue (%p). Error code: %d.\n",
        (void *)info->queue,
        result
      );
      return MOSS_RESULT_ERROR;
    }
  }

  vkFreeCommandBuffers (info->device, info->command_pool, 1, &info->command_buffer);

  return MOSS_RESULT_SUCCESS;
}
