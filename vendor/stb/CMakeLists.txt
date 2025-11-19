cmake_minimum_required(VERSION 4.1.0)

project(stb)

set(SOURCE_FILES
  src/stb_image.c
  # Add new stb source files here...
)

add_library(stb STATIC ${SOURCE_FILES})

target_include_directories(stb PUBLIC include)
