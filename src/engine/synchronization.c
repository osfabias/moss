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

MossResult moss__createImageAvailableSemaphores (MossEngine *const pEngine)
{
  if (pEngine->device == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  const VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const VkResult result = vkCreateSemaphore (
      pEngine->device,
      &semaphore_info,
      NULL,
      &pEngine->imageAvailableSemaphores[ i ]
    );
    if (result == VK_SUCCESS) { continue; }

    moss__error ("Failed to create image available semaphore for frame %u.\n", i);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createRenderFinishedSemaphores (MossEngine *const pEngine)
{
  if (pEngine->device == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  const VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const VkResult result = vkCreateSemaphore (
      pEngine->device,
      &semaphore_info,
      NULL,
      &pEngine->renderFinishedSemaphores[ i ]
    );
    if (result == VK_SUCCESS) { continue; }

    moss__error ("Failed to create render finished semaphore for frame %u.\n", i);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createInFlightFences (MossEngine *const pEngine)
{
  if (pEngine->device == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  const VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const VkResult result =
      vkCreateFence (pEngine->device, &fence_info, NULL, &pEngine->inFlightFences[ i ]);
    if (result == VK_SUCCESS) { continue; }

    moss__error ("Failed to create in-flight fence for frame %u.\n", i);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

MossResult moss__createSynchronizationObjects (MossEngine *const pEngine)
{
  if (moss__createImageAvailableSemaphores (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  if (moss__createRenderFinishedSemaphores (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  if (moss__createInFlightFences (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

void moss__cleanupSemaphores (
  MossEngine *const pEngine,
  VkSemaphore       pSemaphores[ MAX_FRAMES_IN_FLIGHT ]
)
{
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (pSemaphores[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroySemaphore (pEngine->device, pSemaphores[ i ], NULL);
    pSemaphores[ i ] = VK_NULL_HANDLE;
  }
}

void
moss__cleanupFences (MossEngine *const pEngine, VkFence pFences[ MAX_FRAMES_IN_FLIGHT ])
{
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (pFences[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyFence (pEngine->device, pFences[ i ], NULL);
    pFences[ i ] = VK_NULL_HANDLE;
  }
}

