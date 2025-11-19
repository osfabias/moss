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

#include <stdbool.h>
#include <stdint.h>

#include <cglm/cglm.h>

#include "moss/app_info.h"
#include "moss/result.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

typedef struct MossEngine MossEngine;

typedef void (*MossGetWindowFramebufferSizeCallback) (uint32_t *width, uint32_t *height);

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

MossResult moss_create_engine (const MossEngineConfig *config, MossEngine **out_engine);

void moss_destroy_engine (MossEngine *engine);

MossResult moss_begin_frame (MossEngine *engine);

MossResult moss_end_frame (MossEngine *engine);

MossResult moss_set_render_resolution (MossEngine *engine, const vec2 new_resolution);
