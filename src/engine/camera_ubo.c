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
#include <string.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/config.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/buffer.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult moss__createCameraUboBuffers (MossEngine *const pEngine)
{
  const MossVk__CreateBufferInfo createInfo = {
    .device                      = pEngine->device,
    .size                        = sizeof (pEngine->camera),
    .usage                       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    .sharingMode                 = pEngine->bufferSharingMode,
    .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
    .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    {
      const MossResult result =
        mossVk__createBuffer (&createInfo, &pEngine->cameraUboBuffers[ i ]);
      if (result != MOSS_RESULT_SUCCESS)
      {
        for (uint32_t j = i; j >= 0; --j)
        {
          if (j < i)
          {
            if (pEngine->cameraUboMemories[ j ] != VK_NULL_HANDLE)
            {
              vkFreeMemory (pEngine->device, pEngine->cameraUboMemories[ j ], NULL);
            }
            if (pEngine->cameraUboBuffers[ j ] != VK_NULL_HANDLE)
            {
              vkDestroyBuffer (pEngine->device, pEngine->cameraUboBuffers[ j ], NULL);
            }
          }
          else {
            vkDestroyBuffer (pEngine->device, pEngine->cameraUboBuffers[ j ], NULL);
          }
        }
        moss__error ("Failed to create camera UBO buffer.\n");
        return result;
      }
    }

    {
      const MossVk__AllocateBufferMemoryInfo allocInfo = {
        .physicalDevice = pEngine->physicalDevice,
        .device         = pEngine->device,
        .buffer         = pEngine->cameraUboBuffers[ i ],
        .memoryProperties =
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      };
      const MossResult result =
        mossVk__allocateBufferMemory (&allocInfo, &pEngine->cameraUboMemories[ i ]);
      if (result != MOSS_RESULT_SUCCESS)
      {
        for (uint32_t j = i; j >= 0; --j)
        {
          if (pEngine->cameraUboMemories[ j ] != VK_NULL_HANDLE)
          {
            vkFreeMemory (pEngine->device, pEngine->cameraUboMemories[ j ], NULL);
          }
          if (pEngine->cameraUboBuffers[ j ] != VK_NULL_HANDLE)
          {
            vkDestroyBuffer (pEngine->device, pEngine->cameraUboBuffers[ j ], NULL);
          }
        }
        moss__error ("Failed to allocate camera UBO buffer memory.\n");
        return result;
      }
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements (
      pEngine->device,
      pEngine->cameraUboBuffers[ i ],
      &memoryRequirements
    );

    vkMapMemory (
      pEngine->device,
      pEngine->cameraUboMemories[ i ],
      0,
      memoryRequirements.size,
      0,
      &pEngine->cameraUboBufferMappedMemoryBlocks[ i ]
    );
  }

  return MOSS_RESULT_SUCCESS;
}

void moss_updateCameraUboData (MossEngine *const pEngine)
{
  memcpy (
    pEngine->cameraUboBufferMappedMemoryBlocks[ pEngine->currentFrame ],
    &pEngine->camera,
    sizeof (pEngine->camera)
  );
}
