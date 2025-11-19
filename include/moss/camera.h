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

  @file src/camera.c
  @brief Camera struct and function declarations.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <cglm/vec2.h>

#include "moss/engine.h"

typedef struct MossCamera MossCamera;

/*
  @brief Returns camera handle.
  @param engine Engine handle.
  @return Always returns a valid pointer to the engine's camera.
*/
MossCamera *moss_get_camera (MossEngine *engine);

/*
  @brief Sets camera position.
  @param camera Camera handle.
  @param new_position New position to move camera to.
*/
void moss_set_camera_position (MossCamera *camera, const vec2 new_position);

/*
  @brief Sets camera size.
  @param camera Camera handle.
  @param new_size New size to resize camera to.
*/
void moss_set_camera_size (MossCamera *camera, const vec2 new_size);
