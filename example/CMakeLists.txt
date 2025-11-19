add_subdirectory(vendor/stuffy.c)

add_executable(example main.c)
target_link_libraries(example PRIVATE moss stuffy)
target_compile_options(example PRIVATE ${MOSS_COMPILE_OPTIONS})

add_executable(depth_test depth_test.c)
target_link_libraries(depth_test PRIVATE moss stuffy)
target_compile_options(depth_test PRIVATE ${MOSS_COMPILE_OPTIONS})

# Copy shaders directory to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
     FILES_MATCHING
     PATTERN "*.spv"
     PATTERN "*.vert"
     PATTERN "*.frag"
)

# Copy textures directory to build directory.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
     FILES_MATCHING
     PATTERN "*.png"
)
