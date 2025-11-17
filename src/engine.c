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

  @file src/engine.c
  @brief Graphics engine functions implementation.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#include "vulkan/vulkan_core.h"
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

#include <src/internal/stb_image.h>

#include "moss/app_info.h"
#include "moss/engine.h"
#include "moss/result.h"
#include "moss/vertex.h"

#include "src/internal/app_info.h"
#include "src/internal/config.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/memory_utils.h"
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
    TEMPO
  =============================================================================*/

/* Vertex array just for implementing vertex buffers. */
static const MossVertex g_verticies[ 4 ] = {
  {   { 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F }, { 0.0F, 1.0F } },
  {  { 32.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 1.0F, 1.0F } },
  { { 32.0F, 32.0F }, { 0.0F, 0.0F, 1.0F }, { 1.0F, 0.0F } },
  {  { 0.0F, 32.0F }, { 1.0F, 1.0F, 1.0F }, { 0.0F, 0.0F } }
};

/* Index array just for implementing index buffers. */
static const uint16_t g_indices[ 6 ] = { 0, 1, 2, 2, 3, 0 };

/*=============================================================================
    INTERNAL FUNCTION DECLARATIONS
  =============================================================================*/

/*
  @brief Creates Vulkan API instance.
  @param app_info A pointer to a native moss app info struct.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult
moss__create_api_instance (MossEngine *engine, const MossAppInfo *app_info);

/*
  @brief Creates window surface.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_surface (MossEngine *engine);

/*
  @brief Creates logical device and queues.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_logical_device (MossEngine *engine);

/*
  @brief Initializes buffer sharing mode and queue family indices.
  @details Determines whether graphics and transfer queue families are the same,
           and sets up the appropriate sharing mode and queue family indices
           for buffer creation. This should be called after logical device creation.
*/
inline static void moss__init_buffer_sharing_mode (MossEngine *engine);

/*
  @brief Creates swap chain.
  @param width Window width.
  @param height Window height.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult
moss__create_swapchain (MossEngine *engine, uint32_t width, uint32_t height);

/*
  @brief Creates image views for swap chain images.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_swapchain_image_views (MossEngine *engine);

/*
  @brief Creates render pass.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_render_pass (MossEngine *engine);

/*
  @brief Returns Vulkan pipeline vertex input state info.
  @return Vulkan pipeline vertex input state info.
*/
inline static VkPipelineVertexInputStateCreateInfo
moss__create_vk_pipeline_vertex_input_state_info (void);

/*
  @brief Creates descriptor pool.
  @return Returns MOSS_RESULT_SUCCESS on successs, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_descriptor_pool (MossEngine *engine);

/*
  @brief Creates descriptor set layout.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_descriptor_set_layout (MossEngine *engine);

/*
  @brief Allocates descriptor set.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__allocate_descriptor_sets (MossEngine *engine);

/*
  @brief Sets up descriptor sets.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static void moss__configure_descriptor_sets (MossEngine *engine);

/*
  @brief Creates graphics pipeline.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_graphics_pipeline (MossEngine *engine);

/*
  @brief Creates framebuffers.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_framebuffers (MossEngine *engine);

/*
  @brief Creates texture image.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_texture_image (MossEngine *engine);

/*
  @brief Creates texture image view.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_texture_image_view (MossEngine *engine);

/*
  @brief Creates texture sampler.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_texture_sampler (MossEngine *engine);

/*
  @brief Creates vertex buffer.
  @return Returns MOSS_RESULT_SUCCESS on successs, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_vertex_buffer (MossEngine *engine);

/*
  @brief Fills vertex buffer with vertex data.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__fill_vertex_buffer (MossEngine *engine);

/*
  @brief Creates index buffer.
  @return Returns MOSS_RESULT_SUCCESS on successs, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_index_buffer (MossEngine *engine);

/*
  @brief Fills index buffer with index data.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__fill_index_buffer (MossEngine *engine);

/*
  @brief Creates camera UBO buffers.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_camera_ubo_buffers (MossEngine *engine);

/*
  @brief Creates command buffers.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_general_command_buffers (MossEngine *engine);

/*
  @brief Creates image available semaphores.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_image_available_semaphores (MossEngine *engine);

/*
  @brief Creates render finished semaphores.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_render_finished_semaphores (MossEngine *engine);

/*
  @brief Creates in-flight fences.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_in_flight_fences (MossEngine *engine);

/*
  @brief Creates synchronization objects (semaphores and fences).
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_synchronization_objects (MossEngine *engine);

/*
  @brief Cleans up semaphores array.
  @param semaphores Array of semaphores to clean up.
*/
inline static void moss__cleanup_semaphores (MossEngine *engine, VkSemaphore *semaphores);

/*
  @brief Cleans up fences array.
  @param fences Array of fences to clean up.
*/
inline static void moss__cleanup_fences (MossEngine *engine, VkFence *fences);

/*
  @brief Cleans up image available semaphores.
*/
inline static void moss__cleanup_image_available_semaphores (MossEngine *engine);

/*
  @brief Cleans up render finished semaphores.
*/
inline static void moss__cleanup_render_finished_semaphores (MossEngine *engine);

/*
  @brief Cleans up in-flight fences.
*/
inline static void moss__cleanup_in_flight_fences (MossEngine *engine);

/*
  @brief Cleans up synchronization objects.
*/
inline static void moss__cleanup_synchronization_objects (MossEngine *engine);

/*
  @brief Cleans up swapchain framebuffers.
*/
inline static void moss__cleanup_swapchain_framebuffers (MossEngine *engine);

/*
  @brief Cleans up swapchain image views.
*/
inline static void moss__cleanup_swapchain_image_views (MossEngine *engine);

/*
  @brief Cleans up swapchain handle.
*/
inline static void moss__cleanup_swapchain_handle (MossEngine *engine);

/*
  @brief Cleans up swap chain resources.
*/
inline static void moss__cleanup_swapchain (MossEngine *engine);

/*
  @brief Recreates swap chain.
  @param width Window width.
  @param height Window height.
  @return Returns MOSS_RESULT_SUCCESS on success, error code otherwise.
*/
inline static MossResult
moss__recreate_swapchain (MossEngine *engine, uint32_t width, uint32_t height);

/*
  @brief Updates camera ubo data.
*/
inline static void moss__update_camera_ubo_data (MossEngine *engine);

/*=============================================================================
    PUBLIC API FUNCTIONS IMPLEMENTATION
  =============================================================================*/

