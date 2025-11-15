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

  @file src/internal/vk_shader_utils.h
  @brief Vulkan shader utility functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "src/internal/log.h"

/*
  @brief Reads SPIR-V code from a file.
  @param file_path Path to the SPIR-V file.
  @param out_code Pointer to store allocated code buffer (must be freed by caller).
  @param out_code_size Pointer to store code size in bytes.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static VkResult moss__read_shader_file (
  const char *file_path, uint32_t **out_code, size_t *out_code_size
)
{
  FILE *file = fopen (file_path, "rb");
  if (file == NULL)
  {
    moss__error ("Failed to open shader file: %s\n", file_path);
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  fseek (file, 0, SEEK_END);
  const long file_size = ftell (file);
  fseek (file, 0, SEEK_SET);

  if (file_size < 0 || (file_size % 4) != 0)
  {
    moss__error ("Invalid shader file size: %s\n", file_path);
    fclose (file);
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  const size_t code_size = (size_t)file_size;
  uint32_t    *code      = malloc (code_size);
  if (code == NULL)
  {
    moss__error ("Failed to allocate memory for shader code: %s\n", file_path);
    fclose (file);
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  const size_t read_size = fread (code, 1, code_size, file);
  fclose (file);

  if (read_size != code_size)
  {
    moss__error ("Failed to read shader file: %s\n", file_path);
    free (code);
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  *out_code      = code;
  *out_code_size = code_size;

  return VK_SUCCESS;
}

/*
  @brief Creates a shader module from SPIR-V code.
  @param device Logical device.
  @param code Pointer to SPIR-V code.
  @param code_size Size of SPIR-V code in bytes.
  @param out_shader_module Pointer to store created shader module.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static VkResult moss__create_shader_module (
  VkDevice device, const uint32_t *code, size_t code_size, VkShaderModule *out_shader_module
)
{
  const VkShaderModuleCreateInfo create_info = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code_size,
    .pCode    = code,
  };

  const VkResult result = vkCreateShaderModule (device, &create_info, NULL, out_shader_module);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create shader module. Error code: %d.\n", result);
  }

  return result;
}

/*
  @brief Creates a shader module from a SPIR-V file.
  @param device Logical device.
  @param file_path Path to the SPIR-V file.
  @param out_shader_module Pointer to store created shader module.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static VkResult moss__create_shader_module_from_file (
  VkDevice device, const char *file_path, VkShaderModule *out_shader_module
)
{
  uint32_t *code      = NULL;
  size_t    code_size = 0;

  VkResult result = moss__read_shader_file (file_path, &code, &code_size);
  if (result != VK_SUCCESS) { return result; }

  result = moss__create_shader_module (device, code, code_size, out_shader_module);

  free (code);

  return result;
}

