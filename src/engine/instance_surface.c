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
#ifdef __APPLE__
#  include <vulkan/vulkan_metal.h>
#endif

#include "moss/app_info.h"
#include "moss/result.h"

#include "src/internal/app_info.h"
#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/vulkan/utils/instance.h"
#include "src/internal/vulkan/utils/validation_layers.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

MossResult
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

MossResult moss__createSurface (MossEngine *const pEngine)
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

