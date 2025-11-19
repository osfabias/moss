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

#include "moss/sprite.h"
#include "moss/sprite_batch.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __APPLE__
#  include <mach/mach.h>
#  include <mach/mach_time.h>
#endif

#include <moss/camera.h>
#include <moss/engine.h>
#include <moss/result.h>
#include <moss/sprite.h>

#include <stuffy/app.h>
#include <stuffy/input/keyboard.h>
#include <stuffy/input/mouse.h>
#include <stuffy/window.h>

static const MossAppInfo moss_app_info = {
  .app_name    = "Moss Example Application",
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
    .title      = "Moss Example Application",
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

  MossEngine *const engine = moss_create_engine (&moss_engine_config);

  MossCamera *const camera = moss_get_camera (engine);

  // Track camera state locally (since there are no getter functions)
  vec2 camera_position = { 0.0F, 0.0F };
  vec2 camera_size     = { 640.0F, 360.0F };

  moss_set_camera_size (camera, camera_size);
  moss_set_camera_position (camera, camera_position);

  // Create 10000 sprites with random positions, sizes, and velocities
  const size_t NUM_SPRITES = 1000000;
  MossSprite  *sprites     = malloc (sizeof (MossSprite) * NUM_SPRITES);

  // Per-sprite velocity and size change data
  typedef struct
  {
    vec2  velocity;      /* Position velocity. */
    vec2  size_velocity; /* Size change velocity. */
    float min_size;      /* Minimum size. */
    float max_size;      /* Maximum size. */
  } SpriteData;

  SpriteData *sprite_data = malloc (sizeof (SpriteData) * NUM_SPRITES);

  // Seed random number generator
  srand ((unsigned int)time (NULL));

  // Initialize sprites with random positions, sizes, and velocities
  for (size_t i = 0; i < NUM_SPRITES; ++i)
  {
    // Random position within a larger area
    sprites[ i ].position[ 0 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 2000.0F;
    sprites[ i ].position[ 1 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 2000.0F;

    // Random initial size between 20 and 80
    const float initial_size = 20.0F + ((float)rand ( ) / RAND_MAX) * 60.0F;
    sprites[ i ].size[ 0 ]   = initial_size;
    sprites[ i ].size[ 1 ]   = initial_size;

    // Random depth for sorting
    sprites[ i ].depth = ((float)rand ( ) / RAND_MAX) * 1.0F;

    // UV coordinates (full texture)
    sprites[ i ].uv.top_left[ 0 ]     = 0.0F;
    sprites[ i ].uv.top_left[ 1 ]     = 0.0F;
    sprites[ i ].uv.bottom_right[ 0 ] = 1.0F;
    sprites[ i ].uv.bottom_right[ 1 ] = 1.0F;

    // Random velocity between -100 and 100 per second
    sprite_data[ i ].velocity[ 0 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 200.0F;
    sprite_data[ i ].velocity[ 1 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 200.0F;

    // Random size change velocity between -30 and 30 per second
    sprite_data[ i ].size_velocity[ 0 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 60.0F;
    sprite_data[ i ].size_velocity[ 1 ] =
      sprite_data[ i ].size_velocity[ 0 ];  // Keep square

    // Size bounds
    sprite_data[ i ].min_size = 10.0F;
    sprite_data[ i ].max_size = 100.0F;
  }

  // Create sprite batch
  const MossSpriteBatchCreateInfo sprite_batch_info = {
    .engine   = engine,
    .capacity = NUM_SPRITES,
  };
  MossSpriteBatch *const sprite_batch = moss_create_sprite_batch (&sprite_batch_info);

  // Initialize timing
#ifdef __APPLE__
  mach_timebase_info_data_t timebase_info;
  mach_timebase_info (&timebase_info);
  uint64_t start_time      = mach_absolute_time ( );
  uint64_t last_frame_time = start_time;
#else
  struct timespec start_time;
  struct timespec last_frame_time;
  clock_gettime (CLOCK_MONOTONIC, &start_time);
  last_frame_time = start_time;
#endif

  // Frame statistics
  uint64_t frame_count      = 0;
  double   total_frame_time = 0.0;
  double   max_frame_time   = 0.0;        // Worst (biggest) delta time
  double   min_fps          = 1000000.0;  // Worst (lowest) FPS

  // Main loop
  const float base_camera_speed =
    50.0F;  // Base camera movement speed per second (at initial zoom)
  const float zoom_speed            = 10.0F;    // Zoom speed per second
  const float min_zoom              = 1.0F;     // Minimum camera size (zoomed in)
  const float max_zoom              = 2000.0F;  // Maximum camera size (zoomed out)
  const float initial_camera_size_x = camera_size[ 0 ];
  const float initial_camera_size_y = camera_size[ 1 ];

  while (!stuffy_window_should_close (g_window))
  {
    // Calculate delta time at start of frame
#ifdef __APPLE__
    uint64_t current_time = mach_absolute_time ( );
    uint64_t frame_time_nanos =
      (current_time - last_frame_time) * timebase_info.numer / timebase_info.denom;
    double frame_time_seconds = (double)frame_time_nanos / 1000000000.0;
    last_frame_time           = current_time;
#else
    struct timespec current_time;
    clock_gettime (CLOCK_MONOTONIC, &current_time);
    double frame_time_seconds =
      (double)(current_time.tv_sec - last_frame_time.tv_sec) +
      (double)(current_time.tv_nsec - last_frame_time.tv_nsec) / 1000000000.0;
    last_frame_time = current_time;
#endif

    // Clamp delta time to prevent large spikes (e.g., during window resizing)
    // Store original before clamping for statistics
    const double original_frame_time = frame_time_seconds;
    if (frame_time_seconds > 0.1) { frame_time_seconds = 0.1; }

    // Update statistics
    frame_count++;
    total_frame_time += frame_time_seconds;

    // Track worst frame time (before clamping)
    if (original_frame_time > max_frame_time) { max_frame_time = original_frame_time; }

    // Track worst FPS (after clamping to prevent division by zero from huge spikes)
    const double frame_fps = 1.0 / frame_time_seconds;
    if (frame_fps < min_fps) { min_fps = frame_fps; }

    stuffy_app_update ( );

    // Handle camera controls
    const StuffyKeyboardState *keyboard = stuffy_keyboard_get_state ( );
    const StuffyMouseState    *mouse    = stuffy_mouse_get_state ( );

    // Calculate camera speed based on zoom level (proportional to camera size)
    // When zoomed out (larger size), move faster; when zoomed in (smaller size), move
    // slower
    const float zoom_factor          = camera_size[ 1 ] / initial_camera_size_y;
    const float current_camera_speed = base_camera_speed * zoom_factor;

    // Use actual delta time for movement
    const float delta_time    = (float)frame_time_seconds;
    const float move_distance = current_camera_speed * delta_time;
    const float zoom_distance = zoom_speed * delta_time;

    // WASD controls (or arrow keys as alternative)
    if (keyboard->keys[ STUFFY_KEY_W ] || keyboard->keys[ STUFFY_KEY_UP ])
    {
      camera_position[ 1 ] += move_distance;
    }
    if (keyboard->keys[ STUFFY_KEY_S ] || keyboard->keys[ STUFFY_KEY_DOWN ])
    {
      camera_position[ 1 ] -= move_distance;
    }
    if (keyboard->keys[ STUFFY_KEY_A ] || keyboard->keys[ STUFFY_KEY_LEFT ])
    {
      camera_position[ 0 ] -= move_distance;
    }
    if (keyboard->keys[ STUFFY_KEY_D ] || keyboard->keys[ STUFFY_KEY_RIGHT ])
    {
      camera_position[ 0 ] += move_distance;
    }

    moss_set_camera_position (camera, camera_position);

    // Zoom controls
    float zoom_delta = 0.0F;

    // Mouse wheel zoom
    if (mouse->scroll != 0.0F)
    {
      // Negative scroll = zoom in (smaller size), positive = zoom out (larger size)
      zoom_delta = -mouse->scroll * zoom_distance * 5.0F;
    }

    // Keyboard zoom (+/- keys or Page Up/Down)
    if (keyboard->keys[ STUFFY_KEY_EQUAL ] || keyboard->keys[ STUFFY_KEY_ADD ] ||
        keyboard->keys[ STUFFY_KEY_PAGEUP ])
    {
      // Zoom out (increase camera size)
      zoom_delta += zoom_distance;
    }
    if (keyboard->keys[ STUFFY_KEY_MINUS ] || keyboard->keys[ STUFFY_KEY_SUBTRACT ] ||
        keyboard->keys[ STUFFY_KEY_PAGEDOWN ])
    {
      // Zoom in (decrease camera size)
      zoom_delta -= zoom_distance;
    }

    // Apply zoom with limits, maintaining aspect ratio
    if (zoom_delta != 0.0F)
    {
      const float aspect_ratio = initial_camera_size_x / initial_camera_size_y;
      float       new_size_x   = camera_size[ 0 ] + zoom_delta * aspect_ratio;
      float       new_size_y   = camera_size[ 1 ] + zoom_delta;

      // Clamp to min/max zoom limits
      const float min_size_x = min_zoom * aspect_ratio;
      const float max_size_x = max_zoom * aspect_ratio;

      if (new_size_x < min_size_x)
      {
        new_size_x = min_size_x;
        new_size_y = min_zoom;
      }
      else if (new_size_x > max_size_x)
      {
        new_size_x = max_size_x;
        new_size_y = max_zoom;
      }

      camera_size[ 0 ] = new_size_x;
      camera_size[ 1 ] = new_size_y;
      moss_set_camera_size (camera, camera_size);
    }

    // Escape to exit
    if (keyboard->keys[ STUFFY_KEY_ESCAPE ]) { break; }

    // Update sprite positions and sizes every frame
    for (size_t i = 0; i < NUM_SPRITES; ++i)
    {
      // Update position
      sprites[ i ].position[ 0 ] += sprite_data[ i ].velocity[ 0 ] * delta_time;
      sprites[ i ].position[ 1 ] += sprite_data[ i ].velocity[ 1 ] * delta_time;

      // Wrap around if sprites go too far
      const float bounds = 1200.0F;
      if (sprites[ i ].position[ 0 ] > bounds) { sprites[ i ].position[ 0 ] = -bounds; }
      else if (sprites[ i ].position[ 0 ] < -bounds)
      {
        sprites[ i ].position[ 0 ] = bounds;
      }
      if (sprites[ i ].position[ 1 ] > bounds) { sprites[ i ].position[ 1 ] = -bounds; }
      else if (sprites[ i ].position[ 1 ] < -bounds)
      {
        sprites[ i ].position[ 1 ] = bounds;
      }

      // Update size
      sprites[ i ].size[ 0 ] += sprite_data[ i ].size_velocity[ 0 ] * delta_time;
      sprites[ i ].size[ 1 ] += sprite_data[ i ].size_velocity[ 1 ] * delta_time;

      // Clamp size and reverse velocity if bounds reached
      if (sprites[ i ].size[ 0 ] <= sprite_data[ i ].min_size ||
          sprites[ i ].size[ 0 ] >= sprite_data[ i ].max_size)
      {
        sprite_data[ i ].size_velocity[ 0 ] = -sprite_data[ i ].size_velocity[ 0 ];
        sprite_data[ i ].size_velocity[ 1 ] = sprite_data[ i ].size_velocity[ 0 ];
      }

      if (sprites[ i ].size[ 0 ] < sprite_data[ i ].min_size)
      {
        sprites[ i ].size[ 0 ] = sprite_data[ i ].min_size;
        sprites[ i ].size[ 1 ] = sprite_data[ i ].min_size;
      }
      else if (sprites[ i ].size[ 0 ] > sprite_data[ i ].max_size)
      {
        sprites[ i ].size[ 0 ] = sprite_data[ i ].max_size;
        sprites[ i ].size[ 1 ] = sprite_data[ i ].max_size;
      }
    }

    // Rebuild sprite batch with updated positions and sizes
    moss_clear_sprite_batch (sprite_batch);
    moss_begin_sprite_batch (sprite_batch);
    {
      const MossAddSpritesToSpriteBatchInfo add_info = {
        .sprites      = sprites,
        .sprite_count = NUM_SPRITES,
      };
      moss_add_sprites_to_sprite_batch (sprite_batch, &add_info);
    }
    moss_end_sprite_batch (sprite_batch);

    moss_begin_frame (engine);
    moss_draw_sprite_batch (engine, sprite_batch);
    moss_end_frame (engine);
  }

  // Calculate and print frame statistics
  if (frame_count > 0)
  {
    double average_frame_time = total_frame_time / (double)frame_count;
    double average_fps        = 1.0 / average_frame_time;
    double total_time         = total_frame_time;

    printf ("\n===== Frame Statistics =====\n");
    printf ("Total frames: %llu\n", (unsigned long long)frame_count);
    printf ("Total time: %.3f seconds\n", total_time);
    printf ("Average frame time: %.3f ms\n", average_frame_time * 1000.0);
    printf ("Worst frame time: %.3f ms\n", max_frame_time * 1000.0);
    printf ("Average FPS: %.2f\n", average_fps);
    printf ("Lowest FPS: %.2f\n", min_fps);
    printf ("============================\n\n");
  }

  // Cleanup
  free (sprite_data);
  free (sprites);
  moss_destroy_sprite_batch (sprite_batch);
  moss_destroy_engine (engine);
  stuffy_window_close (g_window);
  stuffy_app_deinit ( );

  return EXIT_SUCCESS;
}
