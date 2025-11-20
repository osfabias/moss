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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "moss/app_info.h"
#include "moss/engine.h"
#include "moss/result.h"

#include "src/internal/config.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/command_pool.h"
#include "src/internal/vulkan/utils/physical_device.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    PUBLIC API FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult
mossCreateEngine (const MossEngineConfig *const pConfig, MossEngine **const ppOutEngine)
{
  MossEngine *const pEngine = malloc (sizeof (MossEngine));
  if (pEngine == NULL)
  {
    moss__error ("Failed to allocate memory for engine.\n");
    return MOSS_RESULT_ERROR;
  }

  moss__initEngineState (pEngine);

#ifdef __APPLE__
  // Store metalLayer from config
  pEngine->metalLayer = pConfig->metalLayer;
#endif

  // Store framebuffer size callback
  pEngine->getWindowFramebufferSize = pConfig->getWindowFramebufferSize;

  if (moss__createApiInstance (pEngine, pConfig->appInfo) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createSurface (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  {
    const MossVk__SelectPhysicalDeviceInfo selectInfo = {
      .instance  = pEngine->apiInstance,
      .surface   = pEngine->surface,
      .outDevice = &pEngine->physicalDevice,
    };
    if (mossVk__selectPhysicalDevice (&selectInfo) != VK_SUCCESS)
    {
      mossDestroyEngine ((MossEngine *)pEngine);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__FindQueueFamiliesInfo findInfo = {
      .device  = pEngine->physicalDevice,
      .surface = pEngine->surface,
    };
    pEngine->queueFamilyIndices = mossVk__findQueueFamilies (&findInfo);
  }

  if (moss__createLogicalDevice (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  vkGetDeviceQueue (
    pEngine->device,
    pEngine->queueFamilyIndices.graphicsFamily,
    0,
    &pEngine->graphicsQueue
  );

  vkGetDeviceQueue (
    pEngine->device,
    pEngine->queueFamilyIndices.presentFamily,
    0,
    &pEngine->presentQueue
  );

  vkGetDeviceQueue (
    pEngine->device,
    pEngine->queueFamilyIndices.transferFamily,
    0,
    &pEngine->transferQueue
  );

  moss_initBufferSharingMode (pEngine);

  // Get framebuffer size from callback
  VkExtent2D extent;
  pEngine->getWindowFramebufferSize (&extent.width, &extent.height);

  if (moss__createSwapchain (pEngine, extent) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createSwapchainImageViews (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createRenderPass (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createCameraUboBuffers (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  // Create general command pool
  {
    const MossVk__CreateCommandPoolInfo createInfo = {
      .device           = pEngine->device,
      .queueFamilyIndex = pEngine->queueFamilyIndices.graphicsFamily,
    };
    if (mossVk__createCommandPool (&createInfo, &pEngine->generalCommandPool) !=
        MOSS_RESULT_SUCCESS)
    {
      mossDestroyEngine ((MossEngine *)pEngine);
      return MOSS_RESULT_ERROR;
    }
  }

  // Create transfer command pool
  {
    const MossVk__CreateCommandPoolInfo createInfo = {
      .device           = pEngine->device,
      .queueFamilyIndex = pEngine->queueFamilyIndices.transferFamily,
    };
    if (mossVk__createCommandPool (&createInfo, &pEngine->transferCommandPool) !=
        MOSS_RESULT_SUCCESS)
    {
      mossDestroyEngine ((MossEngine *)pEngine);
      return MOSS_RESULT_ERROR;
    }
  }

  if (moss__createTextureImage (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createTextureImageView (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createTextureSampler (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createDepthResources (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createDescriptorPool (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createDescriptorSetLayout (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss_allocateDescriptorSets (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  moss__configureDescriptorSets (pEngine);

  if (moss__createGraphicsPipeline (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createPresentFramebuffers (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createGeneralCommandBuffers (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  if (moss__createSynchronizationObjects (pEngine) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine ((MossEngine *)pEngine);
    return MOSS_RESULT_ERROR;
  }

  pEngine->currentFrame = 0;

  // Initialize camera UBO data for all frames
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    memcpy (
      pEngine->cameraUboBufferMappedMemoryBlocks[ i ],
      &pEngine->camera,
      sizeof (pEngine->camera)
    );
  }

  *ppOutEngine = pEngine;
  return MOSS_RESULT_SUCCESS;
}

void mossDestroyEngine (MossEngine *const pEngine)
{
  if (pEngine->device != VK_NULL_HANDLE) { vkDeviceWaitIdle (pEngine->device); }

  moss__cleanupSwapchain (pEngine);

  moss__cleanupFences (pEngine, pEngine->inFlightFences);
  moss__cleanupSemaphores (pEngine, pEngine->renderFinishedSemaphores);
  moss__cleanupSemaphores (pEngine, pEngine->imageAvailableSemaphores);

  if (pEngine->device != VK_NULL_HANDLE)
  {
    if (pEngine->transferCommandPool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool (pEngine->device, pEngine->transferCommandPool, NULL);
    }

    if (pEngine->generalCommandPool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool (pEngine->device, pEngine->generalCommandPool, NULL);
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      if (pEngine->cameraUboMemories[ i ] != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, pEngine->cameraUboMemories[ i ], NULL);
      }
      if (pEngine->cameraUboBuffers[ i ] != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, pEngine->cameraUboBuffers[ i ], NULL);
      }
    }

    moss__cleanupDepthResources (pEngine);

    if (pEngine->sampler != VK_NULL_HANDLE)
    {
      vkDestroySampler (pEngine->device, pEngine->sampler, NULL);
    }

    if (pEngine->textureImageView != VK_NULL_HANDLE)
    {
      vkDestroyImageView (pEngine->device, pEngine->textureImageView, NULL);
    }

    if (pEngine->textureImage != VK_NULL_HANDLE)
    {
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
    }

    if (pEngine->textureImageMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
    }

    if (pEngine->graphicsPipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline (pEngine->device, pEngine->graphicsPipeline, NULL);
    }

    if (pEngine->pipelineLayout != VK_NULL_HANDLE)
    {
      vkDestroyPipelineLayout (pEngine->device, pEngine->pipelineLayout, NULL);
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      if (pEngine->descriptorSets[ i ] == VK_NULL_HANDLE) { continue; }

      vkFreeDescriptorSets (
        pEngine->device,
        pEngine->descriptorPool,
        1,
        &pEngine->descriptorSets[ i ]
      );
    }

    if (pEngine->descriptorPool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool (pEngine->device, pEngine->descriptorPool, NULL);
    }

    if (pEngine->descriptorSetLayout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout (pEngine->device, pEngine->descriptorSetLayout, NULL);
    }

    if (pEngine->renderPass != VK_NULL_HANDLE)
    {
      vkDestroyRenderPass (pEngine->device, pEngine->renderPass, NULL);
    }

    vkDestroyDevice (pEngine->device, NULL);
  }

  if (pEngine->surface != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR (pEngine->apiInstance, pEngine->surface, NULL);
  }

  if (pEngine->apiInstance != VK_NULL_HANDLE)
  {
    vkDestroyInstance (pEngine->apiInstance, NULL);
  }

  free (pEngine);
}

MossResult mossBeginFrame (MossEngine *const pEngine)
{
  const VkFence     inFlightFence = pEngine->inFlightFences[ pEngine->currentFrame ];
  const VkSemaphore imageAvailableSemaphore =
    pEngine->imageAvailableSemaphores[ pEngine->currentFrame ];
  const VkCommandBuffer commandBuffer =
    pEngine->generalCommandBuffers[ pEngine->currentFrame ];

  vkWaitForFences (pEngine->device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences (pEngine->device, 1, &inFlightFence);

  VkResult result = vkAcquireNextImageKHR (
    pEngine->device,
    pEngine->swapchain,
    UINT64_MAX,
    imageAvailableSemaphore,
    VK_NULL_HANDLE,
    &pEngine->currentImageIndex
  );

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    // Swap chain is out of date, need to recreate before we can acquire image
    // Get current framebuffer size from callback
    VkExtent2D extent;
    pEngine->getWindowFramebufferSize (&extent.width, &extent.height);
    return moss__recreateSwapchain (pEngine, extent);
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    moss__error ("Failed to acquire swap chain image.\n");
    return MOSS_RESULT_ERROR;
  }

  vkResetCommandBuffer (commandBuffer, 0);

  const VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };

  if (vkBeginCommandBuffer (commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    moss__error ("Failed to begin recording command buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  const VkClearValue clearValues[] = {
    { .color = { { 0.01F, 0.01F, 0.01F, 1.0F } } },
    { .depthStencil = { 1.0F, 0 } },
  };

  const VkRenderPassBeginInfo renderPassInfo = {
    .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass  = pEngine->renderPass,
    .framebuffer = pEngine->presentFramebuffers[ pEngine->currentImageIndex ],
    .renderArea  = {
      .offset = {0, 0},
      .extent = pEngine->swapchainExtent,
    },
    .clearValueCount = sizeof(clearValues) / sizeof(clearValues[0]),
    .pClearValues    = clearValues,
  };

  vkCmdBeginRenderPass (commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline (
    commandBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pEngine->graphicsPipeline
  );

  const VkViewport viewport = {
    .x        = 0.0F,
    .y        = 0.0F,
    .width    = (float)pEngine->swapchainExtent.width,
    .height   = (float)pEngine->swapchainExtent.height,
    .minDepth = 0.0F,
    .maxDepth = 1.0F,
  };

  const VkRect2D scissor = {
    .offset = { 0, 0 },
    .extent = pEngine->swapchainExtent,
  };

  vkCmdSetViewport (commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor (commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets (
    commandBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    pEngine->pipelineLayout,
    0,
    1,
    &pEngine->descriptorSets[ pEngine->currentFrame ],
    0,
    NULL
  );

  // Update camera UBO data before rendering
  moss_updateCameraUboData (pEngine);

  return MOSS_RESULT_SUCCESS;
}

MossResult mossEndFrame (MossEngine *const pEngine)
{
  const VkSemaphore pImageAvailableSemaphore =
    pEngine->imageAvailableSemaphores[ pEngine->currentFrame ];
  const VkSemaphore pRenderFinishedSemaphore =
    pEngine->renderFinishedSemaphores[ pEngine->currentFrame ];
  const VkCommandBuffer pCommandBuffer =
    pEngine->generalCommandBuffers[ pEngine->currentFrame ];
  const VkFence  pInFlightFence      = pEngine->inFlightFences[ pEngine->currentFrame ];
  const uint32_t current_image_index = pEngine->currentImageIndex;

  vkCmdEndRenderPass (pCommandBuffer);

  if (vkEndCommandBuffer (pCommandBuffer) != VK_SUCCESS)
  {
    moss__error ("Failed to end recording command buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  const VkSemaphore pWaitSemaphores[] = { pImageAvailableSemaphore };
  const size_t      wait_semaphore_count =
    sizeof (pWaitSemaphores) / sizeof (pWaitSemaphores[ 0 ]);

  const VkSemaphore pSignalSemaphores[] = { pRenderFinishedSemaphore };
  const size_t      signal_semaphore_count =
    sizeof (pSignalSemaphores) / sizeof (pSignalSemaphores[ 0 ]);

  const VkSubmitInfo submit_info = {
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = wait_semaphore_count,
    .pWaitSemaphores    = pWaitSemaphores,
    .pWaitDstStageMask =
      (const VkPipelineStageFlags[]) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    .commandBufferCount   = 1,
    .pCommandBuffers      = &pCommandBuffer,
    .signalSemaphoreCount = signal_semaphore_count,
    .pSignalSemaphores    = pSignalSemaphores,
  };

  if (vkQueueSubmit (pEngine->graphicsQueue, 1, &submit_info, pInFlightFence) !=
      VK_SUCCESS)
  {
    moss__error ("Failed to submit draw command buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  const VkPresentInfoKHR present_info = {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = signal_semaphore_count,
    .pWaitSemaphores    = pSignalSemaphores,
    .swapchainCount     = 1,
    .pSwapchains        = &pEngine->swapchain,
    .pImageIndices      = &current_image_index,
  };

  const VkResult result = vkQueuePresentKHR (pEngine->presentQueue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    // Swap chain is out of date or suboptimal, need to recreate
    // Get current framebuffer size from callback
    VkExtent2D extent;
    pEngine->getWindowFramebufferSize (&extent.width, &extent.height);
    if (moss__recreateSwapchain (pEngine, extent) != MOSS_RESULT_SUCCESS)
    {
      return MOSS_RESULT_ERROR;
    }
  }
  else if (result != VK_SUCCESS)
  {
    moss__error ("Failed to present swap chain image.\n");
    return MOSS_RESULT_ERROR;
  }

  pEngine->currentFrame = (pEngine->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  return MOSS_RESULT_SUCCESS;
}

MossResult mossSetRenderResolution (MossEngine *const pEngine, const vec2 newResolution)
{
  (void)(pEngine);
  (void)(newResolution);

  return MOSS_RESULT_SUCCESS;
}
