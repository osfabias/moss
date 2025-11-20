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

  @file src/internal/vertex.h
  @brief Internal vertex utility function declarations.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <vulkan/vulkan.h>

#include "vulkan/vulkan_core.h"

/*
  @brief Vertex.
  @warning Whenever you change this struct, please adjust binding and attribute
           descriptions.
*/
typedef struct
{
  vec3 position;       /* Vertex position. */
  vec2 texture_coords; /* Texture coordinates. */
} Moss__Vertex;

/* VkVertexBindingDescription pack. */
typedef struct
{
  /* Binding descriptions. */
  const VkVertexInputBindingDescription *const descriptions;

  /* Description count. */
  const size_t count;
} Moss__VkVertexInputBindingDescriptionPack;

/* VkVertexAttributeDescription pack. */
typedef struct
{
  /* Attribute descriptions. */
  const VkVertexInputAttributeDescription *const descriptions;

  /* Description count. */
  const size_t count;
} Moss__VkVertexInputAttributeDescriptionPack;

/*
  @brief Returns Vulkan input binding description that corresponds to the @ref MossVertex.
  @return Vulkan input binding description.
*/
inline static Moss__VkVertexInputBindingDescriptionPack
moss__getVkVertexInputBindingDescription (void)
{
  static const VkVertexInputBindingDescription bindingDescriptions[] = {
    {
     .binding   = 0,
     .stride    = sizeof (Moss__Vertex),
     .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
     },
  };

  static const Moss__VkVertexInputBindingDescriptionPack descriptionsPack = {
    .count        = sizeof (bindingDescriptions) / sizeof (bindingDescriptions[ 0 ]),
    .descriptions = bindingDescriptions,
  };

  return descriptionsPack;
}

/*
  @brief Returns Vulkan input attribute descipritions that corresponds to the @ref
  MossVertex fields.
  @return Vulkan input attribute descriptions.
*/
inline static Moss__VkVertexInputAttributeDescriptionPack
moss__getVkVertexInputAttributeDescription (void)
{
  static const VkVertexInputAttributeDescription attributeDescriptions[] = {
    {
     .binding  = 0,
     .location = 0,
     .format   = VK_FORMAT_R32G32B32_SFLOAT,
     .offset   = offsetof (Moss__Vertex,       position),
     },
    {
     .binding  = 0,
     .location = 1,
     .format   = VK_FORMAT_R32G32_SFLOAT,
     .offset   = offsetof (Moss__Vertex, texture_coords),
     }
  };

  static const Moss__VkVertexInputAttributeDescriptionPack descriptionsPack = {
    .descriptions = attributeDescriptions,
    .count = sizeof (attributeDescriptions) / sizeof (attributeDescriptions[ 0 ]),
  };

  return descriptionsPack;
}
