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

  @file example/depth_test.c
  @brief Example program to test depth sorting with three pSprites.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#include "moss/sprite.h"
#include "moss/sprite_batch.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <moss/camera.h>
#include <moss/engine.h>
#include <moss/result.h>
#include <moss/sprite.h>

#include <stuffy/app.h>
#include <stuffy/input/keyboard.h>
#include <stuffy/window.h>

static const MossAppInfo moss_app_info = {
  .appName    = "Moss Depth Test",
  .appVersion = { 0, 1, 0 },
};

static StuffyWindow *g_window = NULL;

static void get_window_framebuffer_size (uint32_t *width, uint32_t *height)
{
  const StuffyExtent2D extent = stuffy_window_get_framebuffer_size (g_window);
  *width                      = extent.width;
  *height                     = extent.height;
}

int main (void)
{
  // Initialize stuffy app
  stuffy_app_init ( );

  // Create window
  const StuffyWindowConfig window_config = {
    .title      = "Moss Depth Test",
    .rect       = { .x = 128, .y = 128, .width = 640, .height = 360 },
    .style_mask = STUFFY_WINDOW_STYLE_TITLED_BIT | STUFFY_WINDOW_STYLE_CLOSABLE_BIT |
                  STUFFY_WINDOW_STYLE_RESIZABLE_BIT | STUFFY_WINDOW_STYLE_ICONIFIABLE_BIT,
  };

  g_window = stuffy_window_open (&window_config);

#ifdef __APPLE__
  // Get metal layer from window
  void *metal_layer = stuffy_window_get_metal_layer (g_window);
#endif

  // Create engine
  const MossEngineConfig moss_engine_config = {
    .appInfo                  = &moss_app_info,
    .getWindowFramebufferSize = get_window_framebuffer_size,
#ifdef __APPLE__
    .metalLayer = metal_layer,
#endif
  };

  MossEngine *pEngine;
  if (mossCreateEngine (&moss_engine_config, &pEngine) != MOSS_RESULT_SUCCESS)
  {
    return 1;
  }

  MossCamera *pCamera;
  mossGetCamera (pEngine, &pCamera);

  // Set up camera
  vec2 camera_position = { 0.0F, 0.0F };
  vec2 camera_size     = { 960.0F, 540.0F };

  mossSetCameraSize (pCamera, camera_size);
  mossSetCameraPosition (pCamera, camera_position);

  // Create three pSprites with different depths
  // They will overlap at the center to test depth sorting
  const size_t NUM_SPRITES = 3;
  MossSprite   pSprites[ NUM_SPRITES ];

  // Sprite 1: Back (depth 0.0 - furthest back)
  pSprites[ 0 ].position[ 0 ]       = 0.0F;
  pSprites[ 0 ].position[ 1 ]       = 0.0F;
  pSprites[ 0 ].size[ 0 ]           = 200.0F;
  pSprites[ 0 ].size[ 1 ]           = 200.0F;
  pSprites[ 0 ].depth               = 0.0F;  // Back
  pSprites[ 0 ].uv.topLeft[ 0 ]     = 0.0F;
  pSprites[ 0 ].uv.topLeft[ 1 ]     = 0.0F;
  pSprites[ 0 ].uv.bottomRight[ 0 ] = 1.0F;
  pSprites[ 0 ].uv.bottomRight[ 1 ] = 1.0F;

  // Sprite 2: Middle (depth 0.5)
  pSprites[ 1 ].position[ 0 ]       = 50.0F;  // Slightly offset to see overlap
  pSprites[ 1 ].position[ 1 ]       = 50.0F;
  pSprites[ 1 ].size[ 0 ]           = 200.0F;
  pSprites[ 1 ].size[ 1 ]           = 200.0F;
  pSprites[ 1 ].depth               = 0.5F;  // Middle
  pSprites[ 1 ].uv.topLeft[ 0 ]     = 0.0F;
  pSprites[ 1 ].uv.topLeft[ 1 ]     = 0.0F;
  pSprites[ 1 ].uv.bottomRight[ 0 ] = 1.0F;
  pSprites[ 1 ].uv.bottomRight[ 1 ] = 1.0F;

  // Sprite 3: Front (depth 1.0 - closest to camera)
  pSprites[ 2 ].position[ 0 ]       = 100.0F;  // More offset
  pSprites[ 2 ].position[ 1 ]       = 100.0F;
  pSprites[ 2 ].size[ 0 ]           = 200.0F;
  pSprites[ 2 ].size[ 1 ]           = 200.0F;
  pSprites[ 2 ].depth               = 1.0F;  // Front
  pSprites[ 2 ].uv.topLeft[ 0 ]     = 0.0F;
  pSprites[ 2 ].uv.topLeft[ 1 ]     = 0.0F;
  pSprites[ 2 ].uv.bottomRight[ 0 ] = 1.0F;
  pSprites[ 2 ].uv.bottomRight[ 1 ] = 1.0F;

  // Create sprite batch
  const MossSpriteBatchCreateInfo sprite_batch_info = {
    .pEngine  = pEngine,
    .capacity = NUM_SPRITES,
  };
  MossSpriteBatch *pSpriteBatch;
  if (mossCreateSpriteBatch (&sprite_batch_info, &pSpriteBatch) != MOSS_RESULT_SUCCESS)
  {
    mossDestroyEngine (pEngine);
    return 1;
  }

  mossBeginSpriteBatch (pSpriteBatch);
  {
    const MossAddSpritesToSpriteBatchInfo add_info = {
      .pSprites    = pSprites,
      .spriteCount = NUM_SPRITES,
    };
    mossAddSpritesToSpriteBatch (pSpriteBatch, &add_info);
  }
  mossEndSpriteBatch (pSpriteBatch);

  printf ("Depth Test Example\n");
  printf ("==================\n");
  printf ("Three sprites with different depths:\n");
  printf ("  Sprite 1: depth = 0.0 (back, at 0, 0)\n");
  printf ("  Sprite 2: depth = 0.5 (middle, at 50, 50)\n");
  printf ("  Sprite 3: depth = 1.0 (front, at 100, 100)\n");
  printf ("\nIf depth sorting works correctly, sprite 3 (front) should be on top.\n");
  printf ("Press ESC to exit.\n\n");

  // Main loop
  while (!stuffy_window_should_close (g_window))
  {
    stuffy_app_update ( );

    const StuffyKeyboardState *keyboard = stuffy_keyboard_get_state ( );

    // Escape to exit
    if (keyboard->keys[ STUFFY_KEY_ESCAPE ]) { break; }

    mossBeginFrame (pEngine);
    mossDrawSpriteBatch (pEngine, pSpriteBatch);
    mossEndFrame (pEngine);
  }

  // Cleanup
  mossDestroySpriteBatch (pSpriteBatch);
  mossDestroyEngine (pEngine);
  stuffy_window_close (g_window);
  stuffy_app_deinit ( );

  return EXIT_SUCCESS;
}
