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

MossResult moss__createGeneralCommandBuffers (MossEngine *const pEngine)
{
  const VkCommandBufferAllocateInfo allocInfo = {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = pEngine->generalCommandPool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };

  const VkResult result = vkAllocateCommandBuffers (
    pEngine->device,
    &allocInfo,
    pEngine->generalCommandBuffers
  );
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to allocate command buffers. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

