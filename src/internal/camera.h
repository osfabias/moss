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

  @file src/internal/camera.h
  @brief Private camera struct body declaration.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <cglm/cglm.h>

struct MossCamera
{
  vec2 scale;  /* Camera scale applied to verticies. */
  vec2 offset; /* Camera offset applied to verticies. */
};
