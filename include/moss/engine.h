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

  @file include/moss/engine.h
  @brief Graphics engine functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "moss/apidef.h"
#include "moss/app_info.h"
#include "moss/result.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Moss engine.
*/
typedef struct MossEngine MossEngine;

/*
  @brief Callback function to get window framebuffer size.
  @details This callback is called whenever the engine needs to know the current
           framebuffer size (e.g., when creating or recreating the swapchain).
  @param width Pointer to store the framebuffer width in pixels.
  @param height Pointer to store the framebuffer height in pixels.
  @note The callback must provide valid width and height values.
*/
typedef void (*MossGetWindowFramebufferSizeCallback) (uint32_t *width, uint32_t *height);

/*
  @brief Moss engine configuration.
*/
typedef struct
{
  const MossAppInfo *app_info; /* Application info. */
  MossGetWindowFramebufferSizeCallback
    get_window_framebuffer_size; /* Callback to get framebuffer size. */
#ifdef __APPLE__
  void *metal_layer; /* Metal layer (CAMetalLayer*). */
#endif
} MossEngineConfig;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Creates engine instance.
  @param config Engine configuration.
  @return On success returns valid pointer to engine state, otherwise returns NULL.
*/
__MOSS_API__ MossEngine *moss_create_engine (const MossEngineConfig *config);

/*
  @brief Destroys engine instance.
  @details Cleans up all reserved memory and destroys all GraphicsAPI objects.
  @param engine Engine handler.
*/
__MOSS_API__ void moss_destroy_engine (MossEngine *engine);

/*
  @brief Begins a new frame.
  @details Acquires the next swap chain image, begins command buffer recording,
           and begins the render pass. After calling this function, you can call
           drawing functions like moss_draw_sprite_batch.
  @param engine Engine handle.
  @return On success return MOSS_RESULT_SUCCESS, otherwise returns MOSS_RESULT_ERROR.
  @note Must be paired with moss_end_frame.
*/
__MOSS_API__ MossResult moss_begin_frame (MossEngine *engine);

/*
  @brief Ends the current frame.
  @details Ends the render pass, ends command buffer recording, submits the command
           buffer to the graphics queue, and presents the swap chain image.
  @param engine Engine handle.
  @return On success return MOSS_RESULT_SUCCESS, otherwise returns MOSS_RESULT_ERROR.
  @note Must be paired with moss_begin_frame.
*/
__MOSS_API__ MossResult moss_end_frame (MossEngine *engine);
