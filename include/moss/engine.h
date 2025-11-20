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
  const MossAppInfo *appInfo; /* Application info. */
  MossGetWindowFramebufferSizeCallback
    getWindowFramebufferSize; /* Callback to get framebuffer size. */
#ifdef __APPLE__
  void *metalLayer; /* Metal layer (CAMetalLayer*). */
#endif
} MossEngineConfig;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

MossResult mossCreateEngine (const MossEngineConfig *pConfig, MossEngine **pOutEngine);

void mossDestroyEngine (MossEngine *pEngine);

MossResult mossBeginFrame (MossEngine *pEngine);

MossResult mossEndFrame (MossEngine *pEngine);

MossResult mossSetRenderResolution (MossEngine *pEngine, const vec2 newResolution);
