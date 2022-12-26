#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : enable
// clang-format off
#include "ray_payload.glsl"
// clang-format on

layout(location = 0) rayPayloadInEXT RayPayload ray_payload;

void main() {
  ray_payload.t = -1.0;
}
