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

  @file src/internal/config.h
  @brief Static engine parameters configuration.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

/* Max frames in flight. */
#define MAX_FRAMES_IN_FLIGHT (size_t)(2)

/* Max image count in swapchain. */
#define MAX_SWAPCHAIN_IMAGE_COUNT (size_t)(4)
