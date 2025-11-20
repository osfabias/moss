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

  @file src/internal/engine.h
  @brief Internal engine struct body declaration.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <vulkan/vulkan.h>

#include "moss/camera.h"
#include "moss/engine.h"

#include "src/internal/camera.h"
#include "src/internal/config.h"
#include "src/internal/vulkan/utils/physical_device.h"

/*
  @brief Engine state.
*/
struct MossEngine
{
  /* === Frame parameters. === */
  /* Camera. */
  MossCamera camera;

  /* === Metal layer === */
  /* Metal layer for surface creation on macOS. */
  void *metalLayer;
  /* Callback to get window framebuffer size. */
  MossGetWindowFramebufferSizeCallback getWindowFramebufferSize;

  /* === Vulkan instance and surface === */
  /* Vulkan instance. */
  VkInstance apiInstance;
  /* Window surface. */
  VkSurfaceKHR surface;

  /* === Physical and logical device === */
  /* Physical device. */
  VkPhysicalDevice physicalDevice;
  /* Logical device. */
  VkDevice device;
  /* Queue family indices. */
  MossVk__QueueFamilyIndices queueFamilyIndices;
  /* Graphics queue. */
  VkQueue graphicsQueue;
  /* Present queue. */
  VkQueue presentQueue;
  /* Transfer queue. */
  VkQueue transferQueue;

  /* === Buffer sharing mode and queue family indices === */
  /* Buffer sharing mode for buffers shared between graphics and transfer queues. */
  VkSharingMode bufferSharingMode;
  /* Number of queue family indices that share buffers. */
  uint32_t sharedQueueFamilyIndexCount;
  /* Queue family indices that share buffers. */
  uint32_t sharedQueueFamilyIndices[ 2 ];

  /* === Swap chain === */
  /* Swap chain. */
  VkSwapchainKHR swapchain;
  /* Swap chain images. */
  VkImage swapchainImages[ MAX_SWAPCHAIN_IMAGE_COUNT ];
  /* Number of swap chain images. */
  uint32_t swapchainImageCount;
  /* Swap chain image format. */
  VkFormat swapchainImageFormat;
  /* Swap chain extent. */
  VkExtent2D swapchainExtent;

  /* === Present framebuffers === */
  /* Framebuffers used for final frame presentation. */
  VkFramebuffer presentFramebuffers[ MAX_SWAPCHAIN_IMAGE_COUNT ];
  /* Present framebuffer image views. */
  VkImageView presentFramebufferImageViews[ MAX_SWAPCHAIN_IMAGE_COUNT ];

  /* === Render pipeline === */
  /* Render pass. */
  VkRenderPass renderPass;
  /* Descriptor pool. */
  VkDescriptorPool descriptorPool;
  /* Descriptor set. */
  VkDescriptorSet descriptorSets[ MAX_FRAMES_IN_FLIGHT ];
  /* Descriptor set layout. */
  VkDescriptorSetLayout descriptorSetLayout;
  /* Pipeline layout. */
  VkPipelineLayout pipelineLayout;
  /* Graphics pipeline. */
  VkPipeline graphicsPipeline;

  /* === Depth buffering === */
  /* Depth image. */
  VkImage depthImage;
  /* Depth image view. */
  VkImageView depthImageView;
  /* Depth image. */
  VkDeviceMemory depthImageMemory;

  /* === Texture image :3 === */
  /* Texture image. */
  VkImage textureImage;
  /* Texture image view. */
  VkImageView textureImageView;
  /* Texture image memory. */
  VkDeviceMemory textureImageMemory;
  /* Sampler. */
  VkSampler sampler;
  /* Uniform buffers. */
  VkBuffer       cameraUboBuffers[ MAX_FRAMES_IN_FLIGHT ];
  VkDeviceMemory cameraUboMemories[ MAX_FRAMES_IN_FLIGHT ];
  /* Uniform buffer mapped memory blocks. */
  void *cameraUboBufferMappedMemoryBlocks[ MAX_FRAMES_IN_FLIGHT ];

