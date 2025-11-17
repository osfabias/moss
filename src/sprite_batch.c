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

  @file src/sprite_batch.c
  @brief Sprite batch struct body declaration and sprite batch
         related functions implementation.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "moss/engine.h"
#include "moss/result.h"
#include "moss/sprite.h"
#include "moss/sprite_batch.h"

#include "moss/vertex.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/buffer.h"
#include "src/internal/vulkan/utils/command_buffer.h"
#include "vulkan/vulkan_core.h"

#define MOSS__VERTICIES_PER_SPRITE (size_t)(4)

#define MOSS__INDICES_PER_SPRITE (size_t)(6)

/*=============================================================================
    INTERNAL STRUCT DECLARATIONS
  =============================================================================*/

struct MossSpriteBatch
{
  MossEngine    *original_engine;    /* Engine where this batch was created on. */
  VkBuffer       buffer;             /* Combined vertex and index buffer. */
  VkDeviceMemory buffer_memory;      /* Combined buffer memory. */
  VkBuffer       staging_buffer;     /* Staging buffer. */
  VkDeviceMemory staging_memory;     /* Staging buffer memory. */
  void          *mapped_memory;      /* Mapped staging buffer memory. */
  size_t         buffer_capacity;    /* Total buffer capacity in bytes. */
  size_t         vertex_data_offset; /* Offset where vertex data starts in buffer. */
  size_t         index_data_offset;  /* Offset where index data starts in buffer. */
  size_t         vertex_data_size;   /* Current vertex data size in bytes. */
  size_t         index_data_size;    /* Current index data size in bytes. */
  size_t         vertex_capacity;    /* Maximum vertex capacity in bytes. */
  size_t         index_capacity;     /* Maximum index capacity in bytes. */
  uint32_t       index_count;        /* Number of indices. */
  bool           is_begun;           /* Whether begin has been called. */
};

/*
  @brief Create combined buffer info.
*/
typedef struct
{
  MossEngine *engine; /* Engine handle */
  size_t      size;   /* Desired size of the buffer in bytes. */
} Moss__CreateCombinedBufferInfo;

/*=============================================================================
    PRIVATE FUNCTION DECLARATIONS
  =============================================================================*/

