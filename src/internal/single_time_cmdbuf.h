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

  @file src/internal/single_time_cmdbuf.h
  @brief Singl time command buffer functions.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

inline static VkCommandBuffer moss__begin_single_time_command_buffer (
  const VkDevice      device,
  const VkCommandPool command_pool
)
{
  const VkCommandBufferAllocateInfo allocInfo = {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandPool        = command_pool,
    .commandBufferCount = 1,
  };

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers (device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer (commandBuffer, &beginInfo);

  return commandBuffer;
}

inline static void moss__end_single_time_command_buffer (
  const VkDevice        device,
  const VkCommandPool   command_pool,
  const VkCommandBuffer commandBuffer,
  const VkQueue         transfer_queue
)
{
  vkEndCommandBuffer (commandBuffer);

  VkSubmitInfo submitInfo = {
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers    = &commandBuffer,
  };

  vkQueueSubmit (transfer_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle (transfer_queue);

  vkFreeCommandBuffers (device, command_pool, 1, &commandBuffer);
}
