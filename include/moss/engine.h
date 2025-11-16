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

#include "moss/apidef.h"
#include "moss/app_info.h"
#include "moss/result.h"
#include "moss/window_config.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Moss engine.
*/
typedef struct MossEngine MossEngine;

/*
  @brief Moss engine configuration.
*/
typedef struct
{
  const MossAppInfo      *app_info;      /* Application info. */
  const MossWindowConfig *window_config; /* Window configuration. */
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
  @brief Draws and presents a frame.
  @return On success return MOSS_RESULT_SUCCESS, otherwise returns MOSS_RESULT_ERROR.
*/
__MOSS_API__ MossResult moss_update_engine (MossEngine *engine);
