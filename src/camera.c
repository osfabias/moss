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
  @brief Camera functions implementation.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#include <cglm/vec2.h>

#include "moss/camera.h"
#include "moss/engine.h"

#include "src/internal/camera.h"
#include "src/internal/engine.h"

void moss_get_camera (MossEngine *const engine, MossCamera **out_camera)
{
  *out_camera = &engine->camera;
}

void moss_set_camera_position (MossCamera *const camera, const vec2 new_position)
{
  camera->offset[ 0 ] = -1 * new_position[ 0 ] * camera->scale[ 0 ];
  camera->offset[ 1 ] = -1 * new_position[ 1 ] * camera->scale[ 1 ];
}

void moss_set_camera_size (MossCamera *const camera, const vec2 new_size)
{
  camera->offset[ 0 ] /= camera->scale[ 0 ];
  camera->offset[ 1 ] /= camera->scale[ 1 ];

  camera->scale[ 0 ] = 2.0F / new_size[ 0 ];
  camera->scale[ 1 ] = -2.0F / new_size[ 1 ];  // Flip Y coordinates
                                               //
  camera->offset[ 0 ] *= camera->scale[ 0 ];
  camera->offset[ 1 ] *= camera->scale[ 1 ];
}
