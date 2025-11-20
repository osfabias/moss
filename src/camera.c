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

void mossGetCamera (MossEngine *const pEngine, MossCamera **pOutCamera)
{
  *pOutCamera = &pEngine->camera;
}

void mossSetCameraPosition (MossCamera *const pCamera, const vec2 newPosition)
{
  pCamera->offset[ 0 ] = -1 * newPosition[ 0 ] * pCamera->scale[ 0 ];
  pCamera->offset[ 1 ] = -1 * newPosition[ 1 ] * pCamera->scale[ 1 ];
}

void mossSetCameraSize (MossCamera *const pCamera, const vec2 newSize)
{
  pCamera->offset[ 0 ] /= pCamera->scale[ 0 ];
  pCamera->offset[ 1 ] /= pCamera->scale[ 1 ];

  pCamera->scale[ 0 ] = 2.0F / newSize[ 0 ];
  pCamera->scale[ 1 ] = -2.0F / newSize[ 1 ];  // Flip Y coordinates
                                               //
  pCamera->offset[ 0 ] *= pCamera->scale[ 0 ];
  pCamera->offset[ 1 ] *= pCamera->scale[ 1 ];
}
