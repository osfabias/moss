#version 450

layout(binding = 0) uniform Camera {
  vec2 scale;
  vec2 offset;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    vec2 clipPosition = inPosition.xy * camera.scale + camera.offset;
    gl_Position = vec4(clipPosition.xy, inPosition.z, 1.0);
    fragTexCoord = inTexCoord;
}

