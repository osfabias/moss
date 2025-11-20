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

  @file src/spriteBatch.c
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

#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vertex.h"
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
  MossEngine    *pOriginalEngine;  /* Engine where this batch was created on. */
  VkBuffer       buffer;           /* Combined vertex and index buffer. */
  VkDeviceMemory bufferMemory;     /* Combined buffer memory. */
  VkBuffer       stagingBuffer;    /* Staging buffer. */
  VkDeviceMemory stagingMemory;    /* Staging buffer memory. */
  void          *pMappedMemory;    /* Mapped staging buffer memory. */
  size_t         bufferCapacity;   /* Total buffer capacity in bytes. */
  size_t         vertexDataOffset; /* Offset where vertex data starts in buffer. */
  size_t         indexDataOffset;  /* Offset where index data starts in buffer. */
  size_t         vertexDataSize;   /* Current vertex data size in bytes. */
  size_t         indexDataSize;    /* Current index data size in bytes. */
  size_t         vertexCapacity;   /* Maximum vertex capacity in bytes. */
  size_t         indexCapacity;    /* Maximum index capacity in bytes. */
  uint32_t       indexCount;       /* Number of indices. */
  bool           isBegun;          /* Whether begin has been called. */
};

/*
  @brief Create combined buffer info.
*/
typedef struct
{
  MossEngine *pEngine; /* Engine handle */
  size_t      size;    /* Desired size of the buffer in bytes. */
} Moss__CreateCombinedBufferInfo;

/*=============================================================================
    PRIVATE FUNCTION DECLARATIONS
  =============================================================================*/

/*
  @brief Creates combined vertex and index buffer.
  @param info Required operation info.
  @param outBuffer Output buffer.
  @param outBuffer_memory Output buffer memory.
  @return Returns MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
inline static MossResult mossCreateCombinedBuffer (
  const Moss__CreateCombinedBufferInfo *pInfo,
  VkBuffer                             *pOutBuffer,
  VkDeviceMemory                       *pOutBufferMemory
);

/*
  @brief Generates verticies from sprite.
  @param sprite Sprite to generate vertex data from.
  @param outVertices Output verticies.
  @return Always returns a pack of verticies.
*/
inline static void mossGenerateVerticiesFromSprite (
  const MossSprite *pSprite,
  Moss__Vertex      pOutVertices[ 4 ]
);


