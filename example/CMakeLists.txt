add_executable(example main.c)
target_link_libraries(example PRIVATE moss)
target_compile_options(example PRIVATE ${MOSS_COMPILE_OPTIONS})

# Copy shaders directory to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
     FILES_MATCHING
     PATTERN "*.spv"
     PATTERN "*.vert"
     PATTERN "*.frag"
)
