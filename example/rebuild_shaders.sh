#!/bin/bash

# Script to rebuild SPIR-V shaders from GLSL source files
# This script compiles GLSL shaders and updates the shader bytecode in the engine

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHADERS_DIR="${SCRIPT_DIR}/shaders"

# Check if glslc is available
if ! command -v glslc &> /dev/null; then
    echo "Error: glslc not found. Please install the Vulkan SDK."
    echo "On macOS: brew install vulkan-headers vulkan-loader vulkan-tools"
    exit 1
fi

echo "Compiling shaders..."

# Compile vertex shader
VERT_SRC="${SHADERS_DIR}/shader.vert"
VERT_SPV="${SHADERS_DIR}/shader.vert.spv"
if [ ! -f "${VERT_SRC}" ]; then
    echo "Error: Vertex shader source not found: ${VERT_SRC}"
    exit 1
fi

glslc "${VERT_SRC}" -o "${VERT_SPV}"
echo "  ✓ Compiled ${VERT_SRC} -> ${VERT_SPV}"

# Compile fragment shader
FRAG_SRC="${SHADERS_DIR}/shader.frag"
FRAG_SPV="${SHADERS_DIR}/shader.frag.spv"
if [ ! -f "${FRAG_SRC}" ]; then
    echo "Error: Fragment shader source not found: ${FRAG_SRC}"
    exit 1
fi

glslc "${FRAG_SRC}" -o "${FRAG_SPV}"
echo "  ✓ Compiled ${FRAG_SRC} -> ${FRAG_SPV}"

echo "  ✓ Updated ${ENGINE_SHADERS}"
echo ""
echo "Shader rebuild complete!"