/*=============================================================================
    PUBLIC FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult mossCreateSpriteBatch (
  const MossSpriteBatchCreateInfo *const pInfo,
  MossSpriteBatch                      **ppOutSpriteBatch
)
{
  MossSpriteBatch *const pSpriteBatch = malloc (sizeof (MossSpriteBatch));
  if (pSpriteBatch == NULL)
  {
    moss__error ("Failed to allocate memory for a sprite batch.\n");
    return MOSS_RESULT_ERROR;
  }

  // Calculate buffer sizes
  const size_t vertexDataSize =
    pInfo->capacity * sizeof (Moss__Vertex) * MOSS__VERTICIES_PER_SPRITE;
  const size_t indexDataSize =
    pInfo->capacity * sizeof (uint16_t) * MOSS__INDICES_PER_SPRITE;
  const size_t totalBufferSize = vertexDataSize + indexDataSize;

  // Vertices come first, then indices
  const size_t vertexDataOffset = 0;
  const size_t indexDataOffset  = vertexDataSize;

  {  // Create combined buffer
    const Moss__CreateCombinedBufferInfo bufferInfo = {
      .pEngine = pInfo->pEngine,
      .size    = totalBufferSize,
    };
    const MossResult result = mossCreateCombinedBuffer (
      &bufferInfo,
      &pSpriteBatch->buffer,
      &pSpriteBatch->bufferMemory
    );

    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create combined buffer for sprite batch.\n");
      free (pSpriteBatch);
      return MOSS_RESULT_ERROR;
    }
  }

  {  // Create staging buffer
    {
      const MossVk__CreateBufferInfo createInfo = {
        .device                      = pInfo->pEngine->device,
        .size                        = (VkDeviceSize)totalBufferSize,
        .usage                       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode                 = pInfo->pEngine->bufferSharingMode,
        .sharedQueueFamilyIndexCount = pInfo->pEngine->sharedQueueFamilyIndexCount,
        .sharedQueueFamilyIndices    = pInfo->pEngine->sharedQueueFamilyIndices,
      };

      const MossResult result =
        mossVk__createBuffer (&createInfo, &pSpriteBatch->stagingBuffer);
      if (result != MOSS_RESULT_SUCCESS)
      {
        moss__error ("Failed to create staging buffer.\n");
        if (pSpriteBatch->bufferMemory != VK_NULL_HANDLE)
        {
          vkFreeMemory (pInfo->pEngine->device, pSpriteBatch->bufferMemory, NULL);
        }
        if (pSpriteBatch->buffer != VK_NULL_HANDLE)
        {
          vkDestroyBuffer (pInfo->pEngine->device, pSpriteBatch->buffer, NULL);
        }
        free (pSpriteBatch);
        return MOSS_RESULT_ERROR;
      }
    }

    {
      const MossVk__AllocateBufferMemoryInfo allocInfo = {
        .physicalDevice = pInfo->pEngine->physicalDevice,
        .device         = pInfo->pEngine->device,
        .buffer         = pSpriteBatch->stagingBuffer,
        .memoryProperties =
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      };

      const MossResult result =
        mossVk__allocateBufferMemory (&allocInfo, &pSpriteBatch->stagingMemory);
      if (result != MOSS_RESULT_SUCCESS)
      {
        vkDestroyBuffer (pInfo->pEngine->device, pSpriteBatch->stagingBuffer, NULL);
        if (pSpriteBatch->bufferMemory != VK_NULL_HANDLE)
        {
          vkFreeMemory (pInfo->pEngine->device, pSpriteBatch->bufferMemory, NULL);
        }
        if (pSpriteBatch->buffer != VK_NULL_HANDLE)
        {
          vkDestroyBuffer (pInfo->pEngine->device, pSpriteBatch->buffer, NULL);
        }
        free (pSpriteBatch);
        return MOSS_RESULT_ERROR;
      }
    }
  }

  {  // Map staging buffer memory
    const VkResult result = vkMapMemory (
      pInfo->pEngine->device,
      pSpriteBatch->stagingMemory,
      0,
      (VkDeviceSize)totalBufferSize,
      0,
      &pSpriteBatch->pMappedMemory
    );
    if (result != VK_SUCCESS)
    {
      moss__error ("Failed to map staging buffer memory: %d.\n", result);
      {
        if (pSpriteBatch->stagingMemory != VK_NULL_HANDLE)
        {
          vkFreeMemory (pInfo->pEngine->device, pSpriteBatch->stagingMemory, NULL);
        }
        if (pSpriteBatch->stagingBuffer != VK_NULL_HANDLE)
        {
          vkDestroyBuffer (pInfo->pEngine->device, pSpriteBatch->stagingBuffer, NULL);
        }
      }
      {
        if (pSpriteBatch->bufferMemory != VK_NULL_HANDLE)
        {
          vkFreeMemory (pInfo->pEngine->device, pSpriteBatch->bufferMemory, NULL);
        }
        if (pSpriteBatch->buffer != VK_NULL_HANDLE)
        {
          vkDestroyBuffer (pInfo->pEngine->device, pSpriteBatch->buffer, NULL);
        }
      }
      free (pSpriteBatch);
      return MOSS_RESULT_ERROR;
    }
  }

  // Save original engine
  pSpriteBatch->pOriginalEngine = pInfo->pEngine;

  // Set default field values
  pSpriteBatch->bufferCapacity   = totalBufferSize;
  pSpriteBatch->vertexDataOffset = vertexDataOffset;
  pSpriteBatch->indexDataOffset  = indexDataOffset;
  pSpriteBatch->vertexDataSize   = 0;
  pSpriteBatch->indexDataSize    = 0;
  pSpriteBatch->vertexCapacity   = vertexDataSize;
  pSpriteBatch->indexCapacity    = indexDataSize;
  pSpriteBatch->indexCount       = 0;
  pSpriteBatch->isBegun          = false;

  *ppOutSpriteBatch = pSpriteBatch;
  return MOSS_RESULT_SUCCESS;
}


void mossDestroySpriteBatch (MossSpriteBatch *const pSpriteBatch)
{
  if (pSpriteBatch == NULL) { return; }

  MossEngine *const pEngine = pSpriteBatch->pOriginalEngine;

  // Wait until device finishes all his work
  vkDeviceWaitIdle (pEngine->device);

  // Unmap and cleanup staging buffer
  vkUnmapMemory (pEngine->device, pSpriteBatch->stagingMemory);
  {
    if (pSpriteBatch->stagingMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory (pEngine->device, pSpriteBatch->stagingMemory, NULL);
    }
    if (pSpriteBatch->stagingBuffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer (pEngine->device, pSpriteBatch->stagingBuffer, NULL);
    }
  }

  // Cleanup device-local buffer
  {
    if (pSpriteBatch->bufferMemory != VK_NULL_HANDLE)
    {
      vkFreeMemory (pEngine->device, pSpriteBatch->bufferMemory, NULL);
    }
    if (pSpriteBatch->buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer (pEngine->device, pSpriteBatch->buffer, NULL);
    }
  }

  free (pSpriteBatch);
}

void mossClearSpriteBatch (MossSpriteBatch *pSpriteBatch)
{
  pSpriteBatch->vertexDataSize = 0;
  pSpriteBatch->indexDataSize  = 0;
  pSpriteBatch->indexCount     = 0;
  pSpriteBatch->isBegun        = false;
}

MossResult mossBeginSpriteBatch (MossSpriteBatch *pSpriteBatch)
{
  if (pSpriteBatch->isBegun)
  {
    moss__error ("Sprite batch already begun.\n");
    return MOSS_RESULT_ERROR;
  }

  if (pSpriteBatch->pMappedMemory == NULL)
  {
    moss__error ("Staging buffer not mapped.\n");
    return MOSS_RESULT_ERROR;
  }

  pSpriteBatch->isBegun        = true;
  pSpriteBatch->vertexDataSize = 0;
  pSpriteBatch->indexDataSize  = 0;
  pSpriteBatch->indexCount     = 0;

  return MOSS_RESULT_SUCCESS;
}

MossResult mossAddSpritesToSpriteBatch (
  MossSpriteBatch *const                       pSpriteBatch,
  const MossAddSpritesToSpriteBatchInfo *const pInfo
)
{
  if (!pSpriteBatch->isBegun)
  {
    moss__error ("Sprite batch not begun. Call mossBeginSpriteBatch first.\n");
    return MOSS_RESULT_ERROR;
  }

  if (pSpriteBatch->pMappedMemory == NULL)
  {
    moss__error ("Staging buffer not mapped.\n");
    return MOSS_RESULT_ERROR;
  }

  Moss__Vertex *pVertices =
    (Moss__Vertex *)((char *)pSpriteBatch->pMappedMemory +
                     pSpriteBatch->vertexDataOffset + pSpriteBatch->vertexDataSize);
  uint16_t *pIndices =
    (uint16_t *)((char *)pSpriteBatch->pMappedMemory + pSpriteBatch->indexDataOffset +
                 pSpriteBatch->indexDataSize);
  uint16_t base_vertex =
    (uint16_t)((pSpriteBatch->vertexDataSize) / sizeof (Moss__Vertex));

  // Generate vertices and indices for each sprite
  for (size_t i = 0; i < pInfo->spriteCount; ++i)
  {
    mossGenerateVerticiesFromSprite (&pInfo->pSprites[ i ], pVertices);

    // Create indices: two triangles (0,1,2) and (2,3,0)
    pIndices[ 0 ] = base_vertex + 0;
    pIndices[ 1 ] = base_vertex + 1;
    pIndices[ 2 ] = base_vertex + 2;
    pIndices[ 3 ] = base_vertex + 2;
    pIndices[ 4 ] = base_vertex + 3;
    pIndices[ 5 ] = base_vertex + 0;

    pVertices += MOSS__VERTICIES_PER_SPRITE;
    pIndices += MOSS__INDICES_PER_SPRITE;
    base_vertex += MOSS__VERTICIES_PER_SPRITE;
    pSpriteBatch->vertexDataSize += sizeof (Moss__Vertex) * MOSS__VERTICIES_PER_SPRITE;
    pSpriteBatch->indexDataSize += sizeof (uint16_t) * MOSS__INDICES_PER_SPRITE;
    pSpriteBatch->indexCount += MOSS__INDICES_PER_SPRITE;
  }

  return MOSS_RESULT_SUCCESS;
}

MossResult mossEndSpriteBatch (MossSpriteBatch *pSpriteBatch)
{
  if (!pSpriteBatch->isBegun)
  {
    moss__error ("Sprite batch not begun. Call mossBeginSpriteBatch first.\n");
    return MOSS_RESULT_ERROR;
  }

  MossEngine *const pEngine = pSpriteBatch->pOriginalEngine;

  // Copy from staging buffer to device-local buffer with offsets
  VkCommandBuffer command_buffer;
  {
    const MossVk__BeginOneTimeCommandBufferInfo beginInfo = {
      .device      = pEngine->device,
      .commandPool = pEngine->transferCommandPool,
    };
    const MossResult result =
      mossVk__beginOneTimeCommandBuffer (&beginInfo, &command_buffer);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to begin one time command buffer for sprite batch copy.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  // Copy vertex data
  if (pSpriteBatch->vertexDataSize > 0)
  {
    const VkBufferCopy vertex_copy_region = {
      .srcOffset = pSpriteBatch->vertexDataOffset,
      .dstOffset = pSpriteBatch->vertexDataOffset,
      .size      = (VkDeviceSize)pSpriteBatch->vertexDataSize,
    };
    vkCmdCopyBuffer (
      command_buffer,
      pSpriteBatch->stagingBuffer,
      pSpriteBatch->buffer,
      1,
      &vertex_copy_region
    );
  }

  // Copy index data
  if (pSpriteBatch->indexDataSize > 0)
  {
    const VkBufferCopy index_copy_region = {
      .srcOffset = pSpriteBatch->indexDataOffset,
      .dstOffset = pSpriteBatch->indexDataOffset,
      .size      = (VkDeviceSize)pSpriteBatch->indexDataSize,
    };
    vkCmdCopyBuffer (
      command_buffer,
      pSpriteBatch->stagingBuffer,
      pSpriteBatch->buffer,
      1,
      &index_copy_region
    );
  }

  {
    const MossVk__EndOneTimeCommandBufferInfo endInfo = {
      .device        = pEngine->device,
      .commandPool   = pEngine->transferCommandPool,
      .commandBuffer = command_buffer,
      .queue         = pEngine->transferQueue,
    };
    const MossResult result = mossVk__endOneTimeCommandBuffer (&endInfo);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to end one time command buffer for sprite batch copy.\n");
      return result;
    }
  }

  pSpriteBatch->isBegun = false;

  return MOSS_RESULT_SUCCESS;
}

MossResult
mossDrawSpriteBatch (MossEngine *const pEngine, MossSpriteBatch *const pSpriteBatch)
{
  if (pSpriteBatch == NULL || pEngine == NULL)
  {
    moss__error ("Invalid parameters to mossDrawSpriteBatch.\n");
    return MOSS_RESULT_ERROR;
  }

  if (pSpriteBatch->indexCount == 0) { return MOSS_RESULT_SUCCESS; }

  if (pSpriteBatch->isBegun)
  {
    moss__error ("Sprite batch not ended. Call mossEndSpriteBatch first.\n");
    return MOSS_RESULT_ERROR;
  }

  if (pSpriteBatch->pOriginalEngine != pEngine)
  {
    moss__error ("Sprite batch created with different engine.\n");
    return MOSS_RESULT_ERROR;
  }

  // Get the command buffer that's currently being recorded
  // This assumes the command buffer is already in recording state
  const VkCommandBuffer commandBuffer =
    pEngine->generalCommandBuffers[ pEngine->currentFrame ];

  // Bind vertex buffer with offset
  const VkBuffer     vertexBuffers[]       = { pSpriteBatch->buffer };
  const VkDeviceSize vertexBufferOffsets[] = {
    (VkDeviceSize)pSpriteBatch->vertexDataOffset
  };
  vkCmdBindVertexBuffers (commandBuffer, 0, 1, vertexBuffers, vertexBufferOffsets);

  // Bind index buffer with offset
  vkCmdBindIndexBuffer (
    commandBuffer,
    pSpriteBatch->buffer,
    (VkDeviceSize)pSpriteBatch->indexDataOffset,
    VK_INDEX_TYPE_UINT16
  );

  // Draw indexed
  vkCmdDrawIndexed (commandBuffer, pSpriteBatch->indexCount, 1, 0, 0, 0);

  return MOSS_RESULT_SUCCESS;
}

/*=============================================================================
    PRIVATE FUNCTIONS IMPLEMENTATION
  =============================================================================*/