/*
  @brief Creates combined vertex and index buffer.
  @param info Required operation info.
  @param out_buffer Output buffer.
  @param out_buffer_memory Output buffer memory.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult moss__create_combined_buffer (
  const Moss__CreateCombinedBufferInfo *info,
  VkBuffer                             *out_buffer,
  VkDeviceMemory                       *out_buffer_memory
);

/*=============================================================================
    PUBLIC FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossSpriteBatch *moss_create_sprite_batch (const MossSpriteBatchCreateInfo *const info)
{
  MossSpriteBatch *const sprite_batch = malloc (sizeof (MossSpriteBatch));
  if (sprite_batch == NULL)
  {
    moss__error ("Failed to allocate memory for a sprite batch.\n");
    return NULL;
  }

  // Calculate buffer sizes
  const size_t vertex_data_size =
    info->capacity * sizeof (MossVertex) * MOSS__VERTICIES_PER_SPRITE;
  const size_t index_data_size =
    info->capacity * sizeof (uint16_t) * MOSS__INDICES_PER_SPRITE;
  const size_t total_buffer_size = vertex_data_size + index_data_size;

  // Vertices come first, then indices
  const size_t vertex_data_offset = 0;
  const size_t index_data_offset  = vertex_data_size;

  {  // Create combined buffer
    const Moss__CreateCombinedBufferInfo buffer_info = {
      .engine = info->engine,
      .size   = total_buffer_size,
    };
    const MossResult result = moss__create_combined_buffer (
      &buffer_info,
      &sprite_batch->buffer,
      &sprite_batch->buffer_memory
    );

    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create combined buffer for sprite batch.\n");
      free (sprite_batch);
      return NULL;
    }
  }

  // Save original engine
  sprite_batch->original_engine = info->engine;

  // Set default field values
  sprite_batch->buffer_capacity    = total_buffer_size;
  sprite_batch->vertex_data_offset = vertex_data_offset;
  sprite_batch->index_data_offset  = index_data_offset;
  sprite_batch->vertex_data_size   = 0;
  sprite_batch->index_data_size    = 0;
  sprite_batch->vertex_capacity    = vertex_data_size;
  sprite_batch->index_capacity     = index_data_size;
  sprite_batch->index_count        = 0;
  sprite_batch->is_begun           = false;
  sprite_batch->staging_buffer     = VK_NULL_HANDLE;
  sprite_batch->staging_memory     = VK_NULL_HANDLE;
  sprite_batch->mapped_memory      = NULL;

  return sprite_batch;
}


void moss_destroy_sprite_batch (MossSpriteBatch *const sprite_batch)
{
  if (sprite_batch == NULL) { return; }

  MossEngine *const engine = sprite_batch->original_engine;

  // Wait until device finishes all his work
  vkDeviceWaitIdle (engine->device);

  // Cleanup staging buffer if it exists
  if (sprite_batch->staging_buffer != VK_NULL_HANDLE)
  {
    if (sprite_batch->mapped_memory != NULL)
    {
      vkUnmapMemory (engine->device, sprite_batch->staging_memory);
    }
    moss_vk__destroy_buffer (
      engine->device,
      sprite_batch->staging_buffer,
      sprite_batch->staging_memory
    );
  }

  // Cleanup device-local buffer
  moss_vk__destroy_buffer (
    engine->device,
    sprite_batch->buffer,
    sprite_batch->buffer_memory
  );

  free (sprite_batch);
}

void moss_clear_sprite_batch (MossSpriteBatch *sprite_batch)
{
  sprite_batch->vertex_data_size = 0;
  sprite_batch->index_data_size  = 0;
  sprite_batch->index_count      = 0;
  sprite_batch->is_begun         = false;
}

MossResult moss_begin_sprite_batch (MossSpriteBatch *sprite_batch)
{
  if (sprite_batch->is_begun)
  {
    moss__error ("Sprite batch already begun.\n");
    return MOSS_RESULT_ERROR;
  }

  MossEngine *const engine = sprite_batch->original_engine;

  // Create staging buffer
  {
    const Moss__CreateVkBufferInfo create_info = {
      .physical_device = engine->physical_device,
      .device          = engine->device,
      .size            = (VkDeviceSize)sprite_batch->buffer_capacity,
      .usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .memory_properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      .sharing_mode                    = engine->buffer_sharing_mode,
      .shared_queue_family_index_count = engine->shared_queue_family_index_count,
      .shared_queue_family_indices     = engine->shared_queue_family_indices,
    };

    const MossResult result = moss_vk__create_buffer (
      &create_info,
      &sprite_batch->staging_buffer,
      &sprite_batch->staging_memory
    );
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create staging buffer.\n");
      return result;
    }
  }

  {  // Map staging buffer memory
    const VkResult result = vkMapMemory (
      engine->device,
      sprite_batch->staging_memory,
      0,
      (VkDeviceSize)sprite_batch->buffer_capacity,
      0,
      &sprite_batch->mapped_memory
    );
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to map staging buffer memory: %d.\n", result);
      moss_vk__destroy_buffer (
        engine->device,
        sprite_batch->staging_buffer,
        sprite_batch->staging_memory
      );
      return MOSS_RESULT_ERROR;
    }
  }

  sprite_batch->is_begun         = true;
  sprite_batch->vertex_data_size = 0;
  sprite_batch->index_data_size  = 0;
  sprite_batch->index_count      = 0;

  return MOSS_RESULT_SUCCESS;
}

MossResult moss_add_sprites_to_sprite_batch (
  MossSpriteBatch *const                       sprite_batch,
  const MossAddSpritesToSpriteBatchInfo *const info
)
{
  if (!sprite_batch->is_begun)
  {
    moss__error ("Sprite batch not begun. Call moss_begin_sprite_batch first.\n");
    return MOSS_RESULT_ERROR;
  }

  if (sprite_batch->mapped_memory == NULL)
  {
    moss__error ("Staging buffer not mapped.\n");
    return MOSS_RESULT_ERROR;
  }

  MossVertex *vertices =
    (MossVertex *)((char *)sprite_batch->mapped_memory +
                   sprite_batch->vertex_data_offset + sprite_batch->vertex_data_size);
  uint16_t *indices =
    (uint16_t *)((char *)sprite_batch->mapped_memory + sprite_batch->index_data_offset +
                 sprite_batch->index_data_size);
  uint16_t base_vertex =
    (uint16_t)((sprite_batch->vertex_data_size) / sizeof (MossVertex));

  // Generate vertices and indices for each sprite
  for (size_t i = 0; i < info->sprite_count; ++i)
  {
    const MossSprite *sprite = &info->sprites[ i ];

    // Calculate sprite corners in world space
    const float half_width  = sprite->size[ 0 ] * 0.5F;
    const float half_height = sprite->size[ 1 ] * 0.5F;

    const float x_min = sprite->position[ 0 ] - half_width;
    const float x_max = sprite->position[ 0 ] + half_width;
    const float y_min = sprite->position[ 1 ] - half_height;
    const float y_max = sprite->position[ 1 ] + half_height;

    // Create 4 vertices: top-left, top-right, bottom-right, bottom-left
    vertices[ 0 ] = (MossVertex) {
      .position       = { x_min, y_max }, // Top-left
      .color          = { 1.0F, 1.0F, 1.0F },
      .texture_coords = { sprite->uv.top_left[ 0 ], sprite->uv.top_left[ 1 ] },
    };
    vertices[ 1 ] = (MossVertex) {
      .position       = { x_max, y_max }, // Top-right
      .color          = { 1.0F, 1.0F, 1.0F },
      .texture_coords = { sprite->uv.bottom_right[ 0 ], sprite->uv.top_left[ 1 ] },
    };
    vertices[ 2 ] = (MossVertex) {
      .position       = { x_max, y_min }, // Bottom-right
      .color          = { 1.0F, 1.0F, 1.0F },
      .texture_coords = { sprite->uv.bottom_right[ 0 ], sprite->uv.bottom_right[ 1 ] },
    };
    vertices[ 3 ] = (MossVertex) {
      .position       = { x_min, y_min }, // Bottom-left
      .color          = { 1.0F, 1.0F, 1.0F },
      .texture_coords = { sprite->uv.top_left[ 0 ], sprite->uv.bottom_right[ 1 ] },
    };

    // Create indices: two triangles (0,1,2) and (2,3,0)
    indices[ 0 ] = base_vertex + 0;
    indices[ 1 ] = base_vertex + 1;
    indices[ 2 ] = base_vertex + 2;
    indices[ 3 ] = base_vertex + 2;
    indices[ 4 ] = base_vertex + 3;
    indices[ 5 ] = base_vertex + 0;

    vertices += MOSS__VERTICIES_PER_SPRITE;
    indices += MOSS__INDICES_PER_SPRITE;
    base_vertex += MOSS__VERTICIES_PER_SPRITE;
    sprite_batch->vertex_data_size += sizeof (MossVertex) * MOSS__VERTICIES_PER_SPRITE;
    sprite_batch->index_data_size += sizeof (uint16_t) * MOSS__INDICES_PER_SPRITE;
    sprite_batch->index_count += MOSS__INDICES_PER_SPRITE;
  }

  return MOSS_RESULT_SUCCESS;
}

MossResult moss_end_sprite_batch (MossSpriteBatch *sprite_batch)
{
  if (!sprite_batch->is_begun)
  {
    moss__error ("Sprite batch not begun. Call moss_begin_sprite_batch first.\n");
    return MOSS_RESULT_ERROR;
  }

  MossEngine *const engine = sprite_batch->original_engine;

  // Unmap staging buffer memory
  if (sprite_batch->mapped_memory != NULL)
  {
    vkUnmapMemory (engine->device, sprite_batch->staging_memory);
    sprite_batch->mapped_memory = NULL;
  }

  // Copy from staging buffer to device-local buffer with offsets
  VkCommandBuffer command_buffer;
  {
    const Moss__BeginOneTimeVkCommandBufferInfo begin_info = {
      .device       = engine->device,
      .command_pool = engine->transfer_command_pool,
    };
    command_buffer = moss_vk__begin_one_time_command_buffer (&begin_info);
    if (command_buffer == VK_NULL_HANDLE)
    {
      moss__error ("Failed to begin one time command buffer for sprite batch copy.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  // Copy vertex data
  if (sprite_batch->vertex_data_size > 0)
  {
    const VkBufferCopy vertex_copy_region = {
      .srcOffset = sprite_batch->vertex_data_offset,
      .dstOffset = sprite_batch->vertex_data_offset,
      .size      = (VkDeviceSize)sprite_batch->vertex_data_size,
    };
    vkCmdCopyBuffer (
      command_buffer,
      sprite_batch->staging_buffer,
      sprite_batch->buffer,
      1,
      &vertex_copy_region
    );
  }

  // Copy index data
  if (sprite_batch->index_data_size > 0)
  {
    const VkBufferCopy index_copy_region = {
      .srcOffset = sprite_batch->index_data_offset,
      .dstOffset = sprite_batch->index_data_offset,
      .size      = (VkDeviceSize)sprite_batch->index_data_size,
    };
    vkCmdCopyBuffer (
      command_buffer,
      sprite_batch->staging_buffer,
      sprite_batch->buffer,
      1,
      &index_copy_region
    );
  }

  {
    const Moss__EndOneTimeVkCommandBufferInfo end_info = {
      .device         = engine->device,
      .command_pool   = engine->transfer_command_pool,
      .command_buffer = command_buffer,
      .queue          = engine->transfer_queue,
    };
    const MossResult result = moss_vk__end_one_time_command_buffer (&end_info);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time command buffer for sprite batch copy.\n");
      return result;
    }
  }

  // Cleanup staging buffer
  moss_vk__destroy_buffer (
    engine->device,
    sprite_batch->staging_buffer,
    sprite_batch->staging_memory
  );

  sprite_batch->staging_buffer = VK_NULL_HANDLE;
  sprite_batch->staging_memory = VK_NULL_HANDLE;
  sprite_batch->is_begun       = false;

  return MOSS_RESULT_SUCCESS;
}

MossResult
moss_draw_sprite_batch (MossEngine *const engine, MossSpriteBatch *const sprite_batch)
{
  if (sprite_batch == NULL || engine == NULL)
  {
    moss__error ("Invalid parameters to moss_draw_sprite_batch.\n");
    return MOSS_RESULT_ERROR;
  }

  if (sprite_batch->index_count == 0) { return MOSS_RESULT_SUCCESS; }

  if (sprite_batch->is_begun)
  {
    moss__error ("Sprite batch not ended. Call moss_end_sprite_batch first.\n");
    return MOSS_RESULT_ERROR;
  }

  if (sprite_batch->original_engine != engine)
  {
    moss__error ("Sprite batch created with different engine.\n");
    return MOSS_RESULT_ERROR;
  }

  // Get the command buffer that's currently being recorded
  // This assumes the command buffer is already in recording state
  const VkCommandBuffer command_buffer =
    engine->general_command_buffers[ engine->current_frame ];

  // Bind vertex buffer with offset
  const VkBuffer     vertex_buffers[]        = { sprite_batch->buffer };
  const VkDeviceSize vertex_buffer_offsets[] = {
    (VkDeviceSize)sprite_batch->vertex_data_offset
  };
  vkCmdBindVertexBuffers (command_buffer, 0, 1, vertex_buffers, vertex_buffer_offsets);

  // Bind index buffer with offset
  vkCmdBindIndexBuffer (
    command_buffer,
    sprite_batch->buffer,
    (VkDeviceSize)sprite_batch->index_data_offset,
    VK_INDEX_TYPE_UINT16
  );

  // Draw indexed
  vkCmdDrawIndexed (command_buffer, sprite_batch->index_count, 1, 0, 0, 0);

  return MOSS_RESULT_SUCCESS;
}

/*=============================================================================
    PRIVATE FUNCTIONS IMPLEMENTATION
  =============================================================================*/

inline static MossResult moss__create_combined_buffer (
  const Moss__CreateCombinedBufferInfo *const info,
  VkBuffer                                   *out_buffer,
  VkDeviceMemory                             *out_buffer_memory
)
{
  const Moss__CreateVkBufferInfo create_info = {
    .physical_device = info->engine->physical_device,
    .device          = info->engine->device,
    .size            = (VkDeviceSize)info->size,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
             VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    .memory_properties               = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    .sharing_mode                    = info->engine->buffer_sharing_mode,
    .shared_queue_family_index_count = info->engine->shared_queue_family_index_count,
    .shared_queue_family_indices     = info->engine->shared_queue_family_indices,
  };

  const MossResult result =
    moss_vk__create_buffer (&create_info, out_buffer, out_buffer_memory);
  if (result != MOSS_RESULT_SUCCESS)
  {
    moss__error ("Failed to create combined buffer.\n");
  }

  return result;
}
