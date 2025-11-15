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

  @file src/internal/shaders.h
  @brief Shader file paths for triangle rendering
  @author Ilya Buravov (ilburale@gmail.com)
  @details This file contains paths to SPIR-V shader files used for rendering.
           Shader source files are located in example/shaders/ directory.
*/

#pragma once

/*
  @brief Path to vertex shader SPIR-V file.
  @details Simple vertex shader that outputs a triangle with positions and colors.
           Shader source: example/shaders/shader.vert
*/
#define MOSS__VERT_SHADER_PATH "shaders/shader.vert.spv"

/*
  @brief Path to fragment shader SPIR-V file.
  @details Simple fragment shader that outputs the interpolated color.
           Shader source: example/shaders/shader.frag
*/
#define MOSS__FRAG_SHADER_PATH "shaders/shader.frag.spv"
