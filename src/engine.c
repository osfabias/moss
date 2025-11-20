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
#include <time.h>

#include <vulkan/vulkan.h>
#ifdef __APPLE__
#  include <vulkan/vulkan_metal.h>
#endif

#include <cglm/cglm.h>

#include <stb/stb_image.h>

#include "moss/app_info.h"
#include "moss/engine.h"
#include "moss/result.h"

#include "src/internal/app_info.h"
#include "src/internal/config.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/shaders.h"
#include "src/internal/vertex.h"
#include "src/internal/vulkan/utils/buffer.h"
#include "src/internal/vulkan/utils/command_pool.h"
#include "src/internal/vulkan/utils/image.h"
#include "src/internal/vulkan/utils/image_view.h"
#include "src/internal/vulkan/utils/instance.h"
#include "src/internal/vulkan/utils/physical_device.h"
#include "src/internal/vulkan/utils/shader.h"
#include "src/internal/vulkan/utils/swapchain.h"
#include "src/internal/vulkan/utils/validation_layers.h"

/*=============================================================================
    INTERNAL FUNCTION DECLARATIONS
  =============================================================================*/

inline static MossResult
moss__createApiInstance (MossEngine *pEngine, const MossAppInfo *pAppInfo);

inline static MossResult moss__createSurface (MossEngine *pEngine);

inline static MossResult moss__createLogicalDevice (MossEngine *pEngine);

inline static void mossInitBufferSharingMode (MossEngine *pEngine);

inline static MossResult
moss__createSwapchain (MossEngine *pEngine, const VkExtent2D extent);

inline static MossResult moss__createSwapchainImageViews (MossEngine *pEngine);

inline static MossResult moss__createRenderPass (MossEngine *pEngine);

inline static MossResult moss__createCameraUboBuffers (MossEngine *pEngine);

inline static MossResult moss__createTextureImage (MossEngine *pEngine);

inline static MossResult moss__createTextureImageView (MossEngine *pEngine);

inline static MossResult moss__createTextureSampler (MossEngine *pEngine);

inline static MossResult moss__createDepthResources (MossEngine *pEngine);

inline static MossResult moss__createDescriptorPool (MossEngine *pEngine);

inline static MossResult moss__createDescriptorSetLayout (MossEngine *pEngine);

inline static MossResult moss_allocateDescriptorSets (MossEngine *pEngine);

inline static void moss__configureDescriptorSets (MossEngine *pEngine);

inline static void moss__createVkPipelineVertexInputStateInfo (
  VkPipelineVertexInputStateCreateInfo *pOutInfo
);

inline static MossResult moss__createGraphicsPipeline (MossEngine *pEngine);

inline static MossResult moss__createPresentFramebuffers (MossEngine *pEngine);

inline static MossResult moss__createGeneralCommandBuffers (MossEngine *pEngine);

inline static MossResult moss__createImageAvailableSemaphores (MossEngine *pEngine);

inline static MossResult moss__createRenderFinishedSemaphores (MossEngine *pEngine);

inline static MossResult moss__createInFlightFences (MossEngine *pEngine);

inline static MossResult moss__createSynchronizationObjects (MossEngine *pEngine);

inline static void moss__cleanupSemaphores (
  MossEngine *pEngine,
  VkSemaphore pSemaphores[ MAX_FRAMES_IN_FLIGHT ]
);

inline static void
moss__cleanupFences (MossEngine *pEngine, VkFence pFences[ MAX_FRAMES_IN_FLIGHT ]);

inline static void moss__cleanupSwapchainFramebuffers (MossEngine *pEngine);

inline static void moss__cleanupSwapchainImageViews (MossEngine *pEngine);

inline static void moss__cleanupSwapchainHandle (MossEngine *pEngine);

inline static void moss__cleanupDepthResources (MossEngine *pEngine);

inline static void moss__cleanupSwapchain (MossEngine *pEngine);

