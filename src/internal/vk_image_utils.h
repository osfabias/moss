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

  @file src/internal/vk_image_utils.h
  @brief Vulkan image utility functions.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "src/internal/single_time_cmdbuf.h"
#include "vulkan/vulkan_core.h"
#include <stdint.h>

#include <vulkan/vulkan.h>

void moss__transition_image_layout (
  const VkDevice      device,
  const VkCommandPool command_pool,
  const VkQueue       transfer_queue,
  const VkImage       image,
  const VkImageLayout old_layout,
  const VkImageLayout new_layout
)
{
  VkCommandBuffer command_buffer =
    moss__begin_single_time_command_buffer (device, command_pool);

  VkImageMemoryBarrier barrier = {
    .sType     = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .srcAccessMask = 0,
    .dstAccessMask = 0,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };

  VkPipelineStageFlags sourceStage      = 0;
  VkPipelineStageFlags destinationStage = 0;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
      new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

  moss__end_single_time_command_buffer (
    device,
    command_pool,
    command_buffer,
    transfer_queue
  );
}
