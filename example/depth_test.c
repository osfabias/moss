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
  @brief Example program to test depth sorting with three sprites.
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
  .app_name    = "Moss Depth Test",
  .app_version = { 0, 1, 0 },
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
    .app_info                    = &moss_app_info,
    .get_window_framebuffer_size = get_window_framebuffer_size,
#ifdef __APPLE__
    .metal_layer = metal_layer,
#endif
  };

  MossEngine *engine;
  if (moss_create_engine (&moss_engine_config, &engine) != MOSS_RESULT_SUCCESS)
  {
    return 1;
  }

  MossCamera *camera;
  moss_get_camera (engine, &camera);

  // Set up camera
  vec2 camera_position = { 0.0F, 0.0F };
  vec2 camera_size     = { 960.0F, 540.0F };

  moss_set_camera_size (camera, camera_size);
  moss_set_camera_position (camera, camera_position);

  // Create three sprites with different depths
  // They will overlap at the center to test depth sorting
  const size_t NUM_SPRITES = 3;
  MossSprite   sprites[ NUM_SPRITES ];

  // Sprite 1: Back (depth 0.0 - furthest back)
  sprites[ 0 ].position[ 0 ] = 0.0F;
  sprites[ 0 ].position[ 1 ] = 0.0F;
  sprites[ 0 ].size[ 0 ]     = 200.0F;
  sprites[ 0 ].size[ 1 ]     = 200.0F;
  sprites[ 0 ].depth         = 0.0F;  // Back
  sprites[ 0 ].uv.top_left[ 0 ]     = 0.0F;
  sprites[ 0 ].uv.top_left[ 1 ]     = 0.0F;
  sprites[ 0 ].uv.bottom_right[ 0 ] = 1.0F;
  sprites[ 0 ].uv.bottom_right[ 1 ] = 1.0F;

  // Sprite 2: Middle (depth 0.5)
  sprites[ 1 ].position[ 0 ] = 50.0F;  // Slightly offset to see overlap
  sprites[ 1 ].position[ 1 ] = 50.0F;
  sprites[ 1 ].size[ 0 ]     = 200.0F;
  sprites[ 1 ].size[ 1 ]     = 200.0F;
  sprites[ 1 ].depth         = 0.5F;  // Middle
  sprites[ 1 ].uv.top_left[ 0 ]     = 0.0F;
  sprites[ 1 ].uv.top_left[ 1 ]     = 0.0F;
  sprites[ 1 ].uv.bottom_right[ 0 ] = 1.0F;
  sprites[ 1 ].uv.bottom_right[ 1 ] = 1.0F;

  // Sprite 3: Front (depth 1.0 - closest to camera)
  sprites[ 2 ].position[ 0 ] = 100.0F;  // More offset
  sprites[ 2 ].position[ 1 ] = 100.0F;
  sprites[ 2 ].size[ 0 ]     = 200.0F;
  sprites[ 2 ].size[ 1 ]     = 200.0F;
  sprites[ 2 ].depth         = 1.0F;  // Front
  sprites[ 2 ].uv.top_left[ 0 ]     = 0.0F;
  sprites[ 2 ].uv.top_left[ 1 ]     = 0.0F;
  sprites[ 2 ].uv.bottom_right[ 0 ] = 1.0F;
  sprites[ 2 ].uv.bottom_right[ 1 ] = 1.0F;

  // Create sprite batch
  const MossSpriteBatchCreateInfo sprite_batch_info = {
    .engine   = engine,
    .capacity = NUM_SPRITES,
  };
  MossSpriteBatch *sprite_batch;
  if (moss_create_sprite_batch (&sprite_batch_info, &sprite_batch) != MOSS_RESULT_SUCCESS)
  {
    moss_destroy_engine (engine);
    return 1;
  }

  moss_begin_sprite_batch (sprite_batch);
  {
    const MossAddSpritesToSpriteBatchInfo add_info = {
      .sprites      = sprites,
      .sprite_count = NUM_SPRITES,
    };
    moss_add_sprites_to_sprite_batch (sprite_batch, &add_info);
  }
  moss_end_sprite_batch (sprite_batch);

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

    moss_begin_frame (engine);
    moss_draw_sprite_batch (engine, sprite_batch);
    moss_end_frame (engine);
  }

  // Cleanup
  moss_destroy_sprite_batch (sprite_batch);
  moss_destroy_engine (engine);
  stuffy_window_close (g_window);
  stuffy_app_deinit ( );

  return EXIT_SUCCESS;
}