  /* === Command buffers === */
  /* General command pool. */
  VkCommandPool generalCommandPool;
  /* Command buffers. */
  VkCommandBuffer generalCommandBuffers[ MAX_FRAMES_IN_FLIGHT ];
  /* Transfer command pool. */
  VkCommandPool transferCommandPool;

  /* === Synchronization objects === */
  /* Image available semaphores. */
  VkSemaphore imageAvailableSemaphores[ MAX_FRAMES_IN_FLIGHT ];
  /* Render finished semaphores. */
  VkSemaphore renderFinishedSemaphores[ MAX_FRAMES_IN_FLIGHT ];
  /* In-flight fences. */
  VkFence inFlightFences[ MAX_FRAMES_IN_FLIGHT ];

  /* === Frame state === */
  /* Current frame index. */
  uint32_t currentFrame;
  /* Current swap chain image index (set by moss_begin_frame). */
  uint32_t currentImageIndex;
};

/*
  @brief Initialize engine state to default values.
*/
inline static void moss__initEngineState (MossEngine *engine)
{
  *engine = (MossEngine){
    .camera = {
      .scale  = {1, 1},
      .offset = {0, 0},
    },

    /* Metal layer. */
    .metalLayer = NULL,
    /* Framebuffer size callback. */
    .getWindowFramebufferSize = NULL,

    /* Vulkan instance and surface. */
    .apiInstance = VK_NULL_HANDLE,
    .surface      = VK_NULL_HANDLE,

    /* Physical and logical device. */
    .physicalDevice                            = VK_NULL_HANDLE,
    .device                                     = VK_NULL_HANDLE,
    .graphicsQueue                             = VK_NULL_HANDLE,
    .presentQueue                              = VK_NULL_HANDLE,
    .transferQueue = VK_NULL_HANDLE,
    .queueFamilyIndices = {
      .graphicsFamily       = 0,
      .presentFamily        = 0,
      .transferFamily       = 0,
      .graphicsFamilyFound = false,
      .presentFamilyFound  = false,
      .transferFamilyFound = false,
    },
    .bufferSharingMode            = VK_SHARING_MODE_EXCLUSIVE,
    .sharedQueueFamilyIndexCount = 0,
    .sharedQueueFamilyIndices     = {0, 0},

    /* Swap chain. */
    .swapchain                   = VK_NULL_HANDLE,
    .swapchainImages            = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE },
    .swapchainImageCount       = 0,
    .swapchainImageFormat      = 0,
    .swapchainExtent            = (VkExtent2D) { .width = 0, .height = 0 },
    .presentFramebufferImageViews       = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE },
    .presentFramebuffers      = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE },

    /* Render pipeline. */
    .renderPass           = VK_NULL_HANDLE,
    .descriptorPool       = VK_NULL_HANDLE,
    .descriptorSets       = {VK_NULL_HANDLE, VK_NULL_HANDLE},
    .descriptorSetLayout = VK_NULL_HANDLE,
    .pipelineLayout       = VK_NULL_HANDLE,
    .graphicsPipeline     = VK_NULL_HANDLE,

    /* Depth resources */
    .depthImage        = VK_NULL_HANDLE,
    .depthImageView   = VK_NULL_HANDLE,
    .depthImageMemory = VK_NULL_HANDLE,

    /* Uniform buffers. */
    .cameraUboBuffers   = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .cameraUboMemories  = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .cameraUboBufferMappedMemoryBlocks = { NULL, NULL },

    /* Command buffers. */
    .generalCommandPool    = VK_NULL_HANDLE,
    .generalCommandBuffers = { VK_NULL_HANDLE, VK_NULL_HANDLE },

    /* Synchronization objects. */
    .imageAvailableSemaphores = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .renderFinishedSemaphores = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .inFlightFences           = { VK_NULL_HANDLE, VK_NULL_HANDLE },

    /* Frame state. */
    .currentFrame      = 0,
    .currentImageIndex = 0,
  };
}
