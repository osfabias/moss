# Code Style Review List

This file contains the list of files to review for code style compliance.

## Review Guidelines

1. Functions should return either `MossResult` or `void`.
2. If a function accepts more than one parameter, they should be packed in a structure. Exception: method-like functions (e.g., `moss_set_render_resolution(engine, new_resolution)`) that accept a state structure as the first parameter are allowed.
3. If a function is expected to return a value (not a result code), it should be done via an out parameter.
4. One function returns one thing.

## Review Order

### Phase 1: Vulkan Utility Functions
These are header files with inline functions in `src/internal/vulkan/utils/`:

1. ✅ `src/internal/vulkan/utils/buffer.h` - **REVIEWED & REFACTORED**
2. ✅ `src/internal/vulkan/utils/command_buffer.h` - **REVIEWED & REFACTORED**
3. ✅ `src/internal/vulkan/utils/command_pool.h` - **REVIEWED & REFACTORED**
4. ✅ `src/internal/vulkan/utils/image_view.h` - **REVIEWED & REFACTORED**
5. ✅ `src/internal/vulkan/utils/image.h` - **REVIEWED & REFACTORED**
6. ✅ `src/internal/vulkan/utils/instance.h` - **REVIEWED & REFACTORED** (naming only)
7. ✅ `src/internal/vulkan/utils/physical_device.h` - **REVIEWED & REFACTORED** (naming only)
8. ✅ `src/internal/vulkan/utils/shader.h` - **REVIEWED & REFACTORED** (naming only)
9. ✅ `src/internal/vulkan/utils/swapchain.h` - **REVIEWED & REFACTORED** (naming only)
10. ✅ `src/internal/vulkan/utils/validation_layers.h` - **REVIEWED & REFACTORED** (naming only)

### Phase 2: Internal Utility Functions

11. ✅ `src/internal/memory_utils.h` - **REVIEWED & REFACTORED**
12. ✅ `src/internal/log.h` - **REVIEWED** (macros only, skipped)
13. ✅ `src/internal/app_info.h` - **REVIEWED & REFACTORED**

### Phase 3: Internal Engine Functions

14. ✅ `src/engine.c` (internal functions only, starting with `moss__`) - **REVIEWED & REFACTORED**

### Phase 4: Public API

14. `src/engine.c` (public functions: `moss_create_engine`, `moss_destroy_engine`, `moss_begin_frame`, `moss_end_frame`, `moss_set_render_resolution`)
15. `src/camera.c` (all functions)
16. `src/sprite_batch.c` (all functions)

## Notes

- Review files one at a time
- After each file, ask for approval before proceeding
- Document violations found and fixes applied