inline static MossResult mossCreateCombinedBuffer (
  const Moss__CreateCombinedBufferInfo *const pInfo,
  VkBuffer                                   *pOutBuffer,
  VkDeviceMemory                             *pOutBufferMemory
)
{
  {
    const MossVk__CreateBufferInfo createInfo = {
      .device = pInfo->pEngine->device,
      .size   = (VkDeviceSize)pInfo->size,
      .usage  = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .sharingMode                 = pInfo->pEngine->bufferSharingMode,
      .sharedQueueFamilyIndexCount = pInfo->pEngine->sharedQueueFamilyIndexCount,
      .sharedQueueFamilyIndices    = pInfo->pEngine->sharedQueueFamilyIndices,
    };

    const MossResult result = mossVk__createBuffer (&createInfo, pOutBuffer);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create combined buffer.\n");
      return result;
    }
  }

  {
    const MossVk__AllocateBufferMemoryInfo allocInfo = {
      .physicalDevice   = pInfo->pEngine->physicalDevice,
      .device           = pInfo->pEngine->device,
      .buffer           = *pOutBuffer,
      .memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    const MossResult result = mossVk__allocateBufferMemory (&allocInfo, pOutBufferMemory);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyBuffer (pInfo->pEngine->device, *pOutBuffer, NULL);
      moss__error ("Failed to allocate combined buffer memory.\n");
      return result;
    }
  }

  return MOSS_RESULT_SUCCESS;
}

