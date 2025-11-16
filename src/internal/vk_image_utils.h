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

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"
#include "src/internal/vulkan/utils/command_buffer.h"

MossResult moss__transition_image_layout (
  const VkDevice      device,
  const VkCommandPool command_pool,
  const VkQueue       transfer_queue,
  const VkImage       image,
  const VkImageLayout old_layout,
  const VkImageLayout new_layout
)
{
  VkCommandBuffer command_buffer;
  {
    // Begin one time vk command buffer
    const Moss__BeginOneTimeVkCommandBufferInfo begin_info = {
      .device       = device,
      .command_pool = command_pool,
    };
    command_buffer = moss__begin_one_time_vk_command_buffer (&begin_info);

    if (command_buffer == VK_NULL_HANDLE)
    {
      moss__error ("Failed to begin one time Vulkan command buffer.\n");
      return MOSS_RESULT_ERROR;
    }
  }


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

  {  // End one time command buffer
    const Moss__EndOneTimeVkCommandBufferInfo end_info = {
      .device         = device,
      .command_pool   = command_pool,
      .command_buffer = command_buffer,
      .queue          = transfer_queue,
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
