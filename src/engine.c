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
  @brief Graphics engine functions implementation
  @author Ilya Buravov (ilburale@gmail.com)
*/

#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "moss/app_info.h"
#include "moss/engine.h"
#include "moss/result.h"

#include "src/internal/app_info.h"
#include "src/internal/log.h"
#include "src/internal/vk_instance_utils.h"
#include "vulkan/vulkan_core.h"

/*=============================================================================
    ENGINE STATE
  =============================================================================*/

/*
  @brief Engine state.
*/
typedef struct
{
  VkInstance api_instance;
} Moss__Engine;

/*
  @brief Global engine state.
*/
static Moss__Engine g_engine;

/*=============================================================================
    INTERNAL FUNCTION DECLARATIONS
  =============================================================================*/

/*
  @brief Creates Vulkan API instance.
  @param app_info A pointer to a native moss app info struct.
  @return Returns the result code of vkCreateInstance call.
*/
VkResult moss__create_api_instance (const MossAppInfo *app_info);

/*=============================================================================
    PUBLIC API FUNCTIONS IMPLEMENTATION
  =============================================================================*/

/*
  @brief Initializes engine instance.
  @return Returns a pointer to an engine instance on success, otherwise returns NULL.
*/
MossResult moss_engine_init (const MossEngineConfig *const config)
{
  if (moss__create_api_instance (config->app_info) != VK_SUCCESS)
  {
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Destroys engine instance.
  @details Cleans up all reserved memory and destroys all GraphicsAPI objects.kjA
*/
void moss_engine_deinit (void) { vkDestroyInstance (g_engine.api_instance, NULL); }

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

VkResult moss__create_api_instance (const MossAppInfo *const app_info)
{
  const VkApplicationInfo          vk_app_info = moss__create_vk_app_info (app_info);
  const Moss__VkInstanceExtensions extensions =
    moss__get_required_vk_instance_extensions ( );

  // Make instance create info
  const VkInstanceCreateInfo instance_create_info = {
    .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo        = &vk_app_info,
    .ppEnabledExtensionNames = extensions.names,
    .enabledExtensionCount   = extensions.count,
    .enabledLayerCount       = 0,
    .flags                   = moss__get_required_vk_instance_flags ( )
  };

  const VkResult result =
    vkCreateInstance (&instance_create_info, NULL, &g_engine.api_instance);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create Vulkan instance. Error code: %d.\n", result);
  }

  return result;
}
