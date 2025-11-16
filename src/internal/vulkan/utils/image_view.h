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

  @file src/internal/vulkan/utils/image_view.h
  @brief Vulkan image view creation utility function.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <vulkan/vulkan.h>

#include "src/internal/log.h"
#include "vulkan/vulkan_core.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Required info for Vulkan image view creation.
*/
typedef struct
{
  VkDevice device; /* Device to create image view on. */
  VkImage  image;  /* Image to create view for. */
  VkFormat format; /* Image view format. */
} Moss__VkImageViewCreateInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Creates Vulkan image view instance.
  @param info Vulkan image view creation info.
  @return On success returns valid image view handler, otherwise VK_NULL_HANDLE.
*/
inline static VkImageView
moss__create_vk_image_view (const Moss__VkImageViewCreateInfo *const info)
{
  static const VkImageSubresourceRange subresource_range = {
    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
    .baseMipLevel   = 0,
    .levelCount     = 1,
    .baseArrayLayer = 0,
    .layerCount     = 1,
  };

  static const VkComponentMapping components = {
    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
  };

  const VkImageViewCreateInfo create_info = {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext            = NULL,
    .flags            = 0,
    .image            = info->image,
    .viewType         = VK_IMAGE_VIEW_TYPE_2D,
    .format           = info->format,
    .components       = components,
    .subresourceRange = subresource_range,
  };

  VkImageView    image_view;
  const VkResult result =
    vkCreateImageView (info->device, &create_info, NULL, &image_view);

  if (result != VK_SUCCESS)
  {
    moss__error (
      "Failed to create image view for %p image. Error code: %d.\n",
      (void *)info->image,
      result
    );
    return VK_NULL_HANDLE;
  }
  return image_view;
}