inline static MossResult moss__recreateSwapchain (MossEngine *pEngine, VkExtent2D extent);

inline static void mossUpdateCameraUboData (MossEngine *pEngine);

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

  mossInitBufferSharingMode (pEngine);

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
  mossUpdateCameraUboData (pEngine);

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

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

inline static MossResult
moss__createApiInstance (MossEngine *const pEngine, const MossAppInfo *const pAppInfo)
{
  // Set up validation layers
#ifdef NDEBUG
  const bool enable_validation_layers = false;
#else
  const bool enable_validation_layers = true;
#endif

  uint32_t           validation_layer_count = 0;
  const char *const *pValidationLayerNames  = NULL;

  if (enable_validation_layers)
  {
    if (mossVk__checkValidationLayerSupport ( ))
    {
      const MossVk__ValidationLayers validationLayers = mossVk__getValidationLayers ( );
      validation_layer_count                          = validationLayers.count;
      pValidationLayerNames                           = validationLayers.names;
    }
    else {
      moss__warning (
        "Validation layers are enabled but not supported. Disabling validation layers."
      );
    }
  }

  // Set up other required info
  VkApplicationInfo vkAppInfo;
  moss__createVkAppInfo (pAppInfo, &vkAppInfo);
  const MossVk__InstanceExtensions extensions = mossVk__getRequiredInstanceExtensions ( );

  // Make instance create info
  const VkInstanceCreateInfo instanceCreateInfo = {
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo        = &vkAppInfo,
    .ppEnabledExtensionNames = extensions.names,
    .enabledExtensionCount   = extensions.count,
    .enabledLayerCount       = validation_layer_count,
    .ppEnabledLayerNames     = pValidationLayerNames,
    .flags                   = mossVk__getRequiredInstanceFlags ( ),
  };

  const VkResult result =
    vkCreateInstance (&instanceCreateInfo, NULL, &pEngine->apiInstance);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create Vulkan instance. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createSurface (MossEngine *const pEngine)
{
#ifdef __APPLE__
  const VkMetalSurfaceCreateInfoEXT surface_create_info = {
    .sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
    .pNext  = NULL,
    .flags  = 0,
    .pLayer = pEngine->metalLayer,
  };

  const VkResult result = vkCreateMetalSurfaceEXT (
    pEngine->apiInstance,
    &surface_create_info,
    NULL,
    &pEngine->surface
  );

  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create window surface. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
#else
  moss__error ("Metal layer is only supported on macOS.\n");
  return MOSS_RESULT_ERROR;
#endif
}

inline static MossResult moss__createLogicalDevice (MossEngine *const pEngine)
{
  const MossVk__PhysicalDeviceExtensions extensions =
    mossVk__getRequiredDeviceExtensions ( );

  uint32_t                queue_create_info_count = 0;
  VkDeviceQueueCreateInfo queue_create_infos[ 3 ];
  const float             queue_priority = 1.0F;

  // Add graphics queue create info
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = pEngine->queueFamilyIndices.graphicsFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  // Add present queue create info
  if (pEngine->queueFamilyIndices.graphicsFamily !=
      pEngine->queueFamilyIndices.presentFamily)
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = pEngine->queueFamilyIndices.presentFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  // Add transfer queue create info
  if (pEngine->queueFamilyIndices.graphicsFamily !=
      pEngine->queueFamilyIndices.transferFamily)
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = pEngine->queueFamilyIndices.transferFamily,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  VkPhysicalDeviceFeatures device_features = { 0 };

  const VkDeviceCreateInfo create_info = {
    .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount    = queue_create_info_count,
    .pQueueCreateInfos       = queue_create_infos,
    .enabledExtensionCount   = extensions.count,
    .ppEnabledExtensionNames = extensions.names,
    .pEnabledFeatures        = &device_features,
  };

  const VkResult result =
    vkCreateDevice (pEngine->physicalDevice, &create_info, NULL, &pEngine->device);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create logical device. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void mossInitBufferSharingMode (MossEngine *const pEngine)
{
  if (pEngine->queueFamilyIndices.graphicsFamily ==
      pEngine->queueFamilyIndices.transferFamily)
  {
    pEngine->bufferSharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    pEngine->sharedQueueFamilyIndexCount = 0;
  }
  else {
    pEngine->bufferSharingMode             = VK_SHARING_MODE_CONCURRENT;
    pEngine->sharedQueueFamilyIndices[ 0 ] = pEngine->queueFamilyIndices.graphicsFamily;
    pEngine->sharedQueueFamilyIndices[ 1 ] = pEngine->queueFamilyIndices.transferFamily;
    pEngine->sharedQueueFamilyIndexCount   = 2;
  }
}

inline static MossResult
moss__createSwapchain (MossEngine *const pEngine, const VkExtent2D extent)
{
  const MossVk__QuerySwapchainSupportInfo queryInfo = {
    .device  = pEngine->physicalDevice,
    .surface = pEngine->surface,
  };
  const MossVk__SwapChainSupportDetails swapchainSupport =
    mossVk__querySwapchainSupport (&queryInfo);

  const VkSurfaceFormatKHR surfaceFormat = mossVk__chooseSwapSurfaceFormat (
    swapchainSupport.formats,
    swapchainSupport.formatCount
  );
  const VkPresentModeKHR presentMode = mossVk__chooseSwapPresentMode (
    swapchainSupport.presentModes,
    swapchainSupport.presentModeCount
  );
  const VkExtent2D chosenExtent = mossVk__chooseSwapExtent (
    &swapchainSupport.capabilities,
    extent.width,
    extent.height
  );

  VkSwapchainCreateInfoKHR createInfo = {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = pEngine->surface,
    .minImageCount    = swapchainSupport.capabilities.minImageCount,
    .imageFormat      = surfaceFormat.format,
    .imageColorSpace  = surfaceFormat.colorSpace,
    .imageExtent      = chosenExtent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform     = swapchainSupport.capabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = presentMode,
    .clipped          = VK_TRUE,
    .oldSwapchain     = VK_NULL_HANDLE,
  };

  uint32_t queueFamilyIndices[] = {
    pEngine->queueFamilyIndices.graphicsFamily,
    pEngine->queueFamilyIndices.presentFamily,
  };

  if (pEngine->queueFamilyIndices.graphicsFamily !=
      pEngine->queueFamilyIndices.presentFamily)
  {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  }
  else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = NULL;
  }

  const VkResult result =
    vkCreateSwapchainKHR (pEngine->device, &createInfo, NULL, &pEngine->swapchain);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create swap chain. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  vkGetSwapchainImagesKHR (
    pEngine->device,
    pEngine->swapchain,
    &pEngine->swapchainImageCount,
    NULL
  );

  if (pEngine->swapchainImageCount > MAX_SWAPCHAIN_IMAGE_COUNT)
  {
    moss__error (
      "Real swapchain image count is bigger than expected. (%d > %zu)",
      pEngine->swapchainImageCount,
      MAX_SWAPCHAIN_IMAGE_COUNT
    );
    return MOSS_RESULT_ERROR;
  }

  vkGetSwapchainImagesKHR (
    pEngine->device,
    pEngine->swapchain,
    &pEngine->swapchainImageCount,
    pEngine->swapchainImages
  );

  pEngine->swapchainImageFormat = surfaceFormat.format;
  pEngine->swapchainExtent      = extent;

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createSwapchainImageViews (MossEngine *const pEngine)
{
  MossVk__ImageViewCreateInfo info = {
    .device = pEngine->device,
    .image  = VK_NULL_HANDLE,
    .format = pEngine->swapchainImageFormat,
    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
  };

  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    info.image = pEngine->swapchainImages[ i ];
    const MossResult result =
      mossVk__createImageView (&info, &pEngine->presentFramebufferImageViews[ i ]);
    if (result != MOSS_RESULT_SUCCESS) { return MOSS_RESULT_ERROR; }
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createRenderPass (MossEngine *const pEngine)
{
  const VkAttachmentDescription color_attachment = {
    .format         = pEngine->swapchainImageFormat,
    .flags          = 0,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  const VkAttachmentReference color_attachment_ref = {
    .attachment = 0,
    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  const VkAttachmentDescription depth_attachment = {
    .format         = VK_FORMAT_D32_SFLOAT,
    .flags          = 0,
    .samples        = VK_SAMPLE_COUNT_1_BIT,
    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  const VkAttachmentReference depth_attachment_ref = {
    .attachment = 1,
    .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  const VkSubpassDescription subpass = {
    .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount    = 1,
    .pColorAttachments       = &color_attachment_ref,
    .pDepthStencilAttachment = &depth_attachment_ref,
  };

  const VkSubpassDependency subpass_dependency = {
    .srcSubpass   = VK_SUBPASS_EXTERNAL,
    .dstSubpass   = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };

  const VkAttachmentDescription attachments[] = {
    color_attachment,
    depth_attachment,
  };

  const VkRenderPassCreateInfo render_pass_info = {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = sizeof (attachments) / sizeof (attachments[ 0 ]),
    .pAttachments    = attachments,
    .subpassCount    = 1,
    .pSubpasses      = &subpass,
    .dependencyCount = 1,
    .pDependencies   = &subpass_dependency,
  };

  const VkResult result =
    vkCreateRenderPass (pEngine->device, &render_pass_info, NULL, &pEngine->renderPass);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create render pass. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createCameraUboBuffers (MossEngine *const pEngine)
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

inline static MossResult moss__createTextureImage (MossEngine *const pEngine)
{
  // Load texture
  int textureWidth, textureHeight, texture_channels;  // NOLINT

  const stbi_uc *const pPixels = stbi_load (
    "textures/atlas.png",
    &textureWidth,
    &textureHeight,
    &texture_channels,
    STBI_rgb_alpha
  );
  if (pPixels == NULL)
  {
    moss__error ("Failed to load texture.");
    return MOSS_RESULT_ERROR;
  }

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  {  // Create staging buffer
    {
      const MossVk__CreateBufferInfo createInfo = {
        .device                      = pEngine->device,
        .size                        = (VkDeviceSize)(textureWidth * textureHeight * 4),
        .usage                       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode                 = pEngine->bufferSharingMode,
        .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
        .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
      };
      const MossResult result = mossVk__createBuffer (&createInfo, &stagingBuffer);
      if (result != MOSS_RESULT_SUCCESS)
      {
        moss__error ("Failed to create staging buffer.");
        return MOSS_RESULT_ERROR;
      }
    }

    {
      const MossVk__AllocateBufferMemoryInfo allocInfo = {
        .physicalDevice = pEngine->physicalDevice,
        .device         = pEngine->device,
        .buffer         = stagingBuffer,
        .memoryProperties =
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      };
      const MossResult result =
        mossVk__allocateBufferMemory (&allocInfo, &stagingBufferMemory);
      if (result != MOSS_RESULT_SUCCESS)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
        moss__error ("Failed to allocate staging buffer memory.");
        return MOSS_RESULT_ERROR;
      }
    }
  }

  // Copy pixels into the buffer
  void *pData;
  vkMapMemory (
    pEngine->device,
    stagingBufferMemory,
    0,
    (VkDeviceSize)(textureWidth * textureHeight * 4),
    0,
    &pData
  );
  memcpy (pData, pPixels, (size_t)(textureWidth * textureHeight * 4));
  vkUnmapMemory (pEngine->device, stagingBufferMemory);

  // Free pixels
  stbi_image_free ((void *)pPixels);

  // Create texture image
  const MossVk__CreateImageInfo createInfo = {
    .device      = pEngine->device,
    .format      = VK_FORMAT_R8G8B8A8_SRGB,
    .imageWidth  = textureWidth,
    .imageHeight = textureHeight,
    .usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = pEngine->bufferSharingMode,
    .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
    .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
  };

  {
    const MossResult result = mossVk__createImage (&createInfo, &pEngine->textureImage);
    if (result != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      moss__error ("Failed to create texture image.\n");
      return MOSS_RESULT_ERROR;
    }
  }


  // Allocate memory for texture image
  const MossVk__AllocateImageMemoryInfo allocInfo = {
    .physicalDevice = pEngine->physicalDevice,
    .device         = pEngine->device,
    .image          = pEngine->textureImage,
  };

  {
    const MossResult result =
      mossVk__allocateImageMemory (&allocInfo, &pEngine->textureImageMemory);
    if (result != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      moss__error ("Failed to allocate memory for the texture image.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__TransitionImageLayoutInfo transitionInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .image         = pEngine->textureImage,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    };
    if (mossVk__transitionImageLayout (&transitionInfo) != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__CopyBufferToImageInfo copyInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .buffer        = stagingBuffer,
      .image         = pEngine->textureImage,
      .width         = textureWidth,
      .height        = textureHeight,
    };
    if (mossVk__copyBufferToImage (&copyInfo) != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__TransitionImageLayoutInfo transitionInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .image         = pEngine->textureImage,
      .oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    if (mossVk__transitionImageLayout (&transitionInfo) != MOSS_RESULT_SUCCESS)
    {
      if (stagingBufferMemory != VK_NULL_HANDLE)
      {
        vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
      }
      if (stagingBuffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
      }
      vkFreeMemory (pEngine->device, pEngine->textureImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->textureImage, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    if (stagingBufferMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory (pEngine->device, stagingBufferMemory, NULL);
    }
    if (stagingBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer (pEngine->device, stagingBuffer, NULL);
    }
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createTextureImageView (MossEngine *const pEngine)
{
  const MossVk__ImageViewCreateInfo info = {
    .device = pEngine->device,
    .image  = pEngine->textureImage,
    .format = VK_FORMAT_R8G8B8A8_SRGB,
    .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
  };
  const MossResult result = mossVk__createImageView (&info, &pEngine->textureImageView);
  if (result != MOSS_RESULT_SUCCESS) { return MOSS_RESULT_ERROR; }
  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createTextureSampler (MossEngine *const pEngine)
{
  const VkSamplerCreateInfo create_info = {
    .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter               = VK_FILTER_NEAREST,
    .minFilter               = VK_FILTER_NEAREST,
    .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable        = VK_FALSE,
    .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE,
    .compareEnable           = VK_FALSE,
    .compareOp               = VK_COMPARE_OP_ALWAYS,
    .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .mipLodBias              = 0.0F,
    .minLod                  = 0.0F,
    .maxLod                  = 0.0F,
  };

  const VkResult result =
    vkCreateSampler (pEngine->device, &create_info, NULL, &pEngine->sampler);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create sampler: %d.", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createDepthResources (MossEngine *const pEngine)
{
  // TODO: add moss_vk__select_format function that will select format from
  //       the list of desired formats ordered by priority.
  const VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;


  {  // Create depth image
    const MossVk__CreateImageInfo info = {
      .device                      = pEngine->device,
      .format                      = depthImageFormat,
      .imageWidth                  = pEngine->swapchainExtent.width,
      .imageHeight                 = pEngine->swapchainExtent.height,
      .usage                       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .sharingMode                 = pEngine->bufferSharingMode,
      .sharedQueueFamilyIndices    = pEngine->sharedQueueFamilyIndices,
      .sharedQueueFamilyIndexCount = pEngine->sharedQueueFamilyIndexCount,
    };

    {
      const MossResult result = mossVk__createImage (&info, &pEngine->depthImage);
      if (result != MOSS_RESULT_SUCCESS)
      {
        moss__error ("Failed to create depth image.\n");
        return MOSS_RESULT_ERROR;
      }
    }
  }


  {  // Allocate memory for texture image
    const MossVk__AllocateImageMemoryInfo info = {
      .physicalDevice = pEngine->physicalDevice,
      .device         = pEngine->device,
      .image          = pEngine->depthImage,
    };

    {
      const MossResult result =
        mossVk__allocateImageMemory (&info, &pEngine->depthImageMemory);
      if (result != MOSS_RESULT_SUCCESS)
      {
        vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);

        moss__error ("Failed to allocate memory for the depth image.\n");
        return MOSS_RESULT_ERROR;
      }
    }
  }

  {  // Create depth image view
    const MossVk__ImageViewCreateInfo info = {
      .device = pEngine->device,
      .image  = pEngine->depthImage,
      .format = depthImageFormat,
      .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
    };

    const MossResult result = mossVk__createImageView (&info, &pEngine->depthImageView);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkFreeMemory (pEngine->device, pEngine->depthImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);

      moss__error ("Failed to create depth image view.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Transition depth image layout
    const MossVk__TransitionImageLayoutInfo info = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .transferQueue = pEngine->transferQueue,
      .image         = pEngine->depthImage,
      .oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const MossResult result = mossVk__transitionImageLayout (&info);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyImageView (pEngine->device, pEngine->depthImageView, NULL);
      vkFreeMemory (pEngine->device, pEngine->depthImageMemory, NULL);
      vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);

      moss__error ("Failed to transition depth image layout.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createDescriptorPool (MossEngine *const pEngine)
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

inline static MossResult moss__createDescriptorSetLayout (MossEngine *const pEngine)
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

inline static MossResult moss_allocateDescriptorSets (MossEngine *const pEngine)
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

inline static void moss__configureDescriptorSets (MossEngine *const pEngine)
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

inline static void moss__createVkPipelineVertexInputStateInfo (
  VkPipelineVertexInputStateCreateInfo *const pOutInfo
)
{
  const Moss__VkVertexInputBindingDescriptionPack bindingDescriptionsPack =
    moss__getVkVertexInputBindingDescription ( );

  const Moss__VkVertexInputAttributeDescriptionPack attributeDescriptionsPack =
    moss__getVkVertexInputAttributeDescription ( );

  *pOutInfo = (VkPipelineVertexInputStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = bindingDescriptionsPack.count,
    .pVertexBindingDescriptions      = bindingDescriptionsPack.descriptions,
    .vertexAttributeDescriptionCount = attributeDescriptionsPack.count,
    .pVertexAttributeDescriptions    = attributeDescriptionsPack.descriptions,
  };
}

inline static MossResult moss__createGraphicsPipeline (MossEngine *const pEngine)
{
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;

  {
    const MossVk__CreateShaderModuleFromFileInfo createInfo = {
      .device          = pEngine->device,
      .filePath        = MOSS__VERT_SHADER_PATH,
      .outShaderModule = &vertShaderModule,
    };
    const MossResult result = mossVk__createShaderModuleFromFile (&createInfo);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create fragment shader module.");
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__CreateShaderModuleFromFileInfo createInfo = {
      .device          = pEngine->device,
      .filePath        = MOSS__FRAG_SHADER_PATH,
      .outShaderModule = &fragShaderModule,
    };
    const MossResult result = mossVk__createShaderModuleFromFile (&createInfo);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyShaderModule (pEngine->device, vertShaderModule, NULL);

      moss__error ("Failed to create fragment shader module.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  const VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertShaderModule,
    .pName  = "main",
  };

  const VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragShaderModule,
    .pName  = "main",
  };

  const VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info,
                                                            frag_shader_stage_info };

  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  moss__createVkPipelineVertexInputStateInfo (&vertex_input_info);

  const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  const VkPipelineViewportStateCreateInfo viewport_state = {
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports    = NULL,  // Dynamic viewport
    .scissorCount  = 1,
    .pScissors     = NULL,  // Dynamic scissor
  };

  const VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable        = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode             = VK_POLYGON_MODE_FILL,
    .lineWidth               = 1.0F,
    .cullMode                = VK_CULL_MODE_NONE,
    .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable         = VK_FALSE,
  };

  const VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable  = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  const VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable         = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp        = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp        = VK_BLEND_OP_ADD,
  };

  const VkPipelineColorBlendStateCreateInfo color_blending = {
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable   = VK_FALSE,
    .attachmentCount = 1,
    .pAttachments    = &color_blend_attachment,
  };

  const VkDescriptorSetLayout set_layouts[] = { pEngine->descriptorSetLayout };

  const VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext                  = NULL,
    .setLayoutCount         = sizeof (set_layouts) / sizeof (set_layouts[ 0 ]),
    .pSetLayouts            = set_layouts,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = NULL,
  };

  if (vkCreatePipelineLayout (
        pEngine->device,
        &pipeline_layout_info,
        NULL,
        &pEngine->pipelineLayout
      ) != VK_SUCCESS)
  {
    moss__error ("Failed to create pipeline layout.\n");
    vkDestroyShaderModule (pEngine->device, fragShaderModule, NULL);
    vkDestroyShaderModule (pEngine->device, vertShaderModule, NULL);
    return MOSS_RESULT_ERROR;
  }

  const VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  const VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = sizeof (dynamic_states) / sizeof (dynamic_states[ 0 ]),
    .pDynamicStates    = dynamic_states,
  };

  const VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext                 = VK_FALSE,
    .depthTestEnable       = VK_TRUE,
    .depthWriteEnable      = VK_TRUE,
    .depthCompareOp        = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds        = 0.0F,
    .maxDepthBounds        = 0.0F,
    .stencilTestEnable     = VK_FALSE,
    .front                 = { 0 },
    .back                  = { 0 },
  };

  const VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext               = NULL,
    .stageCount          = 2,
    .pStages             = shader_stages,
    .pVertexInputState   = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState      = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState   = &multisampling,
    .pColorBlendState    = &color_blending,
    .pDynamicState       = &dynamic_state,
    .pDepthStencilState  = &depth_stencil_state,
    .layout              = pEngine->pipelineLayout,
    .renderPass          = pEngine->renderPass,
    .subpass             = 0,
    .basePipelineHandle  = VK_NULL_HANDLE,
    .basePipelineIndex   = -1,
  };

  const VkResult result = vkCreateGraphicsPipelines (
    pEngine->device,
    VK_NULL_HANDLE,
    1,
    &pipeline_info,
    NULL,
    &pEngine->graphicsPipeline
  );

  vkDestroyShaderModule (pEngine->device, fragShaderModule, NULL);
  vkDestroyShaderModule (pEngine->device, vertShaderModule, NULL);

  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create graphics pipeline. Error code: %d.\n", result);
    vkDestroyPipelineLayout (pEngine->device, pEngine->pipelineLayout, NULL);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createPresentFramebuffers (MossEngine *const pEngine)
{
  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    const VkImageView attachments[] = {
      pEngine->presentFramebufferImageViews[ i ],
      pEngine->depthImageView,
    };

    const VkFramebufferCreateInfo framebuffer_info = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = pEngine->renderPass,
      .attachmentCount = sizeof (attachments) / sizeof (attachments[ 0 ]),
      .pAttachments    = attachments,
      .width           = pEngine->swapchainExtent.width,
      .height          = pEngine->swapchainExtent.height,
      .layers          = 1,
    };

    if (vkCreateFramebuffer (
          pEngine->device,
          &framebuffer_info,
          NULL,
          &pEngine->presentFramebuffers[ i ]
        ) != VK_SUCCESS)
    {
      moss__error ("Failed to create framebuffer %u.\n", i);
      for (uint32_t j = 0; j < i; ++j)
      {
        vkDestroyFramebuffer (pEngine->device, pEngine->presentFramebuffers[ j ], NULL);
      }
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__createGeneralCommandBuffers (MossEngine *const pEngine)
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

inline static MossResult moss__createImageAvailableSemaphores (MossEngine *const pEngine)
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

inline static MossResult moss__createRenderFinishedSemaphores (MossEngine *const pEngine)
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

inline static MossResult moss__createInFlightFences (MossEngine *const pEngine)
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

inline static MossResult moss__createSynchronizationObjects (MossEngine *const pEngine)
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

inline static void mossUpdateCameraUboData (MossEngine *const pEngine)
{
  memcpy (
    pEngine->cameraUboBufferMappedMemoryBlocks[ pEngine->currentFrame ],
    &pEngine->camera,
    sizeof (pEngine->camera)
  );
}

inline static void moss__cleanupSwapchainFramebuffers (MossEngine *const pEngine)
{
  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    if (pEngine->presentFramebuffers[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyFramebuffer (pEngine->device, pEngine->presentFramebuffers[ i ], NULL);
    pEngine->presentFramebuffers[ i ] = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanupSwapchainImageViews (MossEngine *const pEngine)
{
  for (uint32_t i = 0; i < pEngine->swapchainImageCount; ++i)
  {
    if (pEngine->presentFramebufferImageViews[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyImageView (
      pEngine->device,
      pEngine->presentFramebufferImageViews[ i ],
      NULL
    );
    pEngine->presentFramebufferImageViews[ i ] = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanupSwapchainHandle (MossEngine *const pEngine)
{
  if (pEngine->swapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR (pEngine->device, pEngine->swapchain, NULL);
    pEngine->swapchain = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanupDepthResources (MossEngine *const pEngine)
{
  if (pEngine->depthImageView != VK_NULL_HANDLE)
  {
    vkDestroyImageView (pEngine->device, pEngine->depthImageView, NULL);
    pEngine->depthImageView = VK_NULL_HANDLE;
  }

  if (pEngine->depthImageMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory (pEngine->device, pEngine->depthImageMemory, NULL);
    pEngine->depthImageMemory = VK_NULL_HANDLE;
  }

  if (pEngine->depthImage != VK_NULL_HANDLE)
  {
    vkDestroyImage (pEngine->device, pEngine->depthImage, NULL);
    pEngine->depthImage = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanupSwapchain (MossEngine *const pEngine)
{
  moss__cleanupSwapchainFramebuffers (pEngine);
  moss__cleanupDepthResources (pEngine);
  moss__cleanupSwapchainImageViews (pEngine);
  moss__cleanupSwapchainHandle (pEngine);

  pEngine->swapchainImageCount  = 0;
  pEngine->swapchainImageFormat = 0;
  pEngine->swapchainExtent      = (VkExtent2D) { .width = 0, .height = 0 };
}

inline static MossResult
moss__recreateSwapchain (MossEngine *const pEngine, const VkExtent2D extent)
{
  vkDeviceWaitIdle (pEngine->device);

  moss__cleanupSwapchain (pEngine);

  if (moss__createSwapchain (pEngine, extent) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__createSwapchainImageViews (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__createDepthResources (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__createPresentFramebuffers (pEngine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}


inline static void moss__cleanupSemaphores (
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

inline static void
moss__cleanupFences (MossEngine *const pEngine, VkFence pFences[ MAX_FRAMES_IN_FLIGHT ])
{
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (pFences[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyFence (pEngine->device, pFences[ i ], NULL);
    pFences[ i ] = VK_NULL_HANDLE;
  }
}
