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

  @file src/internal/vulkan/utils/shader.h
  @brief Vulkan shader utility functions
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/log.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Required info to read shader file.
*/
typedef struct
{
  const char *file_path; /* Path to the SPIR-V file. */
  uint32_t  **out_code;  /* Pointer to store allocated code buffer (must be freed by
                             caller). */
  size_t *out_code_size; /* Pointer to store code size in bytes. */
} Moss__ReadShaderFileInfo;

/*
  @brief Required info to create shader module.
*/
typedef struct
{
  VkDevice        device;            /* Logical device. */
  const uint32_t *code;              /* Pointer to SPIR-V code. */
  size_t          code_size;         /* Size of SPIR-V code in bytes. */
  VkShaderModule *out_shader_module; /* Pointer to store created shader module. */
} Moss__CreateShaderModuleInfo;

/*
  @brief Required info to create shader module from file.
*/
typedef struct
{
  VkDevice        device;            /* Logical device. */
  const char     *file_path;         /* Path to the SPIR-V file. */
  VkShaderModule *out_shader_module; /* Pointer to store created shader module. */
} Moss__CreateShaderModuleFromFileInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Reads SPIR-V code from a file.
  @param info Required info to read shader file.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static MossResult
moss_vk__read_shader_file (const Moss__ReadShaderFileInfo *const info)
{
  FILE *const file = fopen (info->file_path, "rb");
  if (file == NULL)
  {
    moss__error ("Failed to open shader file: %s\n", info->file_path);
    return MOSS_RESULT_ERROR;
  }

  fseek (file, 0, SEEK_END);
  const long file_size = ftell (file);
  fseek (file, 0, SEEK_SET);

  if (file_size < 0 || (file_size % 4) != 0)
  {
    moss__error ("Invalid shader file size: %s\n", info->file_path);
    fclose (file);
    return MOSS_RESULT_ERROR;
  }

  const size_t code_size = (size_t)file_size;
  uint32_t    *code      = malloc (code_size);
  if (code == NULL)
  {
    moss__error ("Failed to allocate memory for shader code: %s\n", info->file_path);
    fclose (file);
    return MOSS_RESULT_ERROR;
  }

  const size_t read_size = fread (code, 1, code_size, file);
  fclose (file);

  if (read_size != code_size)
  {
    moss__error ("Failed to read shader file: %s\n", info->file_path);
    free (code);
    return MOSS_RESULT_ERROR;
  }

  *info->out_code      = code;
  *info->out_code_size = code_size;

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Creates a shader module from SPIR-V code.
  @param info Required info to create shader module.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static MossResult
moss_vk__create_shader_module (const Moss__CreateShaderModuleInfo *const info)
{
  const VkShaderModuleCreateInfo create_info = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext    = NULL,
    .flags    = 0,
    .codeSize = info->code_size,
    .pCode    = info->code,
  };

  const VkResult result =
    vkCreateShaderModule (info->device, &create_info, NULL, info->out_shader_module);
  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create shader module. Error code: %d.\n", result);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Creates a shader module from a SPIR-V file.
  @param info Required info to create shader module from file.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static MossResult moss_vk__create_shader_module_from_file (
  const Moss__CreateShaderModuleFromFileInfo *const info
)
{
  uint32_t *code      = NULL;
  size_t    code_size = 0;

  {  // Read shader file code
    const Moss__ReadShaderFileInfo read_info = {
      .file_path     = info->file_path,
      .out_code      = &code,
      .out_code_size = &code_size,
    };
    const MossResult result = moss_vk__read_shader_file (&read_info);
    if (result != MOSS_RESULT_SUCCESS) { return result; }
  }

  {  // Create shader module
    const Moss__CreateShaderModuleInfo create_info = {
      .device            = info->device,
      .code              = code,
      .code_size         = code_size,
      .out_shader_module = info->out_shader_module,
    };
    const MossResult result = moss_vk__create_shader_module (&create_info);
    if (result != MOSS_RESULT_SUCCESS) { return result; }
  }

  free (code);

  return MOSS_RESULT_SUCCESS;
}
