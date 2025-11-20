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
*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/config.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult moss__createDescriptorPool (MossEngine *const pEngine)
{
  const VkDescriptorPoolSize pool_sizes[] = {
    {
     .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     .descriptorCount = MAX_FRAMES_IN_FLIGHT,
     },
    {
     .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
     .descriptorCount = MAX_FRAMES_IN_FLIGHT,
     }
  };

  const VkDescriptorPoolCreateInfo create_info = {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = sizeof (pool_sizes) / sizeof (pool_sizes[ 0 ]),
    .pPoolSizes    = pool_sizes,
    .maxSets       = MAX_FRAMES_IN_FLIGHT,
    .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
  };

  const VkResult result = vkCreateDescriptorPool (
    pEngine->device,
    &create_info,
    NULL,
    &pEngine->descriptorPool
  );
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create descriptor pool: %d.", result);
    return MOSS_RESULT_ERROR;
  }
  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createDescriptorSetLayout (MossEngine *const pEngine)
{
  const VkDescriptorSetLayoutBinding layout_bindings[] = {
    {
     .binding         = 0,
     .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     .descriptorCount = 1,
     .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT,
     },
    {
     .binding         = 1,
     .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
     .descriptorCount = 1,
     .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
     },
  };

  const VkDescriptorSetLayoutCreateInfo create_info = {
    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = sizeof (layout_bindings) / sizeof (layout_bindings[ 0 ]),
    .pBindings    = layout_bindings,
  };

  const VkResult result = vkCreateDescriptorSetLayout (
    pEngine->device,
    &create_info,
    NULL,
    &pEngine->descriptorSetLayout
  );
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create Vulkan descriptor layout: %d.", result);
    return MOSS_RESULT_ERROR;
  }
  return MOSS_RESULT_SUCCESS;
}

MossResult moss_allocateDescriptorSets (MossEngine *const pEngine)
{
  VkDescriptorSetLayout layouts[ MAX_FRAMES_IN_FLIGHT ];
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    layouts[ i ] = pEngine->descriptorSetLayout;
  }

  const VkDescriptorSetAllocateInfo alloc_info = {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
    .pSetLayouts        = layouts,
    .descriptorPool     = pEngine->descriptorPool
  };

  const VkResult result =
    vkAllocateDescriptorSets (pEngine->device, &alloc_info, pEngine->descriptorSets);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to allocate descriptor sets: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

void moss__configureDescriptorSets (MossEngine *const pEngine)
{
  VkDescriptorBufferInfo pBufferInfos[ MAX_FRAMES_IN_FLIGHT ];
  VkDescriptorImageInfo  pImageInfos[ MAX_FRAMES_IN_FLIGHT ];
  VkWriteDescriptorSet   pDescriptorWrites[ MAX_FRAMES_IN_FLIGHT * 2 ];

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    VkMemoryRequirements pMemoryRequirements;
    vkGetBufferMemoryRequirements (
      pEngine->device,
      pEngine->cameraUboBuffers[ i ],
      &pMemoryRequirements
    );

    pBufferInfos[ i ] = (VkDescriptorBufferInfo) {
      .offset = 0,
      .buffer = pEngine->cameraUboBuffers[ i ],
      .range  = pMemoryRequirements.size,
    };

    pDescriptorWrites[ i ] = (VkWriteDescriptorSet) {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = pEngine->descriptorSets[ i ],
      .dstBinding      = 0,
      .dstArrayElement = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo     = &pBufferInfos[ i ],
    };
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    pImageInfos[ i ] = (VkDescriptorImageInfo) {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .sampler     = pEngine->sampler,
      .imageView   = pEngine->textureImageView,
    };

    pDescriptorWrites[ MAX_FRAMES_IN_FLIGHT + i ] = (VkWriteDescriptorSet) {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = pEngine->descriptorSets[ i ],
      .dstBinding      = 1,
      .dstArrayElement = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .pImageInfo      = &pImageInfos[ i ],
    };
  }

  vkUpdateDescriptorSets (
    pEngine->device,
    sizeof (pDescriptorWrites) / sizeof (pDescriptorWrites[ 0 ]),
    pDescriptorWrites,
    0,
    NULL
  );
}

