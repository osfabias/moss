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

#pragma once

#include <vulkan/vulkan.h>

#include "moss/app_info.h"
#include "moss/result.h"

#include "src/internal/config.h"
#include "src/internal/engine.h"

/*=============================================================================
    INTERNAL FUNCTION DECLARATIONS
  =============================================================================*/

MossResult moss__createApiInstance (MossEngine *pEngine, const MossAppInfo *pAppInfo);

MossResult moss__createSurface (MossEngine *pEngine);

MossResult moss__createLogicalDevice (MossEngine *pEngine);

void moss_initBufferSharingMode (MossEngine *pEngine);

MossResult moss__createSwapchain (MossEngine *pEngine, const VkExtent2D extent);

MossResult moss__createSwapchainImageViews (MossEngine *pEngine);

MossResult moss__createRenderPass (MossEngine *pEngine);

MossResult moss__createCameraUboBuffers (MossEngine *pEngine);

MossResult moss__createTextureImage (MossEngine *pEngine);

MossResult moss__createTextureImageView (MossEngine *pEngine);

MossResult moss__createTextureSampler (MossEngine *pEngine);

MossResult moss__createDepthResources (MossEngine *pEngine);

MossResult moss__createDescriptorPool (MossEngine *pEngine);

MossResult moss__createDescriptorSetLayout (MossEngine *pEngine);

MossResult moss_allocateDescriptorSets (MossEngine *pEngine);

void moss__configureDescriptorSets (MossEngine *pEngine);

void moss__createVkPipelineVertexInputStateInfo (
  VkPipelineVertexInputStateCreateInfo *pOutInfo
);

MossResult moss__createGraphicsPipeline (MossEngine *pEngine);

MossResult moss__createPresentFramebuffers (MossEngine *pEngine);

MossResult moss__createGeneralCommandBuffers (MossEngine *pEngine);

MossResult moss__createImageAvailableSemaphores (MossEngine *pEngine);

MossResult moss__createRenderFinishedSemaphores (MossEngine *pEngine);

MossResult moss__createInFlightFences (MossEngine *pEngine);

MossResult moss__createSynchronizationObjects (MossEngine *pEngine);

void moss__cleanupSemaphores (
  MossEngine *pEngine,
  VkSemaphore pSemaphores[ MAX_FRAMES_IN_FLIGHT ]
);

void moss__cleanupFences (MossEngine *pEngine, VkFence pFences[ MAX_FRAMES_IN_FLIGHT ]);

void moss__cleanupSwapchainFramebuffers (MossEngine *pEngine);

void moss__cleanupSwapchainImageViews (MossEngine *pEngine);

void moss__cleanupSwapchainHandle (MossEngine *pEngine);

void moss__cleanupDepthResources (MossEngine *pEngine);

void moss__cleanupSwapchain (MossEngine *pEngine);

MossResult moss__recreateSwapchain (MossEngine *pEngine, VkExtent2D extent);

void moss_updateCameraUboData (MossEngine *pEngine);
