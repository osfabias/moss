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

MossResult moss__createPresentFramebuffers (MossEngine *const pEngine)
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

