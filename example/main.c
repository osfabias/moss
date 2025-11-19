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

  MossEngine *engine;
  if (moss_create_engine (&moss_engine_config, &engine) != MOSS_RESULT_SUCCESS)
  {
    return 1;
  }

  MossCamera *camera;
  moss_get_camera (engine, &camera);

  // Track camera state locally (since there are no getter functions)
  vec2 camera_position = { 0.0F, 0.0F };
  vec2 camera_size     = { 960.0F, 540.0F };

  moss_set_camera_size (camera, camera_size);
  moss_set_camera_position (camera, camera_position);

  // Create 1000000 static sprites with random positions and sizes
  const size_t NUM_SPRITES = 700000;
  MossSprite  *sprites     = malloc (sizeof (MossSprite) * NUM_SPRITES);

  // Seed random number generator
  srand ((unsigned int)time (NULL));

  // Initialize sprites with random positions and sizes (static, no animation)
  for (size_t i = 0; i < NUM_SPRITES; ++i)
  {
    // Random position within a larger area
    sprites[ i ].position[ 0 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 2000.0F;
    sprites[ i ].position[ 1 ] = ((float)rand ( ) / RAND_MAX - 0.5F) * 2000.0F;

    // Random size between 8 and 32
    const float size       = 8.0F + ((float)rand ( ) / RAND_MAX) * 24.0F;
    sprites[ i ].size[ 0 ] = size;
    sprites[ i ].size[ 1 ] = size;

    // Random depth for sorting
    sprites[ i ].depth = ((float)rand ( ) / RAND_MAX) * 1.0F;

    // UV coordinates (full texture)
    sprites[ i ].uv.top_left[ 0 ]     = 0.5F;
    sprites[ i ].uv.top_left[ 1 ]     = 0.0F;
    sprites[ i ].uv.bottom_right[ 0 ] = 1.0F;
    sprites[ i ].uv.bottom_right[ 1 ] = 1.0F;
  }

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

  // Initialize timing
#ifdef __APPLE__
  mach_timebase_info_data_t timebase_info;
  mach_timebase_info (&timebase_info);
  uint64_t start_time                    = mach_absolute_time ( );
  uint64_t last_frame_time               = start_time;
  uint64_t last_fps_update_absolute_time = start_time;
#else
  struct timespec start_time;
  struct timespec last_frame_time;
  struct timespec last_fps_update_time;
  clock_gettime (CLOCK_MONOTONIC, &start_time);
  last_frame_time      = start_time;
  last_fps_update_time = start_time;
#endif

  // Frame statistics
  uint64_t frame_count      = 0;
  double   total_frame_time = 0.0;
  double   max_frame_time   = 0.0;        // Worst (biggest) delta time
  double   min_fps          = 1000000.0;  // Worst (lowest) FPS
  double   total_draw_time  = 0.0;        // Total time spent on drawing/rendering
  double   max_draw_time    = 0.0;        // Worst (biggest) draw time

  // FPS averaging over 0.5 second window
  double       accumulated_frame_time  = 0.0;  // Accumulated frame time for averaging
  double       accumulated_draw_time   = 0.0;  // Accumulated draw time for averaging
  uint64_t     accumulated_frame_count = 0;    // Number of frames in current window
  double       current_avg_frame_time  = 0.0;  // Current average frame time
  double       current_avg_draw_time   = 0.0;  // Current average draw time
  double       current_fps             = 0.0;  // Current FPS based on average
  const double fps_update_interval     = 0.5;  // Update FPS display every 0.5 seconds

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

    // Measure draw time (rendering)
#ifdef __APPLE__
    uint64_t draw_start_time = mach_absolute_time ( );
#else
    struct timespec draw_start_time;
    clock_gettime (CLOCK_MONOTONIC, &draw_start_time);
#endif

    moss_begin_frame (engine);
    moss_draw_sprite_batch (engine, sprite_batch);
    moss_end_frame (engine);

    // Measure draw time end
#ifdef __APPLE__
    uint64_t draw_end_time = mach_absolute_time ( );
    uint64_t draw_time_nanos =
      (draw_end_time - draw_start_time) * timebase_info.numer / timebase_info.denom;
    double draw_time_seconds = (double)draw_time_nanos / 1000000000.0;
#else
    struct timespec draw_end_time;
    clock_gettime (CLOCK_MONOTONIC, &draw_end_time);
    double draw_time_seconds =
      (double)(draw_end_time.tv_sec - draw_start_time.tv_sec) +
      (double)(draw_end_time.tv_nsec - draw_start_time.tv_nsec) / 1000000000.0;
#endif

    // Update timing statistics
    total_draw_time += draw_time_seconds;
    if (draw_time_seconds > max_draw_time) { max_draw_time = draw_time_seconds; }

    // Accumulate frame time and draw time for averaging
    accumulated_frame_time += frame_time_seconds;
    accumulated_draw_time += draw_time_seconds;
    accumulated_frame_count++;

    // Check if 0.5 seconds have elapsed since last FPS update
#ifdef __APPLE__
    uint64_t current_absolute_time = mach_absolute_time ( );
    uint64_t time_since_last_update_nanos =
      (current_absolute_time - last_fps_update_absolute_time) * timebase_info.numer /
      timebase_info.denom;
    double time_since_last_update = (double)time_since_last_update_nanos / 1000000000.0;
#else
    struct timespec current_absolute_time;
    clock_gettime (CLOCK_MONOTONIC, &current_absolute_time);
    double time_since_last_update =
      (double)(current_absolute_time.tv_sec - last_fps_update_time.tv_sec) +
      (double)(current_absolute_time.tv_nsec - last_fps_update_time.tv_nsec) /
        1000000000.0;
#endif

    if (time_since_last_update >= fps_update_interval)
    {
      // Calculate average frame time and draw time over the window
      current_avg_frame_time = accumulated_frame_time / (double)accumulated_frame_count;
      current_avg_draw_time  = accumulated_draw_time / (double)accumulated_frame_count;
      current_fps            = 1.0 / current_avg_frame_time;

      // Reset accumulators
      accumulated_frame_time  = 0.0;
      accumulated_draw_time   = 0.0;
      accumulated_frame_count = 0;

      // Update last update time
#ifdef __APPLE__
      last_fps_update_absolute_time = current_absolute_time;
#else
      last_fps_update_time = current_absolute_time;
#endif

      // Print frame time in real-time (update in place)
      printf (
        "\rFrame: %llu | Avg frame time: %.3f ms | FPS: %.2f | Avg draw: %.3f ms      ",
        (unsigned long long)frame_count,
        current_avg_frame_time * 1000.0,
        current_fps,
        current_avg_draw_time * 1000.0
      );
      fflush (stdout);
    }
  }

  // Print newline to finish the real-time output line
  printf ("\n");

  // Calculate and print frame statistics
  if (frame_count > 0)
  {
    double average_frame_time = total_frame_time / (double)frame_count;
    double average_fps        = 1.0 / average_frame_time;
    double total_time         = total_frame_time;
    double average_draw_time  = total_draw_time / (double)frame_count;
    double draw_percentage    = (total_draw_time / total_time) * 100.0;

    printf ("\n===== Frame Statistics =====\n");
    printf ("Total frames: %llu\n", (unsigned long long)frame_count);
    printf ("Total time: %.3f seconds\n", total_time);
    printf ("Average frame time: %.3f ms\n", average_frame_time * 1000.0);
    printf ("Worst frame time: %.3f ms\n", max_frame_time * 1000.0);
    printf ("Average FPS: %.2f\n", average_fps);
    printf ("Lowest FPS: %.2f\n", min_fps);
    printf ("\n----- Performance Breakdown -----\n");
    printf (
      "Average draw time: %.3f ms (%.1f%%)\n",
      average_draw_time * 1000.0,
      draw_percentage
    );
    printf ("Worst draw time: %.3f ms\n", max_draw_time * 1000.0);
    printf ("============================\n\n");
  }

  // Cleanup
  free (sprites);
  moss_destroy_sprite_batch (sprite_batch);
  moss_destroy_engine (engine);
  stuffy_window_close (g_window);
  stuffy_app_deinit ( );

  return EXIT_SUCCESS;
}
