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

#include <cglm/vec2.h>

typedef struct
{
  float depth;    /* Image depth. */
  vec2  position; /* World position. */
  vec2  size;     /* World units size. */
  struct
  {
    vec2 topLeft;     /* UV coords of the top left corner on texture altas. */
    vec2 bottomRight; /* UV coords of the bottom right on texture atlas. */
  } uv;
} MossSprite;
