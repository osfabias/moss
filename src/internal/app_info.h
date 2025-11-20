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

  @file src/internal/app_info.h
  @brief Internal app info convert function
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include <vulkan/vulkan.h>

#include "moss/app_info.h"

/*
  @brief Creates VkApplicationInfo from @ref MossAppInfo.
  @param app_info A pointer to a native moss app info struct.
  @param out_vk_app_info Output parameter for VkApplicationInfo struct.
*/
inline static void
moss__createVkAppInfo (const MossAppInfo *pAppInfo, VkApplicationInfo *pOutVkAppInfo)
{
  const uint32_t appVersion = VK_MAKE_VERSION (
    pAppInfo->appVersion.major,
    pAppInfo->appVersion.minor,
    pAppInfo->appVersion.patch
  );

  const uint32_t engineVersion =
    VK_MAKE_VERSION (MOSS_VERSION_MAJOR, MOSS_VERSION_MINOR, MOSS_VERSION_PATCH);
  const char *const pEngineName = "Moss Engine";

  *pOutVkAppInfo = (VkApplicationInfo) {
    .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName   = pAppInfo->appName,
    .applicationVersion = appVersion,
    .pEngineName        = pEngineName,
    .engineVersion      = engineVersion,
    .apiVersion         = VK_API_VERSION_1_0,
  };
}
