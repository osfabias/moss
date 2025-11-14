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

#include "moss/apidef.h"
#include "moss/app_info.h"
#include "moss/result.h"

/*
  @brief Moss engine configuration.
*/
typedef struct
{
  const MossAppInfo *app_info; /* Application info. */
} MossEngineConfig;

/*
  @brief Initializes graphics engine.
  @return Returns a pointer to an engine instance on success, otherwise returns NULL.
*/
__MOSS_API__ MossResult moss_engine_init (const MossEngineConfig *config);

/*
  @brief Deinitializes engine instance.
  @details Cleans up all reserved memory and destroys all GraphicsAPI objects.
*/
__MOSS_API__ void moss_engine_deinit (void);
