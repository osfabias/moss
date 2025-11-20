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
  const char *filePath;  /* Path to the SPIR-V file. */
  uint32_t  **outCode;   /* Pointer to store allocated code buffer (must be freed by
                            caller). */
  size_t *outCodeSize;   /* Pointer to store code size in bytes. */
} MossVk__ReadShaderFileInfo;

/*
  @brief Required info to create shader module.
*/
typedef struct
{
  VkDevice        device;          /* Logical device. */
  const uint32_t *code;            /* Pointer to SPIR-V code. */
  size_t          codeSize;        /* Size of SPIR-V code in bytes. */
  VkShaderModule *outShaderModule; /* Pointer to store created shader module. */
} MossVk__CreateShaderModuleInfo;

/*
  @brief Required info to create shader module from file.
*/
typedef struct
{
  VkDevice        device;          /* Logical device. */
  const char     *filePath;        /* Path to the SPIR-V file. */
  VkShaderModule *outShaderModule; /* Pointer to store created shader module. */
} MossVk__CreateShaderModuleFromFileInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Reads SPIR-V code from a file.
  @param info Required info to read shader file.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static MossResult
mossVk__readShaderFile (const MossVk__ReadShaderFileInfo *const info)
{
  FILE *const file = fopen (info->filePath, "rb");
  if (file == NULL)
  {
    moss__error ("Failed to open shader file: %s\n", info->filePath);
    return MOSS_RESULT_ERROR;
  }

  fseek (file, 0, SEEK_END);
  const long fileSize = ftell (file);
  fseek (file, 0, SEEK_SET);

  if (fileSize < 0 || (fileSize % 4) != 0)
  {
    moss__error ("Invalid shader file size: %s\n", info->filePath);
    fclose (file);
    return MOSS_RESULT_ERROR;
  }

  const size_t codeSize = (size_t)fileSize;
  uint32_t    *code     = malloc (codeSize);
  if (code == NULL)
  {
    moss__error ("Failed to allocate memory for shader code: %s\n", info->filePath);
    fclose (file);
    return MOSS_RESULT_ERROR;
  }

  const size_t readSize = fread (code, 1, codeSize, file);
  fclose (file);

  if (readSize != codeSize)
  {
    moss__error ("Failed to read shader file: %s\n", info->filePath);
    free (code);
    return MOSS_RESULT_ERROR;
  }

  *info->outCode     = code;
  *info->outCodeSize = codeSize;

  return MOSS_RESULT_SUCCESS;
}

/*
  @brief Creates a shader module from SPIR-V code.
  @param info Required info to create shader module.
  @return VK_SUCCESS on success, error code otherwise.
*/
inline static MossResult
mossVk__createShaderModule (const MossVk__CreateShaderModuleInfo *const info)
{
  const VkShaderModuleCreateInfo createInfo = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext    = NULL,
    .flags    = 0,
    .codeSize = info->codeSize,
    .pCode    = info->code,
  };

  const VkResult result =
    vkCreateShaderModule (info->device, &createInfo, NULL, info->outShaderModule);
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
inline static MossResult mossVk__createShaderModuleFromFile (
  const MossVk__CreateShaderModuleFromFileInfo *const info
)
{
  uint32_t *code     = NULL;
  size_t    codeSize = 0;

  {  // Read shader file code
    const MossVk__ReadShaderFileInfo readInfo = {
      .filePath   = info->filePath,
      .outCode    = &code,
      .outCodeSize = &codeSize,
    };
    const MossResult result = mossVk__readShaderFile (&readInfo);
    if (result != MOSS_RESULT_SUCCESS) { return result; }
  }

  {  // Create shader module
    const MossVk__CreateShaderModuleInfo createInfo = {
      .device         = info->device,
      .code           = code,
      .codeSize       = codeSize,
      .outShaderModule = info->outShaderModule,
    };
    const MossResult result = mossVk__createShaderModule (&createInfo);
    if (result != MOSS_RESULT_SUCCESS) { return result; }
  }

  free (code);

  return MOSS_RESULT_SUCCESS;
}
