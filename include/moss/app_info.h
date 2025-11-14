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

  @file include/moss/app_info.h
  @brief App info struct declaration.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "moss/version.h"

/*
  @brief App info.
  @details Used to provide info to the graphics API driver in order to apply specific
           optimization to your app. But, let's be honest, nobody will support it :3
*/
typedef struct
{
  const char *app_name;    /* App name. */
  MossVersion app_version; /* App version. */
} MossAppInfo;