/*
  @brief Creates engine instance.
  @param config Engine configuration.
  @return On success returns valid pointer to engine state, otherwise returns NULL.
*/
MossEngine *moss_create_engine (const MossEngineConfig *const config)
{
  MossEngine *const engine = malloc (sizeof (MossEngine));
  if (engine == NULL) { return NULL; }

  moss__init_engine_state (engine);

#ifdef __APPLE__
  // Store metal_layer from config
  engine->metal_layer = config->metal_layer;
  if (engine->metal_layer == NULL)
  {
    moss__error ("metal_layer must be provided in config.\n");
    free (engine);
    return NULL;
  }
#else
  moss__error ("Metal layer is only supported on macOS.\n");
  free (engine);
  return NULL;
#endif

  // Store framebuffer size callback
  engine->get_window_framebuffer_size = config->get_window_framebuffer_size;
  if (engine->get_window_framebuffer_size == NULL)
  {
    moss__error ("get_window_framebuffer_size callback must be provided in config.\n");
    free (engine);
    return NULL;
  }

  if (moss__create_api_instance (engine, config->app_info) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_surface (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  {
    const Moss__SelectPhysicalDeviceInfo select_info = {
      .instance   = engine->api_instance,
      .surface    = engine->surface,
      .out_device = &engine->physical_device,
    };
    if (moss_vk__select_physical_device (&select_info) != VK_SUCCESS)
    {
      moss_destroy_engine ((MossEngine *)engine);
      return NULL;
    }
  }

  {
    const Moss__FindQueueFamiliesInfo find_info = {
      .device  = engine->physical_device,
      .surface = engine->surface,
    };
    engine->queue_family_indices = moss_vk__find_queue_families (&find_info);
  }

  if (moss__create_logical_device (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  vkGetDeviceQueue (
    engine->device,
    engine->queue_family_indices.graphics_family,
    0,
    &engine->graphics_queue
  );

  vkGetDeviceQueue (
    engine->device,
    engine->queue_family_indices.present_family,
    0,
    &engine->present_queue
  );

  vkGetDeviceQueue (
    engine->device,
    engine->queue_family_indices.transfer_family,
    0,
    &engine->transfer_queue
  );

  moss__init_buffer_sharing_mode (engine);

  // Get framebuffer size from callback
  uint32_t width, height;
  engine->get_window_framebuffer_size (&width, &height);

  if (moss__create_swapchain (engine, width, height) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_swapchain_image_views (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_render_pass (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_camera_ubo_buffers (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  // Create general command pool
  {
    const Moss__CreateVkCommandPoolInfo create_info = {
      .device             = engine->device,
      .queue_family_index = engine->queue_family_indices.graphics_family,
      .out_command_pool   = &engine->general_command_pool,
    };
    if (moss_vk__create_command_pool (&create_info) != MOSS_RESULT_SUCCESS)
    {
      moss_destroy_engine ((MossEngine *)engine);
      return NULL;
    }
  }

  // Create transfer command pool
  {
    const Moss__CreateVkCommandPoolInfo create_info = {
      .device             = engine->device,
      .queue_family_index = engine->queue_family_indices.transfer_family,
      .out_command_pool   = &engine->transfer_command_pool,
    };
    if (moss_vk__create_command_pool (&create_info) != MOSS_RESULT_SUCCESS)
    {
      moss_destroy_engine ((MossEngine *)engine);
      return NULL;
    }
  }

  if (moss__create_texture_image (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_texture_image_view (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_texture_sampler (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }


  if (moss__create_descriptor_pool (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_descriptor_set_layout (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__allocate_descriptor_sets (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  moss__configure_descriptor_sets (engine);

  if (moss__create_graphics_pipeline (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_framebuffers (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_vertex_buffer (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__fill_vertex_buffer (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_index_buffer (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__fill_index_buffer (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_general_command_buffers (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  if (moss__create_synchronization_objects (engine) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine ((MossEngine *)engine);
    return NULL;
  }

  engine->current_frame = 0;

  // Initialize camera UBO data for all frames
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    memcpy (
      engine->camera_ubo_buffer_mapped_memory_blocks[ i ],
      &engine->camera,
      sizeof (engine->camera)
    );
  }

  return (MossEngine *)engine;
}

/*
  @brief Destroys engine instance.
  @details Cleans up all reserved memory and destroys all GraphicsAPI objects.
  @param engine Engine handler.
*/
void moss_destroy_engine (MossEngine *const engine)
{
  if (engine->device != VK_NULL_HANDLE) { vkDeviceWaitIdle (engine->device); }

  moss__cleanup_swapchain (engine);
  moss__cleanup_synchronization_objects (engine);

  if (engine->device != VK_NULL_HANDLE)
  {
    if (engine->transfer_command_pool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool (engine->device, engine->transfer_command_pool, NULL);
      engine->transfer_command_pool = VK_NULL_HANDLE;
    }

    if (engine->general_command_pool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool (engine->device, engine->general_command_pool, NULL);
      engine->general_command_pool = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
      moss_vk__destroy_buffer (
        engine->device,
        engine->camera_ubo_buffers[ i ],
        engine->camera_ubo_memories[ i ]
      );
    }

    moss_vk__destroy_buffer (
      engine->device,
      engine->index_buffer,
      engine->index_buffer_memory
    );

    moss_vk__destroy_buffer (
      engine->device,
      engine->vertex_buffer,
      engine->vertex_buffer_memory
    );

    if (engine->sampler != VK_NULL_HANDLE)
    {
      vkDestroySampler (engine->device, engine->sampler, NULL);
      engine->sampler = VK_NULL_HANDLE;
    }

    if (engine->texture_image_view != VK_NULL_HANDLE)
    {
      vkDestroyImageView (engine->device, engine->texture_image_view, NULL);
      engine->texture_image_view = VK_NULL_HANDLE;
    }

    if (engine->texture_image != VK_NULL_HANDLE)
    {
      vkDestroyImage (engine->device, engine->texture_image, NULL);
      engine->texture_image = VK_NULL_HANDLE;
    }

    if (engine->texture_image_memory != VK_NULL_HANDLE)
    {
      vkFreeMemory (engine->device, engine->texture_image_memory, NULL);
      engine->texture_image_memory = VK_NULL_HANDLE;
    }

    if (engine->graphics_pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline (engine->device, engine->graphics_pipeline, NULL);
      engine->graphics_pipeline = VK_NULL_HANDLE;
    }

    if (engine->pipeline_layout != VK_NULL_HANDLE)
    {
      vkDestroyPipelineLayout (engine->device, engine->pipeline_layout, NULL);
      engine->pipeline_layout = VK_NULL_HANDLE;
    }

    vkFreeDescriptorSets (
      engine->device,
      engine->descriptor_pool,
      MAX_FRAMES_IN_FLIGHT,
      engine->descriptor_sets
    );

    if (engine->descriptor_pool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool (engine->device, engine->descriptor_pool, NULL);
      engine->descriptor_pool = VK_NULL_HANDLE;
    }

    if (engine->descriptor_set_layout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout (engine->device, engine->descriptor_set_layout, NULL);
      engine->descriptor_set_layout = VK_NULL_HANDLE;
    }

    if (engine->render_pass != VK_NULL_HANDLE)
    {
      vkDestroyRenderPass (engine->device, engine->render_pass, NULL);
      engine->render_pass = VK_NULL_HANDLE;
    }

    vkDestroyDevice (engine->device, NULL);
    engine->device                                     = VK_NULL_HANDLE;
    engine->physical_device                            = VK_NULL_HANDLE;
    engine->graphics_queue                             = VK_NULL_HANDLE;
    engine->transfer_queue                             = VK_NULL_HANDLE;
    engine->present_queue                              = VK_NULL_HANDLE;
    engine->queue_family_indices.graphics_family       = 0;
    engine->queue_family_indices.present_family        = 0;
    engine->queue_family_indices.transfer_family       = 0;
    engine->queue_family_indices.graphics_family_found = false;
    engine->queue_family_indices.present_family_found  = false;
    engine->queue_family_indices.transfer_family_found = false;
  }

  if (engine->surface != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR (engine->api_instance, engine->surface, NULL);
    engine->surface = VK_NULL_HANDLE;
  }

  if (engine->api_instance != VK_NULL_HANDLE)
  {
    vkDestroyInstance (engine->api_instance, NULL);
    engine->api_instance = VK_NULL_HANDLE;
  }


  engine->current_frame = 0;

  free (engine);
}

/*
  @brief Begins a new frame.
  @param engine Engine handler.
  @return On success return MOSS_RESULT_SUCCESS, otherwise returns MOSS_RESULT_ERROR.
*/
MossResult moss_begin_frame (MossEngine *const engine)
{
  const VkFence     in_flight_fence = engine->in_flight_fences[ engine->current_frame ];
  const VkSemaphore image_available_semaphore =
    engine->image_available_semaphores[ engine->current_frame ];
  const VkCommandBuffer command_buffer =
    engine->general_command_buffers[ engine->current_frame ];

  vkWaitForFences (engine->device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
  vkResetFences (engine->device, 1, &in_flight_fence);

  uint32_t current_image_index;
  VkResult result = vkAcquireNextImageKHR (
    engine->device,
    engine->swapchain,
    UINT64_MAX,
    image_available_semaphore,
    VK_NULL_HANDLE,
    &current_image_index
  );

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    // Swap chain is out of date, need to recreate before we can acquire image
    // Get current framebuffer size from callback
    uint32_t width, height;
    engine->get_window_framebuffer_size (&width, &height);
    return moss__recreate_swapchain (engine, width, height);
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    moss__error ("Failed to acquire swap chain image.\n");
    return MOSS_RESULT_ERROR;
  }

  engine->current_image_index = current_image_index;

  vkResetCommandBuffer (command_buffer, 0);

  const VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };

  if (vkBeginCommandBuffer (command_buffer, &begin_info) != VK_SUCCESS)
  {
    moss__error ("Failed to begin recording command buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  const VkRenderPassBeginInfo render_pass_info = {
    .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass  = engine->render_pass,
    .framebuffer = engine->swapchain_framebuffers[ current_image_index ],
    .renderArea  = {
      .offset = {0, 0},
      .extent = engine->swapchain_extent,
    },
    .clearValueCount = 1,
    .pClearValues    = (const VkClearValue[]) {
      {.color = {{0.01F, 0.01F, 0.01F, 1.0F}}},
    },
  };

  vkCmdBeginRenderPass (command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline (
    command_buffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    engine->graphics_pipeline
  );

  const VkViewport viewport = {
    .x        = 0.0F,
    .y        = 0.0F,
    .width    = (float)engine->swapchain_extent.width,
    .height   = (float)engine->swapchain_extent.height,
    .minDepth = 0.0F,
    .maxDepth = 1.0F,
  };

  const VkRect2D scissor = {
    .offset = { 0, 0 },
    .extent = engine->swapchain_extent,
  };

  vkCmdSetViewport (command_buffer, 0, 1, &viewport);
  vkCmdSetScissor (command_buffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets (
    command_buffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    engine->pipeline_layout,
    0,
    1,
    &engine->descriptor_sets[ engine->current_frame ],
    0,
    NULL
  );

  // Update camera UBO data before rendering
  moss__update_camera_ubo_data (engine);

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Ends the current frame.
  @param engine Engine handler.
  @return On success return MOSS_RESULT_SUCCESS, otherwise returns MOSS_RESULT_ERROR.
*/
MossResult moss_end_frame (MossEngine *const engine)
{
  const VkSemaphore image_available_semaphore =
    engine->image_available_semaphores[ engine->current_frame ];
  const VkSemaphore render_finished_semaphore =
    engine->render_finished_semaphores[ engine->current_frame ];
  const VkCommandBuffer command_buffer =
    engine->general_command_buffers[ engine->current_frame ];
  const VkFence  in_flight_fence     = engine->in_flight_fences[ engine->current_frame ];
  const uint32_t current_image_index = engine->current_image_index;

  vkCmdEndRenderPass (command_buffer);

  if (vkEndCommandBuffer (command_buffer) != VK_SUCCESS)
  {
    moss__error ("Failed to end recording command buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  const VkSemaphore wait_semaphores[] = { image_available_semaphore };
  const size_t      wait_semaphore_count =
    sizeof (wait_semaphores) / sizeof (wait_semaphores[ 0 ]);

  const VkSemaphore signal_semaphores[] = { render_finished_semaphore };
  const size_t      signal_semaphore_count =
    sizeof (signal_semaphores) / sizeof (signal_semaphores[ 0 ]);

  const VkSubmitInfo submit_info = {
    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = wait_semaphore_count,
    .pWaitSemaphores    = wait_semaphores,
    .pWaitDstStageMask =
      (const VkPipelineStageFlags[]) { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
    .commandBufferCount   = 1,
    .pCommandBuffers      = &command_buffer,
    .signalSemaphoreCount = signal_semaphore_count,
    .pSignalSemaphores    = signal_semaphores,
  };

  if (vkQueueSubmit (engine->graphics_queue, 1, &submit_info, in_flight_fence) !=
      VK_SUCCESS)
  {
    moss__error ("Failed to submit draw command buffer.\n");
    return MOSS_RESULT_ERROR;
  }

  const VkPresentInfoKHR present_info = {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = signal_semaphore_count,
    .pWaitSemaphores    = signal_semaphores,
    .swapchainCount     = 1,
    .pSwapchains        = &engine->swapchain,
    .pImageIndices      = &current_image_index,
  };

  VkResult result = vkQueuePresentKHR (engine->present_queue, &present_info);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    // Swap chain is out of date or suboptimal, need to recreate
    // Get current framebuffer size from callback
    uint32_t width, height;
    engine->get_window_framebuffer_size (&width, &height);
    if (moss__recreate_swapchain (engine, width, height) != MOSS_RESULT_SUCCESS)
    {
      return MOSS_RESULT_ERROR;
    }
  }
  else if (result != VK_SUCCESS)
  {
    moss__error ("Failed to present swap chain image.\n");
    return MOSS_RESULT_ERROR;
  }

  engine->current_frame = (engine->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

  return MOSS_RESULT_SUCCESS;
}

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

inline static MossResult
moss__create_api_instance (MossEngine *const engine, const MossAppInfo *const app_info)
{
  // Set up validation layers
#ifdef NDEBUG
  const bool enable_validation_layers = false;
#else
  const bool enable_validation_layers = true;
#endif

  uint32_t           validation_layer_count = 0;
  const char *const *validation_layer_names = NULL;

  if (enable_validation_layers)
  {
    if (moss_vk__check_validation_layer_support ( ))
    {
      const Moss__VkValidationLayers validation_layers =
        moss_vk__get_validation_layers ( );
      validation_layer_count = validation_layers.count;
      validation_layer_names = validation_layers.names;
    }
    else {
      moss__warning (
        "Validation layers are enabled but not supported. Disabling validation layers."
      );
    }
  }

  // Set up other required info
  const VkApplicationInfo          vk_app_info = moss__create_vk_app_info (app_info);
  const Moss__VkInstanceExtensions extensions =
    moss_vk__get_required_instance_extensions ( );

  // Make instance create info
  const VkInstanceCreateInfo instance_create_info = {
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo        = &vk_app_info,
    .ppEnabledExtensionNames = extensions.names,
    .enabledExtensionCount   = extensions.count,
    .enabledLayerCount       = validation_layer_count,
    .ppEnabledLayerNames     = validation_layer_names,
    .flags                   = moss_vk__get_required_instance_flags ( ),
  };

  const VkResult result =
    vkCreateInstance (&instance_create_info, NULL, &engine->api_instance);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create Vulkan instance. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_surface (MossEngine *const engine)
{
#ifdef __APPLE__
  const VkMetalSurfaceCreateInfoEXT surface_create_info = {
    .sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
    .pNext  = NULL,
    .flags  = 0,
    .pLayer = engine->metal_layer,
  };

  const VkResult result = vkCreateMetalSurfaceEXT (
    engine->api_instance,
    &surface_create_info,
    NULL,
    &engine->surface
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

inline static MossResult moss__create_logical_device (MossEngine *const engine)
{
  const Moss__VkPhysicalDeviceExtensions extensions =
    moss_vk__get_required_device_extensions ( );

  uint32_t                queue_create_info_count = 0;
  VkDeviceQueueCreateInfo queue_create_infos[ 3 ];
  const float             queue_priority = 1.0F;

  // Add graphics queue create info
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = engine->queue_family_indices.graphics_family,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  // Add present queue create info
  if (engine->queue_family_indices.graphics_family !=
      engine->queue_family_indices.present_family)
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = engine->queue_family_indices.present_family,
      .queueCount       = 1,
      .pQueuePriorities = &queue_priority,
    };
    queue_create_infos[ queue_create_info_count++ ] = create_info;
  }

  // Add transfer queue create info
  if (engine->queue_family_indices.graphics_family !=
      engine->queue_family_indices.transfer_family)
  {
    const VkDeviceQueueCreateInfo create_info = {
      .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = engine->queue_family_indices.transfer_family,
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
    vkCreateDevice (engine->physical_device, &create_info, NULL, &engine->device);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create logical device. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void moss__init_buffer_sharing_mode (MossEngine *const engine)
{
  if (engine->queue_family_indices.graphics_family ==
      engine->queue_family_indices.transfer_family)
  {
    engine->buffer_sharing_mode             = VK_SHARING_MODE_EXCLUSIVE;
    engine->shared_queue_family_index_count = 0;
  }
  else {
    engine->buffer_sharing_mode = VK_SHARING_MODE_CONCURRENT;
    engine->shared_queue_family_indices[ 0 ] =
      engine->queue_family_indices.graphics_family;
    engine->shared_queue_family_indices[ 1 ] =
      engine->queue_family_indices.transfer_family;
    engine->shared_queue_family_index_count = 2;
  }
}

inline static MossResult moss__create_swapchain (
  MossEngine *const engine,
  const uint32_t    width,
  const uint32_t    height
)
{
  const Moss__QuerySwapchainSupportInfo query_info = {
    .device  = engine->physical_device,
    .surface = engine->surface,
  };
  const Moss__SwapChainSupportDetails swapchain_support =
    moss_vk__query_swapchain_support (&query_info);

  const VkSurfaceFormatKHR surface_format = moss_vk__choose_swap_surface_format (
    swapchain_support.formats,
    swapchain_support.format_count
  );
  const VkPresentModeKHR present_mode = moss_vk__choose_swap_present_mode (
    swapchain_support.present_modes,
    swapchain_support.present_mode_count
  );
  const VkExtent2D extent =
    moss_vk__choose_swap_extent (&swapchain_support.capabilities, width, height);

  VkSwapchainCreateInfoKHR create_info = {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = engine->surface,
    .minImageCount    = swapchain_support.capabilities.minImageCount,
    .imageFormat      = surface_format.format,
    .imageColorSpace  = surface_format.colorSpace,
    .imageExtent      = extent,
    .imageArrayLayers = 1,
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform     = swapchain_support.capabilities.currentTransform,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode      = present_mode,
    .clipped          = VK_TRUE,
    .oldSwapchain     = VK_NULL_HANDLE,
  };

  uint32_t queue_family_indices[] = {
    engine->queue_family_indices.graphics_family,
    engine->queue_family_indices.present_family,
  };

  if (engine->queue_family_indices.graphics_family !=
      engine->queue_family_indices.present_family)
  {
    create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices   = queue_family_indices;
  }
  else {
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = NULL;
  }

  const VkResult result =
    vkCreateSwapchainKHR (engine->device, &create_info, NULL, &engine->swapchain);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create swap chain. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  vkGetSwapchainImagesKHR (
    engine->device,
    engine->swapchain,
    &engine->swapchain_image_count,
    NULL
  );

  if (engine->swapchain_image_count > MAX_SWAPCHAIN_IMAGE_COUNT)
  {
    moss__error (
      "Real swapchain image count is bigger than expected. (%d > %zu)",
      engine->swapchain_image_count,
      MAX_SWAPCHAIN_IMAGE_COUNT
    );
    return MOSS_RESULT_ERROR;
  }

  vkGetSwapchainImagesKHR (
    engine->device,
    engine->swapchain,
    &engine->swapchain_image_count,
    engine->swapchain_images
  );

  engine->swapchain_image_format = surface_format.format;
  engine->swapchain_extent       = extent;

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_swapchain_image_views (MossEngine *const engine)
{
  Moss__VkImageViewCreateInfo info = {
    .device = engine->device,
    .image  = VK_NULL_HANDLE,
    .format = engine->swapchain_image_format,
  };

  for (uint32_t i = 0; i < engine->swapchain_image_count; ++i)
  {
    info.image                   = engine->swapchain_images[ i ];
    const VkImageView image_view = moss_vk__create_image_view (&info);

    if (image_view == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

    engine->swapchain_image_views[ i ] = image_view;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_render_pass (MossEngine *const engine)
{
  const VkAttachmentDescription color_attachment = {
    .format         = engine->swapchain_image_format,
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

  const VkSubpassDescription subpass = {
    .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments    = &color_attachment_ref,
  };

  const VkSubpassDependency dependency = {
    .srcSubpass    = VK_SUBPASS_EXTERNAL,
    .dstSubpass    = 0,
    .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  const VkRenderPassCreateInfo render_pass_info = {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &color_attachment,
    .subpassCount    = 1,
    .pSubpasses      = &subpass,
    .dependencyCount = 1,
    .pDependencies   = &dependency,
  };

  const VkResult result =
    vkCreateRenderPass (engine->device, &render_pass_info, NULL, &engine->render_pass);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create render pass. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static VkPipelineVertexInputStateCreateInfo
moss__create_vk_pipeline_vertex_input_state_info (void)
{
  const Moss__VkVertexInputBindingDescriptionPack binding_descriptions_pack =
    moss__get_vk_vertex_input_binding_description ( );

  const Moss__VkVertexInputAttributeDescriptionPack attribute_descriptions_pack =
    moss__get_vk_vertex_input_attribute_description ( );

  const VkPipelineVertexInputStateCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = binding_descriptions_pack.count,
    .pVertexBindingDescriptions      = binding_descriptions_pack.descriptions,
    .vertexAttributeDescriptionCount = attribute_descriptions_pack.count,
    .pVertexAttributeDescriptions    = attribute_descriptions_pack.descriptions,
  };

  return info;
}

inline static MossResult moss__create_descriptor_pool (MossEngine *const engine)
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

  const VkResult result =
    vkCreateDescriptorPool (engine->device, &create_info, NULL, &engine->descriptor_pool);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create descriptor pool: %d.", result);
    return MOSS_RESULT_ERROR;
  }
  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_descriptor_set_layout (MossEngine *const engine)
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
    engine->device,
    &create_info,
    NULL,
    &engine->descriptor_set_layout
  );
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create Vulkan descriptor layout: %d.", result);
    return MOSS_RESULT_ERROR;
  }
  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__allocate_descriptor_sets (MossEngine *const engine)
{
  VkDescriptorSetLayout layouts[ MAX_FRAMES_IN_FLIGHT ];
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    layouts[ i ] = engine->descriptor_set_layout;
  }

  const VkDescriptorSetAllocateInfo alloc_info = {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
    .pSetLayouts        = layouts,
    .descriptorPool     = engine->descriptor_pool
  };

  const VkResult result =
    vkAllocateDescriptorSets (engine->device, &alloc_info, engine->descriptor_sets);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to allocate descriptor sets: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void moss__configure_descriptor_sets (MossEngine *const engine)
{
  VkDescriptorBufferInfo buffer_infos[ MAX_FRAMES_IN_FLIGHT ];
  VkDescriptorImageInfo  image_infos[ MAX_FRAMES_IN_FLIGHT ];
  VkWriteDescriptorSet   descriptor_writes[ MAX_FRAMES_IN_FLIGHT * 2 ];

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements (
      engine->device,
      engine->camera_ubo_buffers[ i ],
      &memory_requirements
    );

    buffer_infos[ i ] = (VkDescriptorBufferInfo) {
      .offset = 0,
      .buffer = engine->camera_ubo_buffers[ i ],
      .range  = memory_requirements.size,
    };

    descriptor_writes[ i ] = (VkWriteDescriptorSet) {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = engine->descriptor_sets[ i ],
      .dstBinding      = 0,
      .dstArrayElement = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .pBufferInfo     = &buffer_infos[ i ],
    };
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    image_infos[ i ] = (VkDescriptorImageInfo) {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .sampler     = engine->sampler,
      .imageView   = engine->texture_image_view,
    };

    descriptor_writes[ MAX_FRAMES_IN_FLIGHT + i ] = (VkWriteDescriptorSet) {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = engine->descriptor_sets[ i ],
      .dstBinding      = 1,
      .dstArrayElement = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .pImageInfo      = &image_infos[ i ],
    };
  }

  vkUpdateDescriptorSets (
    engine->device,
    sizeof (descriptor_writes) / sizeof (descriptor_writes[ 0 ]),
    descriptor_writes,
    0,
    NULL
  );
}

inline static MossResult moss__create_graphics_pipeline (MossEngine *const engine)
{
  VkShaderModule vert_shader_module;
  VkShaderModule frag_shader_module;

  {
    const Moss__CreateShaderModuleFromFileInfo create_info = {
      .device            = engine->device,
      .file_path         = MOSS__VERT_SHADER_PATH,
      .out_shader_module = &vert_shader_module,
    };
    if (moss_vk__create_shader_module_from_file (&create_info) != VK_SUCCESS)
    {
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const Moss__CreateShaderModuleFromFileInfo create_info = {
      .device            = engine->device,
      .file_path         = MOSS__FRAG_SHADER_PATH,
      .out_shader_module = &frag_shader_module,
    };
    if (moss_vk__create_shader_module_from_file (&create_info) != VK_SUCCESS)
    {
      vkDestroyShaderModule (engine->device, vert_shader_module, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  const VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vert_shader_module,
    .pName  = "main",
  };

  const VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = frag_shader_module,
    .pName  = "main",
  };

  const VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info,
                                                            frag_shader_stage_info };

  const VkPipelineVertexInputStateCreateInfo vertex_input_info =
    moss__create_vk_pipeline_vertex_input_state_info ( );

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

  const VkDescriptorSetLayout set_layouts[] = { engine->descriptor_set_layout };

  const VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = sizeof (set_layouts) / sizeof (set_layouts[ 0 ]),
    .pSetLayouts            = set_layouts,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = NULL,
  };

  if (vkCreatePipelineLayout (
        engine->device,
        &pipeline_layout_info,
        NULL,
        &engine->pipeline_layout
      ) != VK_SUCCESS)
  {
    moss__error ("Failed to create pipeline layout.\n");
    vkDestroyShaderModule (engine->device, frag_shader_module, NULL);
    vkDestroyShaderModule (engine->device, vert_shader_module, NULL);
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

  const VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount          = 2,
    .pStages             = shader_stages,
    .pVertexInputState   = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState      = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState   = &multisampling,
    .pColorBlendState    = &color_blending,
    .pDynamicState       = &dynamic_state,
    .layout              = engine->pipeline_layout,
    .renderPass          = engine->render_pass,
    .subpass             = 0,
    .basePipelineHandle  = VK_NULL_HANDLE,
    .basePipelineIndex   = -1,
  };

  const VkResult result = vkCreateGraphicsPipelines (
    engine->device,
    VK_NULL_HANDLE,
    1,
    &pipeline_info,
    NULL,
    &engine->graphics_pipeline
  );

  vkDestroyShaderModule (engine->device, frag_shader_module, NULL);
  vkDestroyShaderModule (engine->device, vert_shader_module, NULL);

  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create graphics pipeline. Error code: %d.\n", result);
    vkDestroyPipelineLayout (engine->device, engine->pipeline_layout, NULL);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_framebuffers (MossEngine *const engine)
{
  for (uint32_t i = 0; i < engine->swapchain_image_count; ++i)
  {
    const VkImageView attachments[] = { engine->swapchain_image_views[ i ] };

    const VkFramebufferCreateInfo framebuffer_info = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = engine->render_pass,
      .attachmentCount = 1,
      .pAttachments    = attachments,
      .width           = engine->swapchain_extent.width,
      .height          = engine->swapchain_extent.height,
      .layers          = 1,
    };

    if (vkCreateFramebuffer (
          engine->device,
          &framebuffer_info,
          NULL,
          &engine->swapchain_framebuffers[ i ]
        ) != VK_SUCCESS)
    {
      moss__error ("Failed to create framebuffer %u.\n", i);
      for (uint32_t j = 0; j < i; ++j)
      {
        vkDestroyFramebuffer (engine->device, engine->swapchain_framebuffers[ j ], NULL);
      }
      return MOSS_RESULT_ERROR;
    }
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_texture_image (MossEngine *const engine)
{
  // Load texture
  int texture_width, texture_height, texture_channels;  // NOLINT

  const stbi_uc *const pixels = stbi_load (
    "textures/atlas.png",
    &texture_width,
    &texture_height,
    &texture_channels,
    STBI_rgb_alpha
  );
  if (pixels == NULL)
  {
    moss__error ("Failed to load texture.");
    return MOSS_RESULT_ERROR;
  }

  VkBuffer       staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  {  // Create staging buffer
    const Moss__CreateVkBufferInfo create_info = {
      .physical_device = engine->physical_device,
      .device          = engine->device,
      .size            = (VkDeviceSize)(texture_width * texture_height * 4),
      .usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .memory_properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      .sharing_mode                    = engine->buffer_sharing_mode,
      .shared_queue_family_index_count = engine->shared_queue_family_index_count,
      .shared_queue_family_indices     = engine->shared_queue_family_indices,
    };
    const MossResult result =
      moss_vk__create_buffer (&create_info, &staging_buffer, &staging_buffer_memory);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create staging buffer.");
      return MOSS_RESULT_ERROR;
    }
  }

  // Copy pixels into the buffer
  void *data;
  vkMapMemory (
    engine->device,
    staging_buffer_memory,
    0,
    (VkDeviceSize)(texture_width * texture_height * 4),
    0,
    &data
  );
  memcpy (data, pixels, (size_t)(texture_width * texture_height * 4));
  vkUnmapMemory (engine->device, staging_buffer_memory);

  // Free pixels
  stbi_image_free ((void *)pixels);

  {  // It's time for texture image!
    const VkImageCreateInfo create_info = {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent =
        (VkExtent3D) { .width = texture_width, .height = texture_height, .depth = 1 },
      .mipLevels     = 1,
      .arrayLayers   = 1,
      .format        = VK_FORMAT_R8G8B8A8_SRGB,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode   = engine->buffer_sharing_mode,
      .queueFamilyIndexCount = engine->shared_queue_family_index_count,
      .pQueueFamilyIndices   = engine->shared_queue_family_indices,
      .samples               = VK_SAMPLE_COUNT_1_BIT,
    };
    const VkResult result =
      vkCreateImage (engine->device, &create_info, NULL, &engine->texture_image);
    if (result != VK_SUCCESS)
    {
      moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);
      moss__error ("Failed to create image: %d.\n", result);
      return MOSS_RESULT_ERROR;
    }
  }

  {  // And allocating some memory
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements (
      engine->device,
      engine->texture_image,
      &memory_requirements
    );

    uint32_t suitable_memory_type;
    if (moss__select_suitable_memory_type (
          engine->physical_device,
          memory_requirements.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          &suitable_memory_type
        ) != MOSS_RESULT_SUCCESS)
    {
      moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);
      vkDestroyImage (engine->device, engine->texture_image, NULL);
      moss__error ("Failed to find suitable memory type.");
      return MOSS_RESULT_ERROR;
    }

    const VkMemoryAllocateInfo alloc_info = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize  = memory_requirements.size,
      .memoryTypeIndex = suitable_memory_type,
    };
    const VkResult result =
      vkAllocateMemory (engine->device, &alloc_info, NULL, &engine->texture_image_memory);
    if (result != VK_SUCCESS)
    {
      moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);
      vkDestroyImage (engine->device, engine->texture_image, NULL);
      moss__error ("Failed to allocate memory for the texture: %d.", result);
      return MOSS_RESULT_ERROR;
    }
  }

  vkBindImageMemory (
    engine->device,
    engine->texture_image,
    engine->texture_image_memory,
    0
  );

  {
    const Moss__TransitionVkImageLayoutInfo transition_info = {
      .device         = engine->device,
      .command_pool   = engine->transfer_command_pool,
      .transfer_queue = engine->transfer_queue,
      .image          = engine->texture_image,
      .old_layout     = VK_IMAGE_LAYOUT_UNDEFINED,
      .new_layout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    };
    if (moss_vk__transition_image_layout (&transition_info) != MOSS_RESULT_SUCCESS)
    {
      moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);
      vkFreeMemory (engine->device, engine->texture_image_memory, NULL);
      vkDestroyImage (engine->device, engine->texture_image, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const Moss__CopyVkBufferToImageInfo copy_info = {
      .device         = engine->device,
      .command_pool   = engine->transfer_command_pool,
      .transfer_queue = engine->transfer_queue,
      .buffer         = staging_buffer,
      .image          = engine->texture_image,
      .width          = texture_width,
      .height         = texture_height,
    };
    if (moss_vk__copy_buffer_to_image (&copy_info) != MOSS_RESULT_SUCCESS)
    {
      moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);
      vkFreeMemory (engine->device, engine->texture_image_memory, NULL);
      vkDestroyImage (engine->device, engine->texture_image, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const Moss__TransitionVkImageLayoutInfo transition_info = {
      .device         = engine->device,
      .command_pool   = engine->transfer_command_pool,
      .transfer_queue = engine->transfer_queue,
      .image          = engine->texture_image,
      .old_layout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .new_layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    if (moss_vk__transition_image_layout (&transition_info) != MOSS_RESULT_SUCCESS)
    {
      moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);
      vkFreeMemory (engine->device, engine->texture_image_memory, NULL);
      vkDestroyImage (engine->device, engine->texture_image, NULL);
      return MOSS_RESULT_ERROR;
    }
  }

  moss_vk__destroy_buffer (engine->device, staging_buffer, staging_buffer_memory);

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_texture_image_view (MossEngine *const engine)
{
  const Moss__VkImageViewCreateInfo info = {
    .device = engine->device,
    .image  = engine->texture_image,
    .format = VK_FORMAT_R8G8B8A8_SRGB,
  };
  const VkImageView image_view = moss_vk__create_image_view (&info);

  if (image_view == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  engine->texture_image_view = image_view;
  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_texture_sampler (MossEngine *const engine)
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
    vkCreateSampler (engine->device, &create_info, NULL, &engine->sampler);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create sampler: %d.", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_vertex_buffer (MossEngine *const engine)
{
  const Moss__CreateVkBufferInfo create_info = {
    .physical_device = engine->physical_device,
    .device          = engine->device,
    .size            = sizeof (g_verticies),
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .memory_properties               = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    .sharing_mode                    = engine->buffer_sharing_mode,
    .shared_queue_family_index_count = engine->shared_queue_family_index_count,
    .shared_queue_family_indices     = engine->shared_queue_family_indices,
  };

  const MossResult result = moss_vk__create_buffer (
    &create_info,
    &engine->vertex_buffer,
    &engine->vertex_buffer_memory
  );
  if (result != MOSS_RESULT_SUCCESS)
  {
    moss__error ("Failed to create vertex buffer.\n");
  }

  return result;
}

inline static MossResult moss__fill_vertex_buffer (MossEngine *const engine)
{
  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements (
    engine->device,
    engine->vertex_buffer,
    &memory_requirements
  );

  const Moss__FillVkBufferInfo fill_info = {
    .physical_device                 = engine->physical_device,
    .device                          = engine->device,
    .destination_buffer              = engine->vertex_buffer,
    .buffer_size                     = memory_requirements.size,
    .source_data                     = (void *)g_verticies,
    .data_size                       = sizeof (g_verticies),
    .command_pool                    = engine->transfer_command_pool,
    .transfer_queue                  = engine->transfer_queue,
    .sharing_mode                    = engine->buffer_sharing_mode,
    .shared_queue_family_index_count = engine->shared_queue_family_index_count,
    .shared_queue_family_indices     = engine->shared_queue_family_indices,
  };

  return moss_vk__fill_buffer (&fill_info);
}

inline static MossResult moss__create_index_buffer (MossEngine *const engine)
{
  const Moss__CreateVkBufferInfo create_info = {
    .physical_device = engine->physical_device,
    .device          = engine->device,
    .size            = sizeof (g_indices),
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    .memory_properties               = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    .sharing_mode                    = engine->buffer_sharing_mode,
    .shared_queue_family_index_count = engine->shared_queue_family_index_count,
    .shared_queue_family_indices     = engine->shared_queue_family_indices,
  };

  const MossResult result = moss_vk__create_buffer (
    &create_info,
    &engine->index_buffer,
    &engine->index_buffer_memory
  );
  if (result != MOSS_RESULT_SUCCESS) { moss__error ("Failed to create index buffer.\n"); }

  return result;
}

inline static MossResult moss__fill_index_buffer (MossEngine *const engine)
{
  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements (
    engine->device,
    engine->index_buffer,
    &memory_requirements
  );

  const Moss__FillVkBufferInfo fill_info = {
    .physical_device                 = engine->physical_device,
    .device                          = engine->device,
    .destination_buffer              = engine->index_buffer,
    .buffer_size                     = memory_requirements.size,
    .source_data                     = (void *)g_indices,
    .data_size                       = sizeof (g_indices),
    .command_pool                    = engine->transfer_command_pool,
    .transfer_queue                  = engine->transfer_queue,
    .sharing_mode                    = engine->buffer_sharing_mode,
    .shared_queue_family_index_count = engine->shared_queue_family_index_count,
    .shared_queue_family_indices     = engine->shared_queue_family_indices,
  };

  return moss_vk__fill_buffer (&fill_info);
}

inline static MossResult moss__create_camera_ubo_buffers (MossEngine *const engine)
{
  const Moss__CreateVkBufferInfo create_info = {
    .physical_device = engine->physical_device,
    .device          = engine->device,
    .size            = sizeof (engine->camera),
    .usage           = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    .memory_properties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    .sharing_mode                    = engine->buffer_sharing_mode,
    .shared_queue_family_index_count = engine->shared_queue_family_index_count,
    .shared_queue_family_indices     = engine->shared_queue_family_indices,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const MossResult result = moss_vk__create_buffer (
      &create_info,
      &engine->camera_ubo_buffers[ i ],
      &engine->camera_ubo_memories[ i ]
    );
    if (result != MOSS_RESULT_SUCCESS)
    {
      for (uint32_t j = i; j >= 0; --j)
      {
        moss_vk__destroy_buffer (
          engine->device,
          engine->camera_ubo_buffers[ j ],
          engine->camera_ubo_memories[ j ]
        );
      }
      moss__error ("Failed to create camera UBO buffer.\n");
      return result;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements (
      engine->device,
      engine->camera_ubo_buffers[ i ],
      &memory_requirements
    );

    vkMapMemory (
      engine->device,
      engine->camera_ubo_memories[ i ],
      0,
      memory_requirements.size,
      0,
      &engine->camera_ubo_buffer_mapped_memory_blocks[ i ]
    );
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_general_command_buffers (MossEngine *const engine)
{
  const VkCommandBufferAllocateInfo alloc_info = {
    .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool        = engine->general_command_pool,
    .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };

  const VkResult result = vkAllocateCommandBuffers (
    engine->device,
    &alloc_info,
    engine->general_command_buffers
  );
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to allocate command buffers. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void moss__cleanup_swapchain_framebuffers (MossEngine *const engine)
{
  for (uint32_t i = 0; i < engine->swapchain_image_count; ++i)
  {
    if (engine->swapchain_framebuffers[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyFramebuffer (engine->device, engine->swapchain_framebuffers[ i ], NULL);
    engine->swapchain_framebuffers[ i ] = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanup_swapchain_image_views (MossEngine *const engine)
{
  for (uint32_t i = 0; i < engine->swapchain_image_count; ++i)
  {
    if (engine->swapchain_image_views[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyImageView (engine->device, engine->swapchain_image_views[ i ], NULL);
    engine->swapchain_image_views[ i ] = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanup_swapchain_handle (MossEngine *const engine)
{
  if (engine->swapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR (engine->device, engine->swapchain, NULL);
    engine->swapchain = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanup_swapchain (MossEngine *const engine)
{
  moss__cleanup_swapchain_framebuffers (engine);
  moss__cleanup_swapchain_image_views (engine);
  moss__cleanup_swapchain_handle (engine);

  engine->swapchain_image_count  = 0;
  engine->swapchain_image_format = 0;
  engine->swapchain_extent       = (VkExtent2D) { .width = 0, .height = 0 };
}

inline static MossResult moss__recreate_swapchain (
  MossEngine *const engine,
  const uint32_t    width,
  const uint32_t    height
)
{
  vkDeviceWaitIdle (engine->device);

  moss__cleanup_swapchain (engine);

  if (moss__create_swapchain (engine, width, height) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__create_swapchain_image_views (engine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }
  if (moss__create_framebuffers (engine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void moss__update_camera_ubo_data (MossEngine *const engine)
{
  memcpy (
    engine->camera_ubo_buffer_mapped_memory_blocks[ engine->current_frame ],
    &engine->camera,
    sizeof (engine->camera)
  );
}

inline static MossResult
moss__create_image_available_semaphores (MossEngine *const engine)
{
  if (engine->device == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  const VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const VkResult result = vkCreateSemaphore (
      engine->device,
      &semaphore_info,
      NULL,
      &engine->image_available_semaphores[ i ]
    );
    if (result == VK_SUCCESS) { continue; }

    moss__error ("Failed to create image available semaphore for frame %u.\n", i);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult
moss__create_render_finished_semaphores (MossEngine *const engine)
{
  if (engine->device == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  const VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const VkResult result = vkCreateSemaphore (
      engine->device,
      &semaphore_info,
      NULL,
      &engine->render_finished_semaphores[ i ]
    );
    if (result == VK_SUCCESS) { continue; }

    moss__error ("Failed to create render finished semaphore for frame %u.\n", i);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_in_flight_fences (MossEngine *const engine)
{
  if (engine->device == VK_NULL_HANDLE) { return MOSS_RESULT_ERROR; }

  const VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    const VkResult result =
      vkCreateFence (engine->device, &fence_info, NULL, &engine->in_flight_fences[ i ]);
    if (result == VK_SUCCESS) { continue; }

    moss__error ("Failed to create in-flight fence for frame %u.\n", i);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static MossResult moss__create_synchronization_objects (MossEngine *const engine)
{
  if (moss__create_image_available_semaphores (engine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  if (moss__create_render_finished_semaphores (engine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  if (moss__create_in_flight_fences (engine) != MOSS_RESULT_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void
moss__cleanup_semaphores (MossEngine *const engine, VkSemaphore *semaphores)
{
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (semaphores[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroySemaphore (engine->device, semaphores[ i ], NULL);
    semaphores[ i ] = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanup_fences (MossEngine *const engine, VkFence *fences)
{
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    if (fences[ i ] == VK_NULL_HANDLE) { continue; }

    vkDestroyFence (engine->device, fences[ i ], NULL);
    fences[ i ] = VK_NULL_HANDLE;
  }
}

inline static void moss__cleanup_image_available_semaphores (MossEngine *const engine)
{
  moss__cleanup_semaphores (engine, engine->image_available_semaphores);
}

inline static void moss__cleanup_render_finished_semaphores (MossEngine *const engine)
{
  moss__cleanup_semaphores (engine, engine->render_finished_semaphores);
}

inline static void moss__cleanup_in_flight_fences (MossEngine *const engine)
{
  moss__cleanup_fences (engine, engine->in_flight_fences);
}

inline static void moss__cleanup_synchronization_objects (MossEngine *const engine)
{
  moss__cleanup_in_flight_fences (engine);
  moss__cleanup_render_finished_semaphores (engine);
  moss__cleanup_image_available_semaphores (engine);
}