inline static void mossGenerateVerticiesFromSprite (
  const MossSprite *const pSprite,
  Moss__Vertex            pOutVertices[ 4 ]
)
{
  const float half_width  = pSprite->size[ 0 ] * 0.5F;
  const float half_height = pSprite->size[ 1 ] * 0.5F;

  const float bbox_left   = pSprite->position[ 0 ] - half_width;
  const float bbox_right  = pSprite->position[ 0 ] + half_width;
  const float bbox_bottom = pSprite->position[ 1 ] - half_height;
  const float bbox_top    = pSprite->position[ 1 ] + half_height;

  pOutVertices[ 0 ] = (Moss__Vertex) {
    .position       = { bbox_left, bbox_top, pSprite->depth },
    .texture_coords = { pSprite->uv.topLeft[ 0 ], pSprite->uv.topLeft[ 1 ] },
  };
  pOutVertices[ 1 ] = (Moss__Vertex) {
    .position       = { bbox_right, bbox_top, pSprite->depth },
    .texture_coords = { pSprite->uv.bottomRight[ 0 ], pSprite->uv.topLeft[ 1 ] },
  };
  pOutVertices[ 2 ] = (Moss__Vertex) {
    .position       = { bbox_right, bbox_bottom, pSprite->depth },
    .texture_coords = { pSprite->uv.bottomRight[ 0 ], pSprite->uv.bottomRight[ 1 ] },
  };
  pOutVertices[ 3 ] = (Moss__Vertex) {
    .position       = { bbox_left, bbox_bottom, pSprite->depth },
    .texture_coords = { pSprite->uv.topLeft[ 0 ], pSprite->uv.bottomRight[ 1 ] },
  };
}
