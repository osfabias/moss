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
  void *metal_layer;
  /* Callback to get window framebuffer size. */
  MossGetWindowFramebufferSizeCallback get_window_framebuffer_size;

  /* === Vulkan instance and surface === */
  /* Vulkan instance. */
  VkInstance api_instance;
  /* Window surface. */
  VkSurfaceKHR surface;

  /* === Physical and logical device === */
  /* Physical device. */
  VkPhysicalDevice physical_device;
  /* Logical device. */
  VkDevice device;
  /* Queue family indices. */
  MossVk__QueueFamilyIndices queue_family_indices;
  /* Graphics queue. */
  VkQueue graphics_queue;
  /* Present queue. */
  VkQueue present_queue;
  /* Transfer queue. */
  VkQueue transfer_queue;

  /* === Buffer sharing mode and queue family indices === */
  /* Buffer sharing mode for buffers shared between graphics and transfer queues. */
  VkSharingMode buffer_sharing_mode;
  /* Number of queue family indices that share buffers. */
  uint32_t shared_queue_family_index_count;
  /* Queue family indices that share buffers. */
  uint32_t shared_queue_family_indices[ 2 ];

  /* === Swap chain === */
  /* Swap chain. */
  VkSwapchainKHR swapchain;
  /* Swap chain images. */
  VkImage swapchain_images[ MAX_SWAPCHAIN_IMAGE_COUNT ];
  /* Number of swap chain images. */
  uint32_t swapchain_image_count;
  /* Swap chain image format. */
  VkFormat swapchain_image_format;
  /* Swap chain extent. */
  VkExtent2D swapchain_extent;
  /* Swap chain image views. */
  VkImageView swapchain_image_views[ MAX_SWAPCHAIN_IMAGE_COUNT ];
  /* Swap chain framebuffers. */
  VkFramebuffer swapchain_framebuffers[ MAX_SWAPCHAIN_IMAGE_COUNT ];

  /* === Render pipeline === */
  /* Render pass. */
  VkRenderPass render_pass;
  /* Descriptor pool. */
  VkDescriptorPool descriptor_pool;
  /* Descriptor set. */
  VkDescriptorSet descriptor_sets[ MAX_FRAMES_IN_FLIGHT ];
  /* Descriptor set layout. */
  VkDescriptorSetLayout descriptor_set_layout;
  /* Pipeline layout. */
  VkPipelineLayout pipeline_layout;
  /* Graphics pipeline. */
  VkPipeline graphics_pipeline;

  /* === Depth buffering === */
  /* Depth image. */
  VkImage depth_image;
  /* Depth image view. */
  VkImageView depth_image_view;
  /* Depth image. */
  VkDeviceMemory depth_image_memory;

  /* === Texture image :3 === */
  /* Texture image. */
  VkImage texture_image;
  /* Texture image view. */
  VkImageView texture_image_view;
  /* Texture image memory. */
  VkDeviceMemory texture_image_memory;
  /* Sampler. */
  VkSampler sampler;
  /* Uniform buffers. */
  VkBuffer       camera_ubo_buffers[ MAX_FRAMES_IN_FLIGHT ];
  VkDeviceMemory camera_ubo_memories[ MAX_FRAMES_IN_FLIGHT ];
  /* Uniform buffer mapped memory blocks. */
  void *camera_ubo_buffer_mapped_memory_blocks[ MAX_FRAMES_IN_FLIGHT ];

  /* === Command buffers === */
  /* General command pool. */
  VkCommandPool general_command_pool;
  /* Command buffers. */
  VkCommandBuffer general_command_buffers[ MAX_FRAMES_IN_FLIGHT ];
  /* Transfer command pool. */
  VkCommandPool transfer_command_pool;

  /* === Synchronization objects === */
  /* Image available semaphores. */
  VkSemaphore image_available_semaphores[ MAX_FRAMES_IN_FLIGHT ];
  /* Render finished semaphores. */
  VkSemaphore render_finished_semaphores[ MAX_FRAMES_IN_FLIGHT ];
  /* In-flight fences. */
  VkFence in_flight_fences[ MAX_FRAMES_IN_FLIGHT ];

  /* === Frame state === */
  /* Current frame index. */
  uint32_t current_frame;
  /* Current swap chain image index (set by moss_begin_frame). */
  uint32_t current_image_index;
};

/*
  @brief Initialize engine state to default values.
*/
inline static void moss__init_engine_state (MossEngine *engine)
{
  *engine = (MossEngine){
    .camera = {
      .scale  = {1, 1},
      .offset = {0, 0},
    },

    /* Metal layer. */
    .metal_layer = NULL,
    /* Framebuffer size callback. */
    .get_window_framebuffer_size = NULL,

    /* Vulkan instance and surface. */
    .api_instance = VK_NULL_HANDLE,
    .surface      = VK_NULL_HANDLE,

    /* Physical and logical device. */
    .physical_device                            = VK_NULL_HANDLE,
    .device                                     = VK_NULL_HANDLE,
    .graphics_queue                             = VK_NULL_HANDLE,
    .present_queue                              = VK_NULL_HANDLE,
    .transfer_queue = VK_NULL_HANDLE,
    .queue_family_indices = {
      .graphics_family       = 0,
      .present_family        = 0,
      .transfer_family       = 0,
      .graphics_family_found = false,
      .present_family_found  = false,
      .transfer_family_found = false,
    },
    .buffer_sharing_mode            = VK_SHARING_MODE_EXCLUSIVE,
    .shared_queue_family_index_count = 0,
    .shared_queue_family_indices     = {0, 0},

    /* Swap chain. */
    .swapchain                   = VK_NULL_HANDLE,
    .swapchain_images            = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE },
    .swapchain_image_count       = 0,
    .swapchain_image_format      = 0,
    .swapchain_extent            = (VkExtent2D) { .width = 0, .height = 0 },
    .swapchain_image_views       = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE },
    .swapchain_framebuffers      = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE },

    /* Render pipeline. */
    .render_pass           = VK_NULL_HANDLE,
    .descriptor_pool       = VK_NULL_HANDLE,
    .descriptor_sets       = {VK_NULL_HANDLE, VK_NULL_HANDLE},
    .descriptor_set_layout = VK_NULL_HANDLE,
    .pipeline_layout       = VK_NULL_HANDLE,
    .graphics_pipeline     = VK_NULL_HANDLE,

    /* Depth resources */
    .depth_image        = VK_NULL_HANDLE,
    .depth_image_view   = VK_NULL_HANDLE,
    .depth_image_memory = VK_NULL_HANDLE,

    /* Uniform buffers. */
    .camera_ubo_buffers   = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .camera_ubo_memories  = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .camera_ubo_buffer_mapped_memory_blocks = { NULL, NULL },

    /* Command buffers. */
    .general_command_pool    = VK_NULL_HANDLE,
    .general_command_buffers = { VK_NULL_HANDLE, VK_NULL_HANDLE },

    /* Synchronization objects. */
    .image_available_semaphores = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .render_finished_semaphores = { VK_NULL_HANDLE, VK_NULL_HANDLE },
    .in_flight_fences           = { VK_NULL_HANDLE, VK_NULL_HANDLE },

    /* Frame state. */
    .current_frame      = 0,
    .current_image_index = 0,
  };
}
