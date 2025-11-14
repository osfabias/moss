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

  @file example/main.c
  @brief Example program demonstrating library usage
  @author Ilya Buravov (ilburale@gmail.com)
*/

#include "moss/app_info.h"
#include <stddef.h>
#include <stdlib.h>

#include <stuffy/app.h>
#include <stuffy/window.h>

#include <moss/engine.h>
#include <moss/result.h>

static const WindowConf window_config = {
  .rect       = {.x = 128, .y = 128, .width = 640, .height = 360},
  .title      = "Moss Example Application",
  .style_mask = WINDOW_STYLE_TITLED_BIT | WINDOW_STYLE_CLOSABLE_BIT |
                WINDOW_STYLE_RESIZABLE_BIT | WINDOW_STYLE_ICONIFIABLE_BIT,
};

static const MossAppInfo moss_app_info = {
  .app_name    = "Moss Example Application",
  .app_version = {0, 1, 0},
};

static const MossEngineConfig moss_engine_config = {
  .app_info = &moss_app_info,
};

int main (void)
{
  init_app ( );

  Window *const window = open_window (&window_config);
  if (window == NULL) { return EXIT_FAILURE; }

  if (moss_engine_init (&moss_engine_config) != MOSS_RESULT_SUCCESS)
  {
    return EXIT_FAILURE;
  }

  while (!should_window_close (window)) { update_app ( ); }

  moss_engine_deinit ( );
  deinit_app ( );

  return EXIT_SUCCESS;
}
