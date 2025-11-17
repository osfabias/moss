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

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <moss/camera.h>
#include <moss/engine.h>
#include <moss/result.h>

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

/*
  @brief Updates camera based on keyboard and mouse input.
  @param camera Camera to update.
  @param window Window handle to get aspect ratio from.
  @param move_speed Camera movement speed in world units per frame.
  @param zoom_speed Camera zoom speed multiplier.
*/
static void update_camera_controls (
  MossCamera *const   camera,
  StuffyWindow *const window,
  const float         move_speed,
  const float         zoom_speed
)
{
  const StuffyKeyboardState *const keyboard = stuffy_keyboard_get_state ( );
  const StuffyMouseState *const    mouse    = stuffy_mouse_get_state ( );

  // Get window aspect ratio
  const StuffyExtent2D extent = stuffy_window_get_framebuffer_size (window);
  const float          aspect_ratio =
    (extent.height > 0) ? (float)extent.width / (float)extent.height : 1.0F;

  // Camera movement with WASD or Arrow keys
  float move_x = 0.0F;
  float move_y = 0.0F;

  if (keyboard->keys[ STUFFY_KEY_W ] || keyboard->keys[ STUFFY_KEY_UP ])
  {
    move_y += move_speed;
  }
  if (keyboard->keys[ STUFFY_KEY_S ] || keyboard->keys[ STUFFY_KEY_DOWN ])
  {
    move_y -= move_speed;
  }
  if (keyboard->keys[ STUFFY_KEY_A ] || keyboard->keys[ STUFFY_KEY_LEFT ])
  {
    move_x -= move_speed;
  }
  if (keyboard->keys[ STUFFY_KEY_D ] || keyboard->keys[ STUFFY_KEY_RIGHT ])
  {
    move_x += move_speed;
  }

  camera->position[ 0 ] += move_x;
  camera->position[ 1 ] += move_y;

  // Camera zoom with mouse wheel or +/- keys
  float zoom_factor = 1.0F;
  if (mouse->scroll != 0.0F)
  {
    // Mouse scroll: positive scroll = zoom in, negative = zoom out
    zoom_factor = 1.0F + (mouse->scroll * zoom_speed);
  }
  else if (keyboard->keys[ STUFFY_KEY_EQUAL ] || keyboard->keys[ STUFFY_KEY_ADD ])
  {
    // Zoom in with + or =
    zoom_factor = 1.0F + zoom_speed;
  }
  else if (keyboard->keys[ STUFFY_KEY_MINUS ] || keyboard->keys[ STUFFY_KEY_SUBTRACT ])
  {
    // Zoom out with - or numpad -
    zoom_factor = 1.0F - zoom_speed;
  }

  // Apply zoom while maintaining aspect ratio
  // Use height as reference and calculate width based on aspect ratio
  const float min_height = 10.0F;
  float       new_height = camera->size[ 1 ] * zoom_factor;

  // Clamp minimum size
  if (new_height < min_height) { new_height = min_height; }

  // Calculate width to maintain aspect ratio
  // This ensures the camera size always matches the window aspect ratio,
  // even when the window is resized
  float new_width = new_height * aspect_ratio;

  camera->size[ 0 ] = new_width;
  camera->size[ 1 ] = new_height;
}

int main (void)
{
  // Initialize stuffy app
  if (stuffy_app_init ( ) != 0) { return EXIT_FAILURE; }

  // Create window
  const StuffyWindowConfig window_config = {
    .title      = "Moss Example Application",
    .rect       = { .x = 128, .y = 128, .width = 640, .height = 360 },
    .style_mask = STUFFY_WINDOW_STYLE_TITLED_BIT | STUFFY_WINDOW_STYLE_CLOSABLE_BIT |
                  STUFFY_WINDOW_STYLE_RESIZABLE_BIT | STUFFY_WINDOW_STYLE_ICONIFIABLE_BIT,
  };

  g_window = stuffy_window_open (&window_config);
  if (g_window == NULL)
  {
    stuffy_app_deinit ( );
    return EXIT_FAILURE;
  }

#ifdef __APPLE__
  // Get metal layer from window
  void *metal_layer = stuffy_window_get_metal_layer (g_window);
  if (metal_layer == NULL)
  {
    stuffy_window_close (g_window);
    stuffy_app_deinit ( );
    return EXIT_FAILURE;
  }
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
  if (engine == NULL)
  {
    stuffy_window_close (g_window);
    stuffy_app_deinit ( );
    return EXIT_FAILURE;
  }

  MossCamera *const camera = moss_get_camera (engine);

  camera->position[ 0 ] = 0.0F;
  camera->position[ 1 ] = 0.0F;

  // Initialize camera size based on window aspect ratio
  const StuffyExtent2D initial_extent = stuffy_window_get_framebuffer_size (g_window);
  const float          initial_aspect_ratio =
    (initial_extent.height > 0)
               ? (float)initial_extent.width / (float)initial_extent.height
               : 16.0F / 9.0F;

  const float initial_height = 180.0F;  // Base height in world units
  camera->size[ 0 ]          = initial_height * initial_aspect_ratio;
  camera->size[ 1 ]          = initial_height;

  // Camera control parameters
  const float camera_move_speed = 1.0F;   // World units per frame
  const float camera_zoom_speed = 0.01F;  // Zoom multiplier

  // Main loop
  while (!stuffy_window_should_close (g_window))
  {
    stuffy_app_update ( );

    // Update camera controls (maintains aspect ratio automatically)
    update_camera_controls (camera, g_window, camera_move_speed, camera_zoom_speed);

    if (moss_update_engine (engine) != MOSS_RESULT_SUCCESS) { break; }
  }

  // Cleanup
  moss_destroy_engine (engine);
  stuffy_window_close (g_window);
  stuffy_app_deinit ( );

  return EXIT_SUCCESS;
}
